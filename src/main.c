#include <acpi.h>
#include <cpu/interrupt.h>
#include <cpu/ioapic.h>
#include <cpu/mp.h>
#include <cpu/x86.h>
#include <drivers/screen.h>
#include <fs/fs.h>
#include <fs/initrd.h>
#include <hash_table.h>
#include <liballoc.h>
#include <mem/kpalloc.h>
#include <mem/kwmalloc.h>
#include <mem/mm.h>
#include <mem/pmm.h>
#include <multiboot2.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

size_t block_size;

extern mem_map_t kernel_mem_map[];

#define MAX_MODS 10
static mem_map_t mod_mem_map[MAX_MODS];
static size_t nr_mods = 0;

static void* mmap_info = NULL;
static void* fb_info = NULL;
static void* rsdp = NULL;

void parse_mboot_info(uintptr_t __mboot_info)
{
    uint8_t* p = ((uint8_t*)__mboot_info) + 8;
    int done = 0;
    while (!done) {
        struct multiboot_tag* tag = (struct multiboot_tag*)p;
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_END:
            done = 1;
            break;
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            fb_info = tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_MMAP:
            mmap_info = tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_ACPI_NEW:
        case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
            struct multiboot_tag_new_acpi* tag_acpi = (struct multiboot_tag_new_acpi*)tag;
            rsdp = tag_acpi->rsdp;
        }
            goto end;
        case MULTIBOOT_TAG_TYPE_MODULE: {
            if (nr_mods < MAX_MODS) {
                struct multiboot_tag_module* tag_module = (struct multiboot_tag_module*)tag;
                mem_map_t m = { 1, 0, 0, 0, { { tag_module->mod_start, tag_module->mod_end } } };
                mod_mem_map[nr_mods] = m;
                nr_mods++;
            }
        }
            goto end;
        end:
        default:
#define ROUND_UP_8(x) ((((x) + 7) >> 3) << 3)
            p += ROUND_UP_8(tag->size);
#undef ROUND_UP_8
            continue;
        }
    }
}

static void map_modules()
{
    for (size_t i = 0; i < nr_mods; ++i) {
        uintptr_t pa = mod_mem_map[i].phy_start;
        size_t nr_pages = PG_ROUND_UP(mod_mem_map[i].phy_end - pa) / PAGE_SIZE;
        size_t len = mod_mem_map[i].phy_end - pa;
        uintptr_t va = (uintptr_t)map_phy(pa, len, PTE_W);
        mod_mem_map[i].virt = va;
        mod_mem_map[i].nr_pages = nr_pages;
        mod_mem_map[i].mapped = 1;
    }
}

void __attribute__((cdecl)) main(uint32_t mboot_magic, uintptr_t __mboot_info)
{
    if (mboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC)
        return;
    parse_mboot_info(__mboot_info);

    // TODO reclaim MULTIBOOT_MEMORY_ACPI_RECLAIMABLE entries
    init_pmm(mmap_info);
    for (size_t i = 0; kernel_mem_map[i].present; ++i)
        if (kernel_mem_map[i].mapped)
            for (uintptr_t phy = kernel_mem_map[i].phy_start; phy < kernel_mem_map[i].phy_end; phy += PAGE_SIZE)
                pmm_reserve_frame(phy);
    for (size_t i = 0; i < nr_mods; ++i)
        for (uintptr_t phy = mod_mem_map[i].phy_start; phy < mod_mem_map[i].phy_end; phy += PAGE_SIZE)
            pmm_reserve_frame(phy);

    init_mm();
    map_modules(mod_mem_map, nr_mods);

    init_screen(fb_info);

    acpi_info_t acpi_info = init_acpi(rsdp);

    acpi_info.lapic_addr++;

    init_lapic(acpi_info.lapic_addr);
    init_seg(); // needs get_cpu which needs lapic initialised
    init_ioapic(acpi_info.ioapic_addr, acpi_info.ioapic_id);

    init_isrs();

    void init_timer(uint32_t freq);

    ioapic_enable(IRQ_KBD, get_cpu()->lapic_id);

    init_idt();

    __sync_synchronize();
    asm volatile("sti");
}
