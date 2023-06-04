ENTRY(_start)

SECTIONS
{
    . = 1M;

    kernel_physical_start = .;
    kernel_virtual_start = .;
    screen_physical_start = 0xB8000;
    screen_virtual_start = 0xB8000;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    kernel_physical_end = .;
    kernel_virtual_end = .;
}
