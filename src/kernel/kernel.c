#include <cpu/interrupt.h>
#include <drivers/screen.h>

void main()
{
    isr_install();

    clear_screen();
    char const* str = "In kernel\n";
    puts(str);

    asm("int 0x0");
    asm("int 0x1");
    asm("int 0x2");
}
