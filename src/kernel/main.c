#include "kwmalloc.h"
#include "mm.h"
#include "multiboot.h"
#include "pmm.h"
#include "string.h"

#include <drivers/keyboard.h>
#include <drivers/screen.h>
#include <fs/fs.h>
#include <fs/initrd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x86.h>

extern gdt_desc_t entry_gdt_desc;
extern pde_t entry_pgdir[];

static page_directory_t kernel_page_dir __attribute__((__aligned__(PAGE_SIZE)));

multiboot_info_t mboot_info;
uint32_t mboot_magic;

uintptr_t kernel_start, kernel_end, kernel_physical_start, kernel_physical_end;
uintptr_t stack_bottom, stack_size;

void init_kernel_pgdir(void)
{
    memset(&kernel_page_dir, 0, sizeof(kernel_page_dir));
    kernel_page_dir.physical_addr = ((uintptr_t)&kernel_page_dir.tables_physical - KERN_BASE);

    // 1. set up meta page table in current pgdir and kernel pgdir
    uintptr_t pgtb_phy = pmm_get_frame();
    entry_pgdir[0x3ff] = LPG_ROUND_DOWN(pgtb_phy) | PTE_LP | PTE_W | PTE_P;
    page_table_t* pgtb = (void*)(0xffc00000 + (pgtb_phy - LPG_ROUND_DOWN(pgtb_phy)));
    memset(pgtb->pages, 0, sizeof(pgtb->pages));
    pgtb->pages[0] = pgtb_phy | PTE_W | PTE_P;
    entry_pgdir[0x3ff] = pgtb_phy | PTE_W | PTE_P;
    pgtb = (void*)0xffc00000;

    kernel_page_dir.tables[0x3ff] = pgtb;
    kernel_page_dir.tables_physical[0x3ff] = pgtb_phy | PTE_W | PTE_P;

    // 2. map kernel
    // Phy p <=> Virt (p + KERN_BASE)
    for (uintptr_t virt = kernel_start; virt < kernel_end; virt += PAGE_SIZE)
        map_page(&kernel_page_dir, virt - KERN_BASE, virt, PTE_W);

    // 3. switch to kernel pgdir
    switch_page_directory(&kernel_page_dir);
    unmap_page(&kernel_page_dir, (uintptr_t)entry_pgdir);
}

void main(void)
{
    if (mboot_magic != MULTIBOOT_BOOTLOADER_MAGIC
        || !(mboot_info.flags & MULTIBOOT_INFO_MEM_MAP)
        || !(mboot_info.flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO))
        return;

    // 1. initialise kernel water allocator
    kwmallocinit();

    uintptr_t initrd_phy_addr = *(uintptr_t*)(mboot_info.mods_addr + KERN_BASE);
    uintptr_t initrd_phy_end = *(uintptr_t*)(mboot_info.mods_addr + KERN_BASE + 4);

    // 2. initialise pmm
    pmm_init(mboot_info);
    for (uintptr_t phys = kernel_physical_start; phys < kernel_physical_end; phys += PAGE_SIZE)
        pmm_reserve_frame(phys);

    for (uintptr_t pa = initrd_phy_addr; pa < initrd_phy_end; pa += PAGE_SIZE)
        pmm_reserve_frame(pa);

    // 3. set up kernel pgdir and tables
    init_kernel_pgdir();

    uintptr_t initrd_addr = 0x1000;
    for (uintptr_t pa = initrd_phy_addr, va = initrd_addr; pa < initrd_phy_end; pa += PAGE_SIZE, va += PAGE_SIZE)
        map_page(&kernel_page_dir, pa, va, PTE_W);

    // 4. initialise kernel allocator
    //     kmallocinit();

    // 5. initialise screen
    uintptr_t fb_addr = 0xc03ff000;
    map_page(&kernel_page_dir, mboot_info.framebuffer_addr_low, fb_addr, PTE_W);
    init_screen(fb_addr, mboot_info.framebuffer_width, mboot_info.framebuffer_height);

    printf("\
Kernel      0x%x - 0x%x (Phy0x%x - Phy0x%x)\n\
    GDT         Desc=0x%x, Size=%d, Loc=0x%x\n\
    Stack       0x%x - 0x%x\n\
",
           kernel_start, kernel_end, kernel_physical_start, kernel_physical_end,
           (uintptr_t)&entry_gdt_desc, entry_gdt_desc.size, (uintptr_t)entry_gdt_desc.gdt,
           stack_bottom, stack_bottom + stack_size);

    fs_root = initialise_initrd(initrd_addr);

    printf("\nContents of initrd:\n");

    struct dirent* node = NULL;
    for (size_t i = 0; (node = read_dir_fs(fs_root, i)) != NULL; ++i) {
        printf("Found file ");
        printf("%s", node->name);
        fs_node_t* fsnode = find_dir_fs(fs_root, node->name);

        if (FS_TYPE(fsnode->flags) == FS_DIR)
            printf("\n    (directory)\n");
        else {
            printf("\n     contents:\n\n");
            char buf[1024];
            size_t sz = read_fs(fsnode, 0, 1024, buf);
            for (size_t j = 0; j < sz; ++j)
                printf("%c", buf[j]);
            printf("\nSize= %dB\n", sz);
        }
    }

    init_keyboard();
}
