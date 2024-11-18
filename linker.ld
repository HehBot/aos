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

    .text ALIGN (4K) : AT (ADDR (.text) - KERN_BASE)
    {
        *(.text .text.*)
    }
    .rodata ALIGN (4K) : AT (ADDR (.rodata) - KERN_BASE)
    {
        *(.rodata .rodata.*)
    }
    .bss ALIGN (4K) : AT (ADDR (.bss) - KERN_BASE)
    {
        *(.bss .bss.*)
    }
    .data ALIGN (4K) : AT (ADDR (.data) - KERN_BASE)
    {
        *(.data .data.*)
    }
}
