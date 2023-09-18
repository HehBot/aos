#ifndef SCREEN_H
#define SCREEN_H

#include <stddef.h>
#include <stdint.h>

#define SCREEN_COLOR_BLACK 0
#define SCREEN_COLOR_BLUE 1
#define SCREEN_COLOR_GREEN 2
#define SCREEN_COLOR_CYAN 3
#define SCREEN_COLOR_RED 4
#define SCREEN_COLOR_MAGENTA 5
#define SCREEN_COLOR_BROWN 6
#define SCREEN_COLOR_LIGHTGREY 7
#define SCREEN_COLOR_DARKGREY 8
#define SCREEN_COLOR_LIGHTBLUE 9
#define SCREEN_COLOR_LIGHTGREEN 10
#define SCREEN_COLOR_LIGHTCYAN 11
#define SCREEN_COLOR_LIGHTRED 12
#define SCREEN_COLOR_LIGHTMAGENTA 13
#define SCREEN_COLOR_LIGHTBROWN 14
#define SCREEN_COLOR_WHITE 15

void init_screen(uintptr_t addr, size_t fbw, size_t fbh);
void print_char(char character, size_t col, size_t row, uint8_t fg_color, uint8_t bg_color);
void clear_screen();

void puts(char const* str);
void putc(char character);

#endif // SCREEN_H
