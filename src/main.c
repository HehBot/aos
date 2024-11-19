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
    phys_addr_t start, end;
    int present;
} section_info_t;

static void get_section_info(struct multiboot_tag_elf_sections const* e, section_info_t (*section_info)[4])
{
    section_info_t si[4] = {
        { ".text", PTE_NX, 0, 0, 0 },
        { ".rodata", 0, 0, 0, 0 },
        { ".data", PTE_W, 0, 0, 0 },
        { ".bss", PTE_W, 0, 0, 0 },
    };

    elf_section_header_t shstrtab = (void*)&e->sections[e->shndx * e->entsize];
    char const* strings = (void*)shstrtab->addr;
    for (size_t i = 0; i < e->num; ++i) {
        elf_section_header_t section = (void const*)&e->sections[i * e->entsize];
        if ((section->flags & SHF_ALLOC) == 0)
            continue;
        char const* name = &strings[section->name];
        for (size_t i = 0; i < 4; ++i) {
            if (!memcmp(si[i].name, name, strlen(si[i].name))) {
                si[i].start = section->addr;
                si[i].end = section->addr + section->size;
                si[i].present = 1;
                break;
            }
        }
    }
    if (!si[0].present)
        PANIC("No text section");
    /*
     * coalesce .data and .bss, we know they will be consecutive
     */
    if (si[2].present && si[3].present) {
        si[2].name = ".data+.bss";
        if (si[2].start < si[3].start) {
            if (si[2].end != si[3].start)
                PANIC("Bad linker script");
            si[2].end = si[3].end;
        } else {
            if (si[3].end != si[2].start)
                PANIC("Bad linker script");
            si[2].start = si[3].start;
        }
        si[3] = (section_info_t) {};
    }

    memcpy(section_info, &si, sizeof(si));
}

static void reserve_kernel_frames(section_info_t* section_info, size_t nr_sections)
{
    for (size_t i = 0; i < nr_sections; ++i) {
        if (section_info[i].present) {
            phys_addr_t section_start = phys_addr_of_kernel_static((virt_addr_t)section_info[i].start);
            phys_addr_t section_end = phys_addr_of_kernel_static((virt_addr_t)section_info[i].end);
            printf("Section %s: 0x%lx - 0x%lx\n", section_info[i].name, section_start, section_end);
            phys_addr_t first_frame = PAGE_ROUND_DOWN(section_start);
            phys_addr_t last_frame = PAGE_ROUND_DOWN(section_end - 1);
            for (phys_addr_t p = first_frame; p <= last_frame; p += PAGE_SIZE)
                if (frame_allocator_reserve_frame(p) != FRAME_ALLOCATOR_OK)
                    PANIC("Unable to reserve kernel frames");
        }
    }
}

void main(phys_addr_t phys_addr_mboot_info)
{
    struct multiboot_info mboot_info = parse_mboot_info(kernel_static_from_phys_addr(phys_addr_mboot_info));

    init_screen(mboot_info.tag_framebuffer, NULL);
    clear_screen();
    printf("Hello from C!\n");

    /*
     * set up gdt and idt
     */
    uint8_t double_fault_ist = init_gdt();
    init_idt(double_fault_ist);

    asm volatile("int3");

    /*
     * get section info from multiboot
     */
    section_info_t section_info[4];
    get_section_info(mboot_info.tag_elf_sections, &section_info);

    /*
     * initialise frame allocator and reserve kernel frames
     * TODO also reserve multiboot info frames
     */
    init_frame_allocator(mboot_info.tag_mmap);
    reserve_kernel_frames(section_info, sizeof(section_info) / sizeof(section_info[0]));

    /*
     * initialise paging
     */
    init_paging();

    virt_addr_t mmio_devices = (virt_addr_t)0x2000000;
    mmio_devices = init_screen(mboot_info.tag_framebuffer, mmio_devices);

    /*
     * TODO reclaim MULTIBOOT_MEMORY_ACPI_RECLAIMABLE entries
     */
    // acpi_info_t acpi_info = init_acpi(mboot_info.tag_old_acpi->rsdp);

    // init_lapic(acpi_info.lapic_addr);
    // init_seg(); // needs get_cpu which needs lapic initialised

    // init_ioapic(acpi_info.ioapic_addr, acpi_info.ioapic_id);
    // ioapic_enable(IRQ_KBD, get_cpu()->lapic_id);

    /*
     * all required frames have been reserved
     * now we can safely use frame_allocator_get_frame
     */

    virt_addr_t heap_start = (virt_addr_t)0xffff800000000000;
    size_t heap_size = 0x200000;
    for (size_t i = 0; i < heap_size / PAGE_SIZE; ++i) {
        phys_addr_t frame = frame_allocator_get_frame();
        if (frame == FRAME_ALLOCATOR_ERROR_NO_FRAME_AVAILABLE)
            PANIC("Unable to allocate frames for heap");
        int err = paging_map(heap_start + i * PAGE_SIZE, frame, PAGE_4KiB, PTE_W | PTE_P);
        if (err != PAGING_OK)
            PANIC("Unable to map heap");
    }

    for (size_t i = 0; i < heap_size / PAGE_SIZE; ++i) {
        uint64_t* x = heap_start + i * PAGE_SIZE;
        *x = 0x123456789abcdef0;
        if (*x != 0x123456789abcdef0)
            PANIC("read != write");
    }

    int err = paging_update_flags(heap_start, PAGE_4KiB, PTE_P);
    if (err != PAGING_OK)
        PANIC("heh");
    uint64_t* x = heap_start;
    *x = 0x123456789abcdef0;

    printf("Haven't crashed\n");

    while (1)
        ;

    // __sync_synchronize();
    // asm volatile("sti");
}
