#include "kmalloc.h"
#include "mm.h"
#include "multiboot.h"
#include "pmm.h"
#include "string.h"

#include <drivers/keyboard.h>
#include <drivers/screen.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x86.h>

extern gdt_desc_t entry_gdt_desc;
page_directory_t kernel_page_dir __attribute__((__aligned__(PAGE_SIZE)));
multiboot_info_t mboot_info;
uint32_t mboot_magic;

uintptr_t kernel_start, kernel_end, kernel_physical_start, kernel_physical_end;
uintptr_t stack_bottom, stack_size;

extern pde_t entry_pgdir[];

void main(void)
{
    if (mboot_magic != MULTIBOOT_BOOTLOADER_MAGIC
        || !(mboot_info.flags & MULTIBOOT_INFO_MEM_MAP)
        || !(mboot_info.flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO))
        return;

    // 1. initialise kernel malloc
    kmallocinit();

    // 2. initialise pmm
    pmm_init(mboot_info);
    for (uintptr_t phys = kernel_physical_start; phys < kernel_physical_end; phys += PAGE_SIZE)
        pmm_reserve_frame(phys);

    memset(&kernel_page_dir, 0, sizeof(kernel_page_dir));
    kernel_page_dir.physical_addr = ((uintptr_t)&kernel_page_dir.tables_physical - KERN_BASE);

    // 3. set up kernel pgdir and tables
    // 3(a). set up meta page table in current pgdir and kernel pgdir
    uintptr_t pgtb_phy = pmm_get_frame();
    entry_pgdir[0x3ff] = LPG_ROUND_DOWN(pgtb_phy) | PTE_LP | PTE_W | PTE_P;
    page_table_t* pgtb = (void*)(0xffc00000 + (pgtb_phy - LPG_ROUND_DOWN(pgtb_phy)));
    memset(pgtb->pages, 0, sizeof(pgtb->pages));
    pgtb->pages[0] = pgtb_phy | PTE_W | PTE_P;
    entry_pgdir[0x3ff] = pgtb_phy | PTE_W | PTE_P;
    pgtb = (void*)0xffc00000;

    kernel_page_dir.tables[0x3ff] = pgtb;
    kernel_page_dir.tables_physical[0x3ff] = pgtb_phy | PTE_W | PTE_P;

    // 3(b). map kernel
    // Phy p <=> Virt (p + KERN_BASE)
    for (uintptr_t virt = kernel_start; virt < kernel_end; virt += PAGE_SIZE) {
        pte_t* z = get_pte(&kernel_page_dir, virt, 1);
        memset(z, 0, sizeof(*z));
        *z = (virt - KERN_BASE) | PTE_W | PTE_P;
    }

    // 3(c). switch to kernel pgdir
    switch_page_directory(&kernel_page_dir);
    pmm_free_frame((uintptr_t)entry_pgdir - KERN_BASE);
    pte_t* pte = get_pte(&kernel_page_dir, (uintptr_t)entry_pgdir, 0);
    *pte = 0;

    // 4. initialise screen
    uintptr_t fb_addr = 0xc03ff000;
    pte = get_pte(&kernel_page_dir, fb_addr, 1);
    *pte = (mboot_info.framebuffer_addr_low & 0xfffff000) | PTE_W | PTE_P;
    size_t fb_size = (mboot_info.framebuffer_bpp >> 3) * mboot_info.framebuffer_width * mboot_info.framebuffer_height;
    init_screen(fb_addr /*, mboot_info->framebuffer_type*/);

    printf("\
Kernel      0x%x - 0x%x (Phy0x%x - Phy0x%x)\n\
    GDT         Desc=0x%x, Size=%d, Loc=0x%x\n\
    Stack       0x%x - 0x%x\n\
FB          0x%x - 0x%x (Phy0x%x - Phy0x%x)\n\
",
           kernel_start, kernel_end, kernel_physical_start, kernel_physical_end,
           (uintptr_t)&entry_gdt_desc, entry_gdt_desc.size, (uintptr_t)entry_gdt_desc.gdt,
           stack_bottom, stack_bottom + stack_size,
           fb_addr, fb_addr + fb_size, mboot_info.framebuffer_addr_low, mboot_info.framebuffer_addr_low + fb_size);

    char* buf = (void*)0x0;
    pte = get_pte(&kernel_page_dir, (uintptr_t)buf, 1);
    uintptr_t phy = pmm_get_frame();
    *pte = phy | PTE_W | PTE_P;

    buf[0] = 'a';

    init_keyboard();
}
