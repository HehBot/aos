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
#include <multiboot2.h>
#include <proc/proc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern mem_map_t kernel_mem_map[];

size_t block_size;

fs_node_t* root;

static void map_modules(mem_map_t* map, size_t nr_mods)
{
    for (size_t i = 0; i < nr_mods; ++i) {
        uintptr_t pa = map[i].phy_start;
        size_t nr_pages = PG_ROUND_UP(map[i].phy_end - pa) / PAGE_SIZE;
        uintptr_t va = (uintptr_t)kpalloc(nr_pages);
        map[i].virt = va;
        map[i].nr_pages = nr_pages;
        for (size_t i = 0; i < nr_pages; ++i, pa += PAGE_SIZE, va += PAGE_SIZE)
            remap_page(pa, va, PTE_W);
    }
}

#define ROUND_UP_8(x) ((((x) + 7) >> 3) << 3)

void __attribute__((cdecl)) main(uint32_t mboot_magic, uintptr_t __mboot_info)
{
    if (mboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC)
        return;

#define MAX_MODS 10
    mem_map_t mod_mem_map[MAX_MODS];
    size_t nr_mods = 0;

    struct multiboot_tag_framebuffer fbinfo;

    uint8_t* p = ((uint8_t*)__mboot_info) + 8;
    int done = 0;
    while (!done) {
        struct multiboot_tag* tag = (struct multiboot_tag*)p;
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_END:
            done = 1;
            break;
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            memcpy(&fbinfo, (struct multiboot_tag_framebuffer*)tag, sizeof(struct multiboot_tag_framebuffer));
            goto end;
        case MULTIBOOT_TAG_TYPE_MMAP: {
            struct multiboot_tag_mmap* tag_mmap = (struct multiboot_tag_mmap*)tag;
            size_t nr_entries = (tag_mmap->size - sizeof(*tag_mmap)) / sizeof(tag_mmap->entries[0]);
            init_pmm(&tag_mmap->entries[0], nr_entries);
        }
            goto end;
        case MULTIBOOT_TAG_TYPE_ACPI_NEW:
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
        case MULTIBOOT_TAG_TYPE_CMDLINE:
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
        case MULTIBOOT_TAG_TYPE_BOOTDEV:
        case MULTIBOOT_TAG_TYPE_VBE:
        case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
        case MULTIBOOT_TAG_TYPE_APM:
        case MULTIBOOT_TAG_TYPE_EFI32:
        case MULTIBOOT_TAG_TYPE_EFI64:
        case MULTIBOOT_TAG_TYPE_SMBIOS:
        case MULTIBOOT_TAG_TYPE_ACPI_OLD:
        case MULTIBOOT_TAG_TYPE_NETWORK:
        case MULTIBOOT_TAG_TYPE_EFI_MMAP:
        case MULTIBOOT_TAG_TYPE_EFI_BS:
        case MULTIBOOT_TAG_TYPE_EFI32_IH:
        case MULTIBOOT_TAG_TYPE_EFI64_IH:
        case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
        end:
            p += ROUND_UP_8(tag->size);
            continue;
        }
    }

    for (size_t i = 0; kernel_mem_map[i].present; ++i)
        if (kernel_mem_map[i].mapped)
            for (uintptr_t phy = kernel_mem_map[i].phy_start; phy < kernel_mem_map[i].phy_end; phy += PAGE_SIZE)
                pmm_reserve_frame(phy);
    for (size_t i = 0; i < nr_mods; ++i)
        for (uintptr_t phy = mod_mem_map[i].phy_start; phy < mod_mem_map[i].phy_end; phy += PAGE_SIZE)
            pmm_reserve_frame(phy);

    // 2. initialise mm, kernel heap, map modules
    init_mm();
    map_modules(mod_mem_map, nr_mods);

    // 3. initialise screen
    init_screen(&fbinfo);

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
