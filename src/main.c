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

void main(phys_addr_t phys_addr_mboot_info)
{
    struct multiboot_info mboot_info = parse_mboot_info(kernel_static_from_phys_addr(phys_addr_mboot_info));

    init_ega(mboot_info.tag_framebuffer, NULL);
    ega_clear();
    printf("Hello from C!\n");

    section_info_t section_info[4];
    parse_elf_section_info(mboot_info.tag_elf_sections, &section_info);

    init_frame_allocator(mboot_info.tag_mmap);

    struct multiboot_tag_module const* m = mboot_info.tag_initrd_module;
    phys_addr_t mod_start_frame = PAGE_ROUND_DOWN(m->mod_start);
    phys_addr_t mod_end_frame = PAGE_ROUND_DOWN(m->mod_end - 1);
    for (phys_addr_t frame = mod_start_frame; frame <= mod_end_frame; frame += PAGE_SIZE)
        ASSERT(frame_allocator_reserve_frame(frame) == FRAME_ALLOCATOR_OK);

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

    __sync_synchronize();
    enable_interrupts();

    while (1)
        hlt();
}
