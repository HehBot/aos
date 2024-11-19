#include <acpi.h>
#include <cpu/interrupt.h>
#include <cpu/ioapic.h>
#include <cpu/mp.h>
#include <cpu/x86.h>
#include <drivers/screen.h>
#include <fs/fs.h>
#include <fs/initrd.h>
#include <hash_table.h>
#include <memory/frame_allocator.h>
#include <memory/kalloc.h>
#include <multiboot2.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct multiboot_info {
    struct multiboot_tag_mmap const* tag_mmap;
    struct multiboot_tag_elf_sections const* tag_elf_sections;
    struct multiboot_tag_framebuffer const* tag_framebuffer;
    struct multiboot_tag_old_acpi const* tag_old_acpi;
    struct multiboot_tag_new_acpi const* tag_new_acpi;
    size_t size_reserved;
};

struct multiboot_info parse_mboot_info(void* mboot_info)
{
    struct multiboot_info info = { 0 };
    uint8_t* p = (uint8_t*)mboot_info;
    info.size_reserved = *((size_t*)p);
    p += 8;
    int done = 0;
    while (!done) {
        struct multiboot_tag* tag = (struct multiboot_tag*)p;
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_MMAP:
            info.tag_mmap = (void*)tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
            info.tag_elf_sections = (void*)tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            info.tag_framebuffer = (void*)tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_ACPI_NEW:
            info.tag_new_acpi = (void*)tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_ACPI_OLD:
            info.tag_old_acpi = (void*)tag;
            goto end;
        end:
        default:
#define ROUND_UP_8(x) ((((x) + 7) >> 3) << 3)
            p += ROUND_UP_8(tag->size);
#undef ROUND_UP_8
            continue;
        case MULTIBOOT_TAG_TYPE_END:
            done = 1;
            break;
        }
    }
    return info;
}

// #define MAX_MODS 10
// static mem_map_t mod_mem_map[MAX_MODS];
// static size_t nr_mods = 0;
// static void map_modules()
// {
//     for (size_t i = 0; i < nr_mods; ++i) {
//         uintptr_t pa = mod_mem_map[i].phy_start;
//         size_t nr_pages = PG_ROUND_UP(mod_mem_map[i].phy_end - pa) / PAGE_SIZE;
//         size_t len = mod_mem_map[i].phy_end - pa;
//         uintptr_t va = (uintptr_t)map_phy(pa, len, PTE_W);
//         mod_mem_map[i].virt = va;
//         mod_mem_map[i].nr_pages = nr_pages;
//         mod_mem_map[i].mapped = 1;
//     }
// }

typedef struct elf_section_header {
    uint32_t name; // Offset into the string table
    uint32_t type; // Section type
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
    uint64_t flags; // Section attributes
    uint64_t addr; // Memory address
    uint64_t offset; // Offset in file
    uint64_t size; // Section size
    uint32_t link; // Associated section
    uint32_t info; // Extra information
    uint64_t addralign; // Alignment
    uint64_t entsize; // Entry size
} const* elf_section_header_t;

uint8_t init_gdt()
{
    uint8_t double_fault_ist = 0;

    static tss_t tss;
    static uint8_t excep_stack[256] __attribute__((__aligned__(16)));
    static gdt_entry_t gdt[NR_GDT_ENTRIES] = {
        [KERNEL_CODE_GDT_INDEX] = { {} },
        [TSS_GDT_INDEX] = { {} },
        [TSS_GDT_INDEX + 1] = { {} },
    };

    tss.ist[double_fault_ist] = (uint64_t)&excep_stack[sizeof(excep_stack)];
    tss.iomap_base = sizeof(tss_t);

    // to verify that excep_stack is indeed used, uncomment
    //      the next line and see that `int $T_DOUBLE_FAULT` causes #GPF
    // tss.ist[double_fault_ist] = 0x8fffffffffff;

    gdt[KERNEL_CODE_GDT_INDEX] = SEG(KERNEL_PL, 1, 0);
    gdt[TSS_GDT_INDEX] = SEG_TSS1(KERNEL_PL, (uintptr_t)&tss, sizeof(tss));
    gdt[TSS_GDT_INDEX + 1] = SEG_TSS2((uintptr_t)&tss);

    lgdt(&gdt[0], sizeof(gdt), KERNEL_CODE_SEG);
    ltr(TSS_SEG);

    return double_fault_ist;
}

typedef struct {
    char const* name;
    pte_flags_t flags;
    elf_section_header_t section;
} section_info_t;

static void get_section_info(struct multiboot_tag_elf_sections const* e, section_info_t* requested_section_info, size_t nr_sections)
{
    elf_section_header_t shstrtab = (void*)&e->sections[e->shndx * e->entsize];
    char const* strings = (void*)shstrtab->addr;
    for (size_t i = 0; i < e->num; ++i) {
        elf_section_header_t section = (void const*)&e->sections[i * e->entsize];
        if ((section->flags & SHF_ALLOC) == 0)
            continue;
        char const* name = &strings[section->name];
        for (size_t i = 0; i < nr_sections; ++i) {
            if (!memcmp(requested_section_info[i].name, name, strlen(requested_section_info[i].name))) {
                requested_section_info[i].section = section;
                break;
            }
        }
    }
}

static void reserve_kernel_frames(section_info_t* section_info, size_t nr_sections)
{
    for (size_t i = 0; i < nr_sections; ++i) {
        elf_section_header_t section = section_info[i].section;
        if (section != NULL) {
            phys_addr_t first_frame = phys_addr_of_kernel_static((virt_addr_t)PAGE_ROUND_DOWN(section->addr));
            phys_addr_t last_frame = phys_addr_of_kernel_static((virt_addr_t)PAGE_ROUND_DOWN(section->addr + section->size - 1));
            for (phys_addr_t p = first_frame; p <= last_frame; p += PAGE_SIZE)
                pmm_reserve_frame(p);
        }
    }
}

void main(phys_addr_t phys_addr_mboot_info)
{
    /*
     * set up gdt and idt
     */
    uint8_t double_fault_ist = init_gdt();
    init_idt(double_fault_ist);

    asm volatile("int3");

    struct multiboot_info mboot_info = parse_mboot_info(kernel_static_from_phys_addr(phys_addr_mboot_info));

    init_screen(mboot_info.tag_framebuffer, 0);
    printf("Hello from C!\n");

    /*
     * get section info from multiboot
     * we want sections .text, .rodata, .data, .bss
     */
    section_info_t section_info[4] = {
        { ".text", PTE_NX, NULL },
        { ".rodata", 0, NULL },
        { ".data", PTE_W, NULL },
        { ".bss", PTE_W, NULL },
    };
    size_t nr_sections = sizeof(section_info) / sizeof(section_info[0]);
    get_section_info(mboot_info.tag_elf_sections, section_info, nr_sections);
    if (section_info[0].section == NULL)
        PANIC("No text section");

    init_pmm(mboot_info.tag_mmap);
    reserve_kernel_frames(section_info, nr_sections);
    /*
     * reserve multiboot info frames
     */
    for (phys_addr_t p = PAGE_ROUND_DOWN(phys_addr_mboot_info); p <= PAGE_ROUND_DOWN(phys_addr_mboot_info + mboot_info.size_reserved); p += PAGE_SIZE)
        pmm_reserve_frame(p);

    // init_mm();

    // init_screen(fb_info);

    // acpi_info_t acpi_info = init_acpi(mboot_info.tag_old_acpi->rsdp);
    // TODO reclaim MULTIBOOT_MEMORY_ACPI_RECLAIMABLE entries

    // init_lapic(acpi_info.lapic_addr);
    // init_seg(); // needs get_cpu which needs lapic initialised
    // init_ioapic(acpi_info.ioapic_addr, acpi_info.ioapic_id);

    // ioapic_enable(IRQ_KBD, get_cpu()->lapic_id);

    printf("Haven't crashed\n");

    // __sync_synchronize();
    // asm volatile("sti");
}
