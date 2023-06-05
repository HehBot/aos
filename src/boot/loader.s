MULTIBOOT_PAGE_ALIGN    equ     1 << 0
MULTIBOOT_MEMORY_INFO   equ     1 << 1
MULTIBOOT_USE_GFX       equ     1 << 2
MULTIBOOT_HEADER_MAGIC  equ     0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ     MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_USE_GFX
MULTIBOOT_CHECKSUM      equ     -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

[section .multiboot.data]
align 4
dd MULTIBOOT_HEADER_MAGIC
dd MULTIBOOT_HEADER_FLAGS
dd MULTIBOOT_CHECKSUM
dd 0x0  ; header_addr
dd 0x0  ; load_addr
dd 0x0  ; load_end_addr
dd 0x0  ; bss_end_addr
dd 0x0  ; entry_addr
; GFX requests
dd 0x1
dd 0x0
dd 0x0
dd 0x0

align 8
temp_gdt:
%include "gdt.s"
TEMP_CODE_SEG equ temp_gdt.code_seg - temp_gdt.start
TEMP_DATA_SEG equ temp_gdt.data_seg - temp_gdt.start

[section .bss]
alignb 16
STACK_SIZE equ 0x4000
stack_bottom:
resb STACK_SIZE
stack_top:

mboot:
resb 120
mboot_magic:
resb 4

alignb 4096
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
        cli

        ; load GDT
        lgdt    [temp_gdt.descriptor]
        ; fix segments
        mov     cx, TEMP_DATA_SEG
        mov     ds, cx
        mov     es, cx
        mov     fs, cx
        mov     gs, cx
        jmp     TEMP_CODE_SEG:.fix_cs
.fix_cs:

        ; store mboot header
        mov     dword [mboot_magic-0xc0000000], eax
        mov     edi, mboot-0xc0000000
        mov     esi, ebx
        mov     ecx, 0x78
        rep     movsb

        ; enable paging
        mov     edi, temp_page_table-0xc0000000
        mov     esi, 0
        mov     ecx, 1023
.1:
;         cmp     esi, _kernel_start
        cmp     esi, 0
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
        lgdt    [gdt.descriptor]
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

        ; install interrupt handlers and enable interrupts
        [extern isr_install]
        call    isr_install
        sti

        [extern main]
        [extern kernel_physical_end]
        [extern kernel_physical_start]
        [extern kernel_end]
        [extern kernel_start]
times 1 push    0x00000000
        push    kernel_physical_end
        push    kernel_physical_start
        push    kernel_end
        push    kernel_start
        push    STACK_SIZE
        push    stack_bottom
        push    gdt.descriptor
        push    temp_page_table
        push    page_dir
        push    dword [mboot_magic]
        push    mboot
        call    main
        jmp     $
.end:
