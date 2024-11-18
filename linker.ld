ENTRY(_start)

KERN_BASE = 0xffffffffc0000000;

SECTIONS
{
    . = 1M;

    .boot.rodata :
    {
        *(.multiboot2.header)
        *(.boot.rodata)
    }
    .boot.bss :
    {
        *(.boot.bss)
    }
    .boot.text :
    {
        *(.boot.text)
    }

    . += KERN_BASE;

    .bss : AT (ADDR (.bss) - KERN_BASE)
    {
        *(COMMON)
        *(.bss)
    }
    .text ALIGN (4K) : AT (ADDR (.text) - KERN_BASE)
    {
        *(.text)
    }
    .rodata : AT (ADDR (.rodata) - KERN_BASE)
    {
        *(.rodata)
    }
    .data ALIGN (4K) : AT (ADDR (.data) - KERN_BASE)
    {
        *(.data)
    }
}
