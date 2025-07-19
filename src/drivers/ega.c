#include "ega.h"

#include <cpu/port.h>
#include <cpu/spinlock.h>
#include <cpu/x86.h>
#include <memory/frame_allocator.h>
#include <memory/paging.h>
#include <multiboot2.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PORT_EGA_CTRL 0x3d4
#define PORT_EGA_DATA 0x3d5

static spinlock_t lock = {};
static volatile int locking = 0;
static uint8_t* vid_mem;
static size_t screen_rows;
static size_t screen_cols;

void init_ega(struct multiboot_tag_framebuffer const* fbinfo, virt_addr_t* mapping_addr_ptr)
{
    struct multiboot_tag_framebuffer_common const* common = &fbinfo->common;

    size_t fb_size = (common->framebuffer_width * (common->framebuffer_height + 1) * (common->framebuffer_bpp >> 3));

    if (mapping_addr_ptr == NULL)
        vid_mem = (uint8_t*)common->framebuffer_addr;
    else {
        vid_mem = *mapping_addr_ptr;
        virt_addr_t addr = *mapping_addr_ptr;
        phys_addr_t first_frame = PAGE_ROUND_DOWN(common->framebuffer_addr);
        phys_addr_t last_frame = PAGE_ROUND_DOWN(common->framebuffer_addr + fb_size - 1);
        for (phys_addr_t frame = first_frame; frame <= last_frame; frame += PAGE_SIZE, addr += PAGE_SIZE) {
            int err = frame_allocator_reserve_frame(frame);
            ASSERT(err == FRAME_ALLOCATOR_ERROR_NO_SUCH_FRAME);
            err = paging_kernel_map(addr, frame, PAGE_4KiB, PTE_NX | PTE_W | PTE_P);
            ASSERT(err == PAGING_OK);
        }
        *mapping_addr_ptr = addr;
    }

    screen_rows = common->framebuffer_height;
    screen_cols = common->framebuffer_width;
}

void ega_enable_lock()
{
    locking = 1;
}

static inline size_t get_screen_offset(size_t cols, size_t rows)
{
    return cols + screen_cols * rows;
}
static inline size_t get_cursor()
{
    port_write_byte(PORT_EGA_CTRL, 14);
    size_t offset = port_read_byte(PORT_EGA_DATA);
    offset <<= 8;
    port_write_byte(PORT_EGA_CTRL, 15);
    offset |= port_read_byte(PORT_EGA_DATA);
    return offset;
}
static inline void set_cursor(size_t offset)
{
    port_write_byte(PORT_EGA_CTRL, 14);
    port_write_byte(PORT_EGA_DATA, (offset >> 8) & 0xff);
    port_write_byte(PORT_EGA_CTRL, 15);
    port_write_byte(PORT_EGA_DATA, offset & 0xff);
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

static void ega_putc_spl_unlocked(char character, size_t col, size_t row, uint8_t fg_color, uint8_t bg_color, int move_cursor)
{
    if (character == '\0')
        return;

    if (fg_color > 0xf || bg_color > 0xf) {
        fg_color = EGA_COLOR_BLACK;
        bg_color = EGA_COLOR_WHITE;
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
    if (move_cursor)
        set_cursor(offset);
}

void ega_putc_spl(char character, size_t col, size_t row, uint8_t fg_color, uint8_t bg_color)
{
    if (locking)
        acquire(&lock);
    ega_putc_spl_unlocked(character, col, row, fg_color, bg_color, 1);
    if (locking)
        release(&lock);
}

void ega_clear()
{
    if (locking)
        acquire(&lock);
    for (size_t row = 0; row < screen_rows; ++row)
        for (size_t col = 0; col < screen_cols; ++col)
            ega_putc_spl_unlocked(' ', col, row, EGA_COLOR_BLACK, EGA_COLOR_WHITE, 0);
    set_cursor(get_screen_offset(0, 0));
    if (locking)
        release(&lock);
}

void ega_puts(char const* str)
{
    if (locking)
        acquire(&lock);
    while (*str != '\0')
        ega_putc_spl_unlocked(*(str++), -1, -1, EGA_COLOR_BLACK, EGA_COLOR_WHITE, 1);
    if (locking)
        release(&lock);
}
void ega_putc(char character)
{
    ega_putc_spl(character, -1, -1, EGA_COLOR_BLACK, EGA_COLOR_WHITE);
}

void __attribute__((format(printf, 1, 2))) panic(char const* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    locking = 0;
    disable_interrupts();

    vprintf(fmt, ap);

    hlt();

    __builtin_unreachable();
}
