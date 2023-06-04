MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
MBFLAGS  equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + MBFLAGS)

[section .multiboot.data]
align 4
dd MAGIC
dd MBFLAGS
dd CHECKSUM

align 8
temp_gdt:
%include "gdt.s"
TEMP_CODE_SEG equ temp_gdt.code_seg - temp_gdt.start
TEMP_DATA_SEG equ temp_gdt.data_seg - temp_gdt.start

[section .bss]
align 16
stack_bottom:
resb 16384
stack_top:

align 4096
page_dir:
resb 4096
temp_page_table:
resb 4096

[section .data]
align 8
gdt:
%include "gdt.s"
CODE_SEG equ gdt.code_seg - gdt.start
DATA_SEG equ gdt.data_seg - gdt.start

[section .multiboot.text]
[global _start]
[extern _kernel_start]
[extern kernel_end]
_start:
        ; load GDT
        cli
        lgdt    [temp_gdt.descriptor]
        sti
        ; fix segments
        mov     ax, TEMP_DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        jmp     TEMP_CODE_SEG:.fix_cs
.fix_cs:

        ; enable paging
        mov     edi, temp_page_table-0xc0000000
        mov     esi, 0
        mov     ecx, 1023
.1:
        cmp     esi, _kernel_start
        jl      .2
        cmp     esi, kernel_end - 0xc0000000
        jge     .3

        mov     edx, esi
        or      edx, 0x003
        mov     dword [edi], edx
.2:
        add     esi, 4096
        add     edi, 4
        loop    .1
.3:
        mov     dword [temp_page_table-0xc0000000+1023*4], 0x000b8003

        mov     dword [page_dir-0xc0000000+0], temp_page_table-0xc0000000+0x003
        mov     dword [page_dir-0xc0000000+768*4], temp_page_table-0xc0000000+0x003

        ; set up paging
        mov     eax, page_dir-0xc0000000
        mov     cr3, eax
        mov     ebx, cr4
        or      ebx, 0x00000010
        mov     cr4, ebx
        mov     ebx, cr0
        or      ebx, 0x80010000
        mov     cr0, ebx

        jmp     fix_eip

[section .text]
fix_eip:
        ; reload GDT
        cli
        lgdt    [gdt.descriptor]
        sti
        ; fix segments
        mov     ax, DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        jmp     CODE_SEG:.fix_cs
.fix_cs:
        mov     ebp, stack_top
        mov     ss, ax
        mov     esp, ebp

        mov     dword [page_dir+0], 0x00000000
        mov     eax, cr3
        mov     cr3, eax

        [extern isr_install]
        call    isr_install

        [extern main]
        [extern kernel_physical_end]
        [extern kernel_physical_start]
        [extern kernel_end]
        [extern kernel_start]
        [extern screen_physical_start]
        [extern screen_start]
        push    kernel_physical_end
        push    kernel_physical_start
        push    kernel_end
        push    kernel_start
        push    screen_physical_start
        push    screen_start
        push    gdt.descriptor
        push    page_dir
        call    main
        jmp     $
.end:
