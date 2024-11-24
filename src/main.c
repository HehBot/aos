#include <acpi.h>
#include <bootinfo.h>
#include <cpu/interrupt.h>
#include <cpu/ioapic.h>
#include <cpu/mp.h>
#include <cpu/x86.h>
#include <drivers/ega.h>
#include <fs/fs.h>
#include <fs/initrd.h>
#include <hash_table.h>
#include <memory/frame_allocator.h>
#include <memory/kalloc.h>
#include <memory/paging.h>
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

void stack_overflow(size_t i)
{
    printf("%s %lx\n", __func__, i);
    stack_overflow(i + 1);
}

void main(phys_addr_t phys_addr_mboot_info)
{
    struct multiboot_info mboot_info = parse_mboot_info(kernel_static_from_phys_addr(phys_addr_mboot_info));

    init_ega(mboot_info.tag_framebuffer, NULL);
    ega_clear();
    printf("Hello from C!\n");

    section_info_t section_info[4];
    parse_elf_section_info(mboot_info.tag_elf_sections, &section_info);

    init_frame_allocator(mboot_info.tag_mmap);

    init_paging(section_info, sizeof(section_info) / sizeof(section_info[0]));

    virt_addr_t heap_start = (virt_addr_t)0xffff800000000000;

    init_ega(mboot_info.tag_framebuffer, &heap_start);

    acpi_info_t acpi_info = parse_acpi(mboot_info.tag_old_acpi, mboot_info.tag_new_acpi, &heap_start);
    init_lapic(&heap_start, acpi_info.lapic_addr);

    init_idt();
    init_cpu(); // needs get_cpu which needs lapic initialised

    /*
     * now we can start using spinlocks
     */
    ega_enable_lock();

    init_ioapic(&heap_start, acpi_info.ioapic_addr, acpi_info.ioapic_id);
    ioapic_enable(IRQ_KBD, get_cpu()->lapic_id);

    /*
     * all required frames have been reserved
     * now we can safely use frame_allocator_get_frame
     */
    init_kpalloc(heap_start);

    pgdir_t kpgdir = paging_new_pgdir();

    /*
     * stack overflow test
     */
    // stack_overflow(0);

    /*
     * heap NXE test
     */
    // uint8_t* instr = kmalloc(20);
    // instr[0] = 0x90;
    // instr[1] = 0xc3;
    // asm volatile("call %0"::"r"(instr));

    __sync_synchronize();
    enable_interrupts();

    while (1)
        hlt();
}
