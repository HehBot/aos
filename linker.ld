ENTRY(_start)

SECTIONS
{
    . = 1M;

    _kernel_start = .;

    .multiboot.data :
    {
        *(.multiboot.data)
    }
    .multiboot.text :
    {
        *(.multiboot.text)
    }

    kernel_physical_start = .;
    . += 0xC0000000;
    kernel_start = .;
    screen_start = 0xC03FF000;

    .text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000)
    {
        *(.text)
    }
    .rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000)
    {
        *(.rodata)
    }
    .data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000)
    {
        *(.data)
    }
    .bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
    }
    /* Add a symbol that indicates the end address of the kernel. */
    kernel_end = .;
    kernel_physical_end = . - 0xC0000000;
}
