#include <drivers/keyboard.h>
#include <drivers/screen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void read_drive(uint32_t buf, uint8_t first_sect, uint8_t num_sects);

void __attribute__((cdecl)) main(uint32_t page_dir_virtual_start, uint32_t page_dir_physical_start, uint32_t screen_virtual_start, uint32_t screen_physical_start, uint32_t kernel_virtual_start, uint32_t kernel_virtual_end, uint32_t kernel_physical_start, uint32_t kernel_physical_end)
{
    init_screen(screen_virtual_start);
    init_keyboard();

    uint32_t* x = (uint32_t*)page_dir_virtual_start;
    x[0] = 0x2;

    printf("\
Page dir at\n\
    Physical addr   0x%x\n\
    Virtual addr    0x%x\n\
Screen at\n\
    Physical addr   0x%x\n\
    Virtual addr    0x%x\n\
Kernel at\n\
    Physical addr   0x%x - 0x%x\n\
    Virtual addr    0x%x - 0x%x\n\
",
           page_dir_physical_start, page_dir_virtual_start,
           screen_physical_start, screen_virtual_start,
           kernel_physical_start, kernel_physical_end, kernel_virtual_start, kernel_virtual_end);

    // enable access to the 4MB page statring at 0xffc00000
    uint32_t* z = (uint32_t*)page_dir_virtual_start;
    uint32_t addr = 0xffc00000;
    z[addr >> 22] = 0x00400083;
    uint32_t* y = (uint32_t*)0xfffff000;
    *y = 0xcafebabe;
    printf("0x%x\n", *y);
}
