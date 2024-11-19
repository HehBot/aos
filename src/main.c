#include <acpi.h>
#include <bootinfo.h>
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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

static void reserve_kernel_frames(section_info_t* section_info, size_t nr_sections)
{
    for (size_t i = 0; i < nr_sections; ++i) {
        if (section_info[i].present) {
            phys_addr_t section_start = phys_addr_of_kernel_static((virt_addr_t)section_info[i].start);
            phys_addr_t section_end = phys_addr_of_kernel_static((virt_addr_t)section_info[i].end);
            phys_addr_t first_frame = PAGE_ROUND_DOWN(section_start);
            phys_addr_t last_frame = PAGE_ROUND_DOWN(section_end - 1);
            for (phys_addr_t p = first_frame; p <= last_frame; p += PAGE_SIZE) {
                int err = frame_allocator_reserve_frame(p);
                if (err != FRAME_ALLOCATOR_OK) {
                    printf("%d\n", err);
                    PANIC("Unable to reserve kernel frames");
                }
            }
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
     * get elf section info from multiboot
     */
    section_info_t section_info[4];
    parse_elf_section_info(mboot_info.tag_elf_sections, &section_info);

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

    virt_addr_t useful_memory_base = (virt_addr_t)0x2000000;
    init_screen(mboot_info.tag_framebuffer, &useful_memory_base);

    /*
     * TODO reclaim MULTIBOOT_MEMORY_ACPI_RECLAIMABLE entries
     */
    acpi_info_t acpi_info = parse_acpi(mboot_info.tag_old_acpi->rsdp, &useful_memory_base);

    int err = paging_map(useful_memory_base, acpi_info.lapic_addr, PAGE_4KiB, PTE_W | PTE_P);
    if (err != PAGING_OK)
        printf("Unable to map lapic register");
    init_lapic(useful_memory_base);
    useful_memory_base += PAGE_SIZE;

    init_idt();
    init_cpu(); // needs get_cpu which needs lapic initialised

    asm volatile("int3");
    // asm volatile("int $0x8");

    err = paging_map(useful_memory_base, acpi_info.ioapic_addr, PAGE_4KiB, PTE_W | PTE_P);
    if (err != PAGING_OK)
        printf("Unable to map ioapic register");
    init_ioapic(useful_memory_base, acpi_info.ioapic_id);
    useful_memory_base += PAGE_SIZE;
    ioapic_enable(IRQ_KBD, get_cpu()->lapic_id);

    /*
     * all required frames have been reserved
     * now we can safely use frame_allocator_get_frame
     */

    /*
     * set up kernel heap
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

    printf("Haven't crashed\n");

    __sync_synchronize();
    asm volatile("sti");

    while (1)
        ;
}
