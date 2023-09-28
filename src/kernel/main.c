#include "kpalloc.h"
#include "kwmalloc.h"
#include "mm.h"
#include "pmm.h"

#include <cpu/interrupt.h>
#include <cpu/x86.h>
#include <drivers/ata.h>
#include <drivers/keyboard.h>
#include <drivers/screen.h>
#include <fs/fs.h>
#include <fs/initrd.h>
#include <liballoc.h>
#include <multiboot.h>
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

    // 2. initialise mm
    init_mm();

    // 3. set up kernel heap
    init_kpalloc();

    // 4. initialise screen
    init_screen(&mboot_info);

    // 5. read initrd
    for (uintptr_t virt = 0x1000, phy = mod_mem_map[0].phy_start; phy < mod_mem_map[0].phy_end; virt += PAGE_SIZE, phy += PAGE_SIZE)
        map_page(phy, virt, PTE_W);
    fs_root = initialise_initrd(0x1000);
    printf("Contents of initrd:\n");
    struct dirent* node = NULL;
    for (size_t i = 0; (node = read_dir_fs(fs_root, i)) != NULL; ++i) {
        printf("Found file \"%s\"\n", node->name);
        fs_node_t* fsnode = find_dir_fs(fs_root, node->name);

        if (FS_TYPE(fsnode->flags) == FS_DIR)
            printf("    (directory)\n");
        else {
            printf("     contents:\n");
            char buf[256];
            size_t sz = read_fs(fsnode, 0, 256, buf);
            for (size_t j = 0; j < sz; ++j)
                printf("%c", buf[j]);
            printf("\n     total %d bytes\n\n", sz);
        }
    }

    // 6. read fs
    ata_init();
    void* buf = kmalloc(512);
    volatile int flag;
    ata_req(1, 0, buf, &flag);
    while (!flag)
        ;
    int (*user_func)(void) = buf;
    printf("Loaded function returned 0x%x\n", user_func());
    kfree(buf);

    test_kpalloc();
    void* z = kmalloc(20);
    printf("0x%x\n", z);
    kfree(z);

    init_keyboard();
}
