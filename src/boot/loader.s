MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
MBFLAGS  equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + MBFLAGS)

[section .multiboot]
align 4
dd MAGIC
dd MBFLAGS
dd CHECKSUM

[section .data]
%include "gdt.s"

[section .bss]
align 16
stack_bottom:
resb 16384
stack_top:

[section .text]
[global _start:function (_start.end - _start)]
_start:
        ; load GDT
        cli
        lgdt    [gdt_descriptor]
        sti
        mov     ax, DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax
        jmp     CODE_SEG:.fix_cs
.fix_cs:
        mov     ebp, stack_top
        mov     esp, ebp

        [extern isr_install]
        call    isr_install

        [extern main]
        [extern kernel_physical_end]
        [extern kernel_physical_start]
        [extern kernel_virtual_end]
        [extern kernel_virtual_start]
        [extern screen_physical_start]
        [extern screen_virtual_start]
        push    kernel_physical_end
        push    kernel_physical_start
        push    kernel_virtual_end
        push    kernel_virtual_start
        push    screen_physical_start
        push    screen_virtual_start
        push    0xcafebabe
        push    0xdeadbeef
        call    main
.hang:
        jmp     .hang
.end:
