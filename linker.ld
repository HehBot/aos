ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot.data :
    {
        *(.multiboot.data)
    }
    .multiboot.text :
    {
        *(.multiboot.text)
    }

    _kernel_physical_start = .;
    . += 0xC0000000;
    _kernel_start = .;

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
    _kernel_end = .;
    _kernel_physical_end = . - 0xC0000000;
}
