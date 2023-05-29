#include "screen.h"

#include <cpu/port.h>
#include <string.h>

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80

static unsigned char* const vid_mem = (unsigned char*)VIDEO_ADDRESS;

static int get_screen_offset(int cols, int rows)
{
    return cols + MAX_COLS * rows;
}
static int get_cursor()
{
    port_write_byte(0x3d4, 14);
    int offset = port_read_byte(0x3d5);
    offset <<= 8;
    port_write_byte(0x3d4, 15);
    offset |= port_read_byte(0x3d5);
    return offset;
}
static void set_cursor(int offset)
{
    port_write_byte(0x3d4, 14);
    port_write_byte(0x3d5, (offset >> 8) & 0xff);
    port_write_byte(0x3d4, 15);
    port_write_byte(0x3d5, offset & 0xff);
}

static int handle_scrolling(int offset)
{
    int shift = ((offset / MAX_COLS) - MAX_ROWS + 1) * MAX_COLS;
    if (shift <= 0)
        return offset;

    memmove(vid_mem, vid_mem + 2 * shift, 2 * (MAX_ROWS * MAX_COLS - shift));
    for (int i = MAX_ROWS * MAX_COLS - 1 - shift; i < MAX_ROWS * MAX_COLS; ++i)
        vid_mem[2 * i] = ' ';
    return offset - shift;
}

void print_char(char character, int col, int row, unsigned char fg_color, unsigned char bg_color)
{
    if (fg_color > 0xf || bg_color > 0xf) {
        fg_color = SCREEN_COLOR_BLACK;
        bg_color = SCREEN_COLOR_WHITE;
    }

    int offset;

    if (col >= 0 && row >= 0)
        offset = get_screen_offset(col, row);
    else
        offset = get_cursor();

    if (character == '\n') {
        int rows = offset / MAX_COLS;
        offset = get_screen_offset(MAX_COLS - 1, rows);
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
    for (int row = 0; row < MAX_ROWS; ++row)
        for (int col = 0; col < MAX_COLS; ++col)
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
