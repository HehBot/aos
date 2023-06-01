ENTRY(_start)

SECTIONS {
    . = 0xC0000000;

    screen_virtual_start = 0xC0009000;
    screen_physical_start = 0xB8000;
    page_dir_virtual_start = 0xC0008000;
    page_dir_physical_start = 0x8000;

    kernel_virtual_start = .;
    kernel_physical_start = . - 0xC0000000;

    /* align at 4 KB and load at 1 MB */
    .text ALIGN (0x1000) : AT(ADDR(.text)-0xC0000000)
    {
        *(.text)
    }
    /* align at 4 KB and load at 1 MB + . */
    .rodata ALIGN (0x1000) : AT(ADDR(.rodata)-0xC0000000)
    {
        *(.rodata*)
    }

    /* align at 4 KB and load at 1 MB + . */
    .data ALIGN (0x1000) : AT(ADDR(.data)-0xC0000000)
    {
        *(.data)
    }
    /* align at 4 KB and load at 1 MB + . */
    .bss ALIGN (0x1000) : AT(ADDR(.bss)-0xC0000000)
    {
        *(COMMON)
        *(.bss)
    }
    kernel_virtual_end = .;
    kernel_physical_end = . - 0xC0000000;
}
