#include <drivers/screen.h>

void main()
{
    clear_screen();
    char const* str = "In kernel\n";
    puts(str);
}
