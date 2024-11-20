#ifndef EGA_H
#define EGA_H

#include <memory/page.h>
#include <stddef.h>
#include <stdint.h>

#define EGA_COLOR_BLACK 0
#define EGA_COLOR_BLUE 1
#define EGA_COLOR_GREEN 2
#define EGA_COLOR_CYAN 3
#define EGA_COLOR_RED 4
#define EGA_COLOR_MAGENTA 5
#define EGA_COLOR_BROWN 6
#define EGA_COLOR_LIGHTGREY 7
#define EGA_COLOR_DARKGREY 8
#define EGA_COLOR_LIGHTBLUE 9
#define EGA_COLOR_LIGHTGREEN 10
#define EGA_COLOR_LIGHTCYAN 11
#define EGA_COLOR_LIGHTRED 12
#define EGA_COLOR_LIGHTMAGENTA 13
#define EGA_COLOR_LIGHTBROWN 14
#define EGA_COLOR_WHITE 15

struct multiboot_tag_framebuffer;

void init_ega(struct multiboot_tag_framebuffer const* fbinfo, virt_addr_t* mapping_addr_ptr);
void ega_putc_spl(char character, size_t col, size_t row, uint8_t fg_color, uint8_t bg_color);
void ega_clear();
void ega_enable_lock();

void ega_puts(char const* str);
void ega_putc(char character);

#endif // EGA_H
