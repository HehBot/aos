#include "screen.h"

#include <cpu/port.h>
#include <cpu/x86.h>
#include <memory/frame_allocator.h>
#include <memory/paging.h>
#include <multiboot2.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PORT_SCREEN_CTRL 0x3d4
#define PORT_SCREEN_DATA 0x3d5

static uint8_t* vid_mem;

static size_t screen_rows;
static size_t screen_cols;

virt_addr_t init_screen(struct multiboot_tag_framebuffer const* fbinfo, virt_addr_t addr_to_use_for_mapping)
{
    struct multiboot_tag_framebuffer_common const* common = &fbinfo->common;

    size_t fb_size = (common->framebuffer_width * (common->framebuffer_height + 1) * (common->framebuffer_bpp >> 3));

    if (addr_to_use_for_mapping == NULL)
        vid_mem = (uint8_t*)common->framebuffer_addr;
    else {
        vid_mem = addr_to_use_for_mapping;
        for (size_t i = 0; i < fb_size; i += PAGE_SIZE, addr_to_use_for_mapping += PAGE_SIZE) {
            int err = frame_allocator_reserve_frame(common->framebuffer_addr + i);
            if (err != FRAME_ALLOCATOR_OK && err != FRAME_ALLOCATOR_ERROR_NO_SUCH_FRAME)
                PANIC("Unable to reserve framebuffer frames");
            err = map_to(addr_to_use_for_mapping, common->framebuffer_addr, PAGE_4KiB, PTE_W | PTE_P);
            if (err != PAGING_OK)
                PANIC("Unable to map framebuffer frames");
        }
    }

    screen_rows = common->framebuffer_height;
    screen_cols = common->framebuffer_width;

    return addr_to_use_for_mapping;
}

static inline size_t get_screen_offset(size_t cols, size_t rows)
{
    return cols + screen_cols * rows;
}
static inline size_t get_cursor()
{
    port_write_byte(PORT_SCREEN_CTRL, 14);
    size_t offset = port_read_byte(PORT_SCREEN_DATA);
    offset <<= 8;
    port_write_byte(PORT_SCREEN_CTRL, 15);
    offset |= port_read_byte(PORT_SCREEN_DATA);
    return offset;
}
static inline void set_cursor(size_t offset)
{
    port_write_byte(PORT_SCREEN_CTRL, 14);
    port_write_byte(PORT_SCREEN_DATA, (offset >> 8) & 0xff);
    port_write_byte(PORT_SCREEN_CTRL, 15);
    port_write_byte(PORT_SCREEN_DATA, offset & 0xff);
}

static inline size_t handle_scrolling(size_t offset)
{
    size_t shift = ((offset / screen_cols) + 1);
    if (shift <= screen_rows)
        return offset;
    shift -= screen_rows;
    shift *= screen_cols;

    memmove(vid_mem, vid_mem + 2 * shift, 2 * (screen_rows * screen_cols - shift));
    for (size_t i = screen_rows * screen_cols - shift; i < screen_rows * screen_cols; ++i)
        vid_mem[2 * i] = ' ';
    return offset - shift;
}

void print_char(char character, size_t col, size_t row, uint8_t fg_color, uint8_t bg_color)
{
    if (character == '\0')
        return;

    if (fg_color > 0xf || bg_color > 0xf) {
        fg_color = SCREEN_COLOR_BLACK;
        bg_color = SCREEN_COLOR_WHITE;
    }

    size_t offset;

    if (col < screen_cols && row < screen_rows)
        offset = get_screen_offset(col, row);
    else
        offset = get_cursor();

    if (character == '\n') {
        size_t rows = offset / screen_cols;
        offset = get_screen_offset(screen_cols - 1, rows);
    } else {
        vid_mem[2 * offset] = character;
        vid_mem[2 * offset + 1] = (fg_color << 4) | bg_color;
    }

    ++offset;
    offset = handle_scrolling(offset);
    set_cursor(offset);
}

void clear_screen()
{
    for (size_t row = 0; row < screen_rows; ++row)
        for (size_t col = 0; col < screen_cols; ++col)
            print_char(' ', col, row, SCREEN_COLOR_BLACK, SCREEN_COLOR_WHITE);
    set_cursor(get_screen_offset(0, 0));
}

void puts(char const* str)
{
    while (*str != '\0')
        print_char(*(str++), -1, -1, SCREEN_COLOR_BLACK, SCREEN_COLOR_WHITE);
}
void putc(char character)
{
    print_char(character, -1, -1, SCREEN_COLOR_BLACK, SCREEN_COLOR_WHITE);
}
