#include <cpu/interrupt.h>
#include <cpu/x86.h>
#include <drivers/ata.h>
#include <drivers/keyboard.h>
#include <drivers/screen.h>
#include <drivers/timer.h>
#include <fs/fs.h>
#include <fs/initrd.h>
#include <liballoc.h>
#include <mem/kwmalloc.h>
#include <mem/mm.h>
#include <mem/pmm.h>
#include <multiboot.h>
#include <proc/proc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

multiboot_info_t mboot_info;
uint32_t mboot_magic;

extern mem_map_t kernel_mem_map[];
static mem_map_t* mod_mem_map;

void main(void)
{
    if (mboot_magic != MULTIBOOT_BOOTLOADER_MAGIC
        || !(mboot_info.flags & MULTIBOOT_INFO_MEM_MAP)
        || !(mboot_info.flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO))
        return;

    // 0. read module mappings
    multiboot_module_t* mods = (multiboot_module_t*)(mboot_info.mods_addr + KERN_BASE);
    mod_mem_map = kwmalloc((mboot_info.mods_count + 1) * (sizeof mod_mem_map[0]));
    for (size_t i = 0; i < mboot_info.mods_count; ++i) {
        mem_map_t m = { 1, 0, PTE_W, 0, { { mods[i].mod_start, mods[i].mod_end } } };
        mod_mem_map[i] = m;
    }

    // 1. initialise pmm
    init_pmm(&mboot_info);
    for (size_t i = 0; kernel_mem_map[i].present; ++i)
        if (kernel_mem_map[i].mapped)
            for (uintptr_t phy = kernel_mem_map[i].phy_start; phy < kernel_mem_map[i].phy_end; phy += PAGE_SIZE)
                pmm_reserve_frame(phy);
    for (size_t i = 0; mod_mem_map[i].present; ++i)
        for (uintptr_t phy = mod_mem_map[i].phy_start; phy < mod_mem_map[i].phy_end; phy += PAGE_SIZE)
            pmm_reserve_frame(phy);

    // 2. initialise mm and kernel heap
    init_mm();

    // 4. initialise screen
    init_screen(&mboot_info);

    // 5. initialise tss
    init_tss();

    // 6. initialise ata
    ata_init();

    // 6. set up user addr space
    size_t d = alloc_page_directory();
    switch_page_directory(d);

    void* user_code = 0x0;
    map_page(pmm_get_frame(), (uintptr_t)user_code, PTE_W | PTE_U);
    volatile int flag = 0;
    ata_req(1, 0, user_code, &flag);
    while (!flag)
        ;

    void* user_stack = (void*)(KERN_BASE - 16);
    map_page(pmm_get_frame(), (uintptr_t)user_stack, PTE_W | PTE_U);

    void* kstack = kmalloc(0x500);

    set_tss(kstack);
    cpu_state_t c = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, USER_DATA_SEG, USER_DATA_SEG, USER_DATA_SEG, USER_DATA_SEG, 0x0, 0x0, (uintptr_t)user_code, USER_CODE_SEG, 0x0, (uintptr_t)user_stack, USER_DATA_SEG };
    enter_usermode(&c);
}
