#include "screen.h"

#include <cpu/port.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define VIDEO_ADDRESS 0xb8000

static uint8_t* const vid_mem = (uint8_t*)VIDEO_ADDRESS;

static size_t get_screen_offset(size_t cols, size_t rows)
{
    return cols + SCREEN_MAX_COLS * rows;
}
static size_t get_cursor()
{
    port_write_byte(0x3d4, 14);
    size_t offset = port_read_byte(0x3d5);
    offset <<= 8;
    port_write_byte(0x3d4, 15);
    offset |= port_read_byte(0x3d5);
    return offset;
}
static void set_cursor(size_t offset)
{
    port_write_byte(PORT_SCREEN_CTRL, 14);
    port_write_byte(PORT_SCREEN_DATA, (offset >> 8) & 0xff);
    port_write_byte(PORT_SCREEN_CTRL, 15);
    port_write_byte(PORT_SCREEN_DATA, offset & 0xff);
}

static size_t handle_scrolling(size_t offset)
{
    size_t shift = ((offset / SCREEN_MAX_COLS) + 1);
    if (shift <= SCREEN_MAX_ROWS)
        return offset;
    shift -= SCREEN_MAX_ROWS;
    shift *= SCREEN_MAX_COLS;

    memmove(vid_mem, vid_mem + 2 * shift, 2 * (SCREEN_MAX_ROWS * SCREEN_MAX_COLS - shift));
    for (size_t i = SCREEN_MAX_ROWS * SCREEN_MAX_COLS - 1 - shift; i < SCREEN_MAX_ROWS * SCREEN_MAX_COLS; ++i)
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

    if (col < SCREEN_MAX_COLS && row < SCREEN_MAX_ROWS)
        offset = get_screen_offset(col, row);
    else
        offset = get_cursor();

    if (character == '\n') {
        size_t rows = offset / SCREEN_MAX_COLS;
        offset = get_screen_offset(SCREEN_MAX_COLS - 1, rows);
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
    for (size_t row = 0; row < SCREEN_MAX_ROWS; ++row)
        for (size_t col = 0; col < SCREEN_MAX_COLS; ++col)
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
