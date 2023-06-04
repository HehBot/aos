#include <drivers/keyboard.h>
#include <drivers/screen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void __attribute__((cdecl)) main(uint32_t page_dir_start, uint32_t gdt_start, uint32_t screen_start, uint32_t screen_physical_start, uint32_t kernel_start, uint32_t kernel_end, uint32_t kernel_physical_start, uint32_t kernel_physical_end)
{
    init_screen(screen_start);
    init_keyboard();

    printf("\
Page dir at 0x%x\n\
GDT at      0x%x\n\
Screen at   0x%x\n\
    Physical addr   0x%x\n\
Kernel at   0x%x - 0x%x\n\
    Physical addr   0x%x - 0x%x\n\
",
           page_dir_start,
           gdt_start,
           screen_start,
           screen_physical_start,
           kernel_start, kernel_end,
           kernel_physical_start, kernel_physical_end);
}
