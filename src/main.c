#include "ext2.h"

#include <cpu/x86.h>
#include <drivers/disk/ata.h>
#include <drivers/disk/block.h>
#include <drivers/keyboard.h>
#include <drivers/screen.h>
#include <drivers/timer.h>
#include <fs/fs.h>
#include <fs/initrd.h>
#include <hash_table.h>
#include <liballoc.h>
#include <mem/kpalloc.h>
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

size_t block_size;

fs_node_t* root;

static mem_map_t* read_module_map(multiboot_module_t* mods, size_t count)
{
    mem_map_t* map = kwmalloc((count + 1) * (sizeof map[0]));
    for (size_t i = 0; i < count; ++i) {
        mem_map_t m = { 1, 0, 0, 0, { { mods[i].mod_start, mods[i].mod_end } } };
        map[i] = m;
    }
    map[mboot_info.mods_count].present = 0;
    return map;
}
static void map_modules(mem_map_t* map)
{
    for (size_t i = 0; map[i].present; ++i) {
        uintptr_t pa = map[i].phy_start;
        size_t nr_pages = PG_ROUND_UP(map[i].phy_end - pa) / PAGE_SIZE;
        uintptr_t va = (uintptr_t)kpalloc(nr_pages);
        map[i].virt = va;
        map[i].nr_pages = nr_pages;
        for (size_t i = 0; i < nr_pages; ++i, pa += PAGE_SIZE, va += PAGE_SIZE)
            remap_page(pa, va, PTE_W);
    }
}

void main(void)
{
    if (mboot_magic != MULTIBOOT_BOOTLOADER_MAGIC
        || !(mboot_info.flags & MULTIBOOT_INFO_MEM_MAP)
        || !(mboot_info.flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO))
        return;

    // 0. read module mappings
    mem_map_t* mod_mem_map = read_module_map((multiboot_module_t*)(mboot_info.mods_addr + KERN_BASE), mboot_info.mods_count);

    // 1. initialise pmm
    init_pmm(&mboot_info);
    for (size_t i = 0; kernel_mem_map[i].present; ++i)
        if (kernel_mem_map[i].mapped)
            for (uintptr_t phy = kernel_mem_map[i].phy_start; phy < kernel_mem_map[i].phy_end; phy += PAGE_SIZE)
                pmm_reserve_frame(phy);
    for (size_t i = 0; mod_mem_map[i].present; ++i)
        for (uintptr_t phy = mod_mem_map[i].phy_start; phy < mod_mem_map[i].phy_end; phy += PAGE_SIZE)
            pmm_reserve_frame(phy);

    // 2. initialise mm, kernel heap, map modules
    init_mm();
    map_modules(mod_mem_map);

    // 3. initialise screen
    init_screen(&mboot_info);

    // 4. initialise ata and disk cache
    size_t fsdisk = ata_init();

    // 5. mount initrd to '/'
    root = read_initrd(mod_mem_map[0].virt);

    // 6. muck around with ext2
    block_size = 1024; // superblock is of length 1024 at offset 1024 bytes

    block_t* b = alloc_block();
    b->dev = fsdisk;
    b->block = 1;
    ata_sync(b);
    superblock_t* sup = (void*)b->data;
    b->data = NULL;
    free_block(b);
    block_size = (1 << (sup->log_block_sz + 10));
    size_t inode_size = sup->inode_sz;

    size_t nr_block_grp = (sup->nr_inodes + sup->inodes_per_block_grp - 1) / sup->inodes_per_block_grp;
    if (nr_block_grp != (sup->nr_blocks + sup->blocks_per_block_grp - 1) / sup->blocks_per_block_grp) {
        printf("Block group count mismatch\n");
        return;
    }

    b = alloc_block();
    b->dev = fsdisk;
    b->block = (block_size > 1024 ? 1 : 2);
    ata_sync(b);
    block_group_desc_t* block_grp_desc_table = (void*)b->data;
    b->data = NULL;
    free_block(b);

    dir_ent_t d = { 0 };
    printf("%d\n", d.inode);

    size_t root_dir_inode = 2;
    {
        size_t block_grp = (root_dir_inode - 1) / sup->inodes_per_block_grp;
        size_t index = (root_dir_inode - 1) % sup->inodes_per_block_grp;
        size_t inode_table_block = block_grp_desc_table[block_grp].inode_table_block;

        size_t containing_block = inode_table_block + (index * inode_size / block_size);
        size_t off = ((index * inode_size) % block_size) / inode_size;

        b = alloc_block();
        b->dev = fsdisk;
        b->block = containing_block;
        ata_sync(b);
        inode_t root_inode = *(inode_t*)((uintptr_t)b->data + off * inode_size);
        free_block(b);

        b = alloc_block();
        b->dev = fsdisk;
        b->block = root_inode.dbp[0];
        ata_sync(b);
    }

    printf("ext2 sign: '0x%x'\n", sup->ext2_sign);

    // 7. initialise proc
    init_proc();
}
