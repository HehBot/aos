#include "mm.h"
#include "multiboot.h"

#include <drivers/keyboard.h>
#include <drivers/screen.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void __attribute__((cdecl)) main(multiboot_info_t* mbi, uint32_t mboot_magic, uint32_t* page_dir_start, uint32_t* temp_page_table_start, uint32_t* gdt_start, uintptr_t stack_bottom, uint32_t stack_size, uintptr_t kernel_start, uintptr_t kernel_end, uintptr_t kernel_physical_start, uintptr_t kernel_physical_end)
{
    if (mboot_magic != MULTIBOOT_BOOTLOADER_MAGIC)
        return;

    multiboot_info_t mboot_info;
    memcpy(&mboot_info, mbi, sizeof(mboot_info));

    if (!((mboot_info.flags >> 6) & 1) || !((mboot_info.flags >> 12) & 1))
        return;

    mm_init(page_dir_start, temp_page_table_start);
    // tell memory manager about availabe regions of RAM
    for (size_t i = 0; i < mboot_info.mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mboot_info.mmap_addr + 0xc0000000 + i);
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
            mm_add_physical(mmmt->addr_low, mmmt->len_low);
    }
    for (uintptr_t i = kernel_physical_start, j = kernel_start; i < kernel_physical_end && j < kernel_end; i += 4096, j += 4096) {
        mm_reserve_physical_page(i);
        mm_reserve_page(j);
    }

    // get rid of temporary pages for multiboot
    for (uintptr_t i = 0xc0000; i < kernel_start >> 12; ++i)
        temp_page_table_start[i & 0x3ff] = 0x0;

    uintptr_t fb_addr = 0xc03ff000;
    temp_page_table_start[(fb_addr >> 12) & 0x3ff] = mboot_info.framebuffer_addr_low | 0x3;
    size_t fb_size = (mboot_info.framebuffer_bpp >> 3) * mboot_info.framebuffer_width * mboot_info.framebuffer_height;

    init_screen(fb_addr /*, mboot_info->framebuffer_type*/);

    printf("\
Kernel      0x%x - 0x%x (Phy0x%x - Phy0x%x)\n\
    Page dir    0x%x\n\
    Tmp Pg Tbl  0x%x\n\
    GDT         0x%x\n\
    Stack       0x%x - 0x%x\n\
FB          0x%x - 0x%x (Phy0x%x - Phy0x%x)\n\
",
           kernel_start, kernel_end, kernel_physical_start, kernel_physical_end,
           page_dir_start,
           temp_page_table_start,
           gdt_start,
           stack_bottom, stack_bottom + stack_size,
           fb_addr, fb_addr + fb_size, mboot_info.framebuffer_addr_low, mboot_info.framebuffer_addr_low + fb_size);

    init_keyboard();
}
