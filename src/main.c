#include "ext2.h"

#include <acpi.h>
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
#include <multiboot2.h>
#include <proc/proc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

size_t block_size;
static fs_node_t* root;

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
        uintptr_t va = (uintptr_t)kpalloc(nr_pages);
        mod_mem_map[i].virt = va;
        mod_mem_map[i].nr_pages = nr_pages;
        for (size_t i = 0; i < nr_pages; ++i, pa += PAGE_SIZE, va += PAGE_SIZE)
            remap_page(pa, va, PTE_W);
    }
}

void __attribute__((cdecl)) main(uint32_t mboot_magic, uintptr_t __mboot_info)
{
    if (mboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC)
        return;
    // 0. parse __mboot_info and initialise pmm
    parse_mboot_info(__mboot_info);

    // 1. initialise pmm and reserve kernel and module frames
    // TODO relaim MULTIBOOT_MEMORY_ACPI_RECLAIMABLE entries
    init_pmm(mmap_info);
    for (size_t i = 0; kernel_mem_map[i].present; ++i)
        if (kernel_mem_map[i].mapped)
            for (uintptr_t phy = kernel_mem_map[i].phy_start; phy < kernel_mem_map[i].phy_end; phy += PAGE_SIZE)
                pmm_reserve_frame(phy);
    for (size_t i = 0; i < nr_mods; ++i)
        for (uintptr_t phy = mod_mem_map[i].phy_start; phy < mod_mem_map[i].phy_end; phy += PAGE_SIZE)
            pmm_reserve_frame(phy);

    // 2. initialise mm, kernel heap; map modules
    init_mm();
    map_modules(mod_mem_map, nr_mods);

    // 3. initialise screen
    init_screen(fb_info);

    // 4. initialise acpi and detect other processors
    init_acpi(rsdp);

    // 5. initialise other processors

    // initialise ata and disk cache
    size_t fsdisk = ata_init();

    // mount initrd to '/'
    root = read_initrd(mod_mem_map[0].virt);

    // muck around with ext2
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
