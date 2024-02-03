ENTRY(_start)

SECTIONS
{
    . = 1M;

    load_addr = .;

    .multiboot.data :
    {
        *(.multiboot.data)
    }
    .multiboot.text :
    {
        *(.multiboot.text)
    }
    . += 0xC0000000;
    .text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000)
    {
        kernel_code_start = .;
        kernel_phy_code_start = . - 0xC0000000;
        *(.text)
    }
    .rodata : AT (ADDR (.rodata) - 0xC0000000)
    {
        *(.rodata)
        kernel_phy_code_end = . - 0xC0000000;
    }

    .data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000)
    {
        kernel_data_start = .;
        kernel_phy_data_start = . - 0xC0000000;
        *(.data)
    }

    load_end_addr = . - 0xC0000000;

    .bss : AT (ADDR (.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
        kernel_phy_data_end = . - 0xC0000000;
    }

    bss_end_addr = . - 0xC0000000;

    . = ALIGN(4K);
    kernel_heap_start = .;
}
