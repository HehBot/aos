#ifndef SCREEN_H
#define SCREEN_H

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

void print_char(char character, int col, int row, unsigned char fg_color, unsigned char bg_color);
void clear_screen();

void puts(char const* str);
void putc(char character);

#endif // SCREEN_H
