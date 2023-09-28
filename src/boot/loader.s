MULTIBOOT_PAGE_ALIGN    equ     1 << 0
MULTIBOOT_MEMORY_INFO   equ     1 << 1
MULTIBOOT_VIDEO_MODE    equ     1 << 2
MULTIBOOT_HEADER_MAGIC  equ     0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ     MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE
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
dd 0x1  ; mode_type
dd 0x0  ; width
dd 0x0  ; height
dd 0x0  ; depth (bpp)

[section .bss]
alignb 16
STACK_SIZE equ 0x1000
canary:
resb 16
stack_top:
resb STACK_SIZE
stack:

KERN_BASE               equ     0xc0000000

[section .multiboot.text]
[global _start]
[extern entry_pgdir]
[extern mboot_magic]
[extern mboot_info]
_start:
        cli

        ; store mboot header
        mov     dword [mboot_magic-KERN_BASE], eax
        mov     edi, mboot_info-KERN_BASE
        mov     esi, ebx
        mov     ecx, 0x78
        rep     movsb

        ; enable paging
        mov     eax, entry_pgdir-KERN_BASE
        mov     cr3, eax
        mov     ebx, cr4
        or      ebx, 0x00000010
        mov     cr4, ebx
        mov     ebx, cr0
        or      ebx, 0x80010000
        mov     cr0, ebx

        jmp     start

[section .text]
[extern entry_gdt_desc]
CODE_SEG equ 0x8
DATA_SEG equ 0x10
start:
        ; load GDT
        lgdt    [entry_gdt_desc]
        ; fix segments
        jmp     CODE_SEG:start.fix_cs
.fix_cs:
        mov     ax, DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        ; set up stack
        mov     ebp, stack
        mov     ss, ax
        mov     esp, ebp

        ; remove identity map entry
        mov     dword [entry_pgdir+0], 0x00000000

        ; install interrupt handlers and enable interrupts
        [extern isr_install]
        call    isr_install
        sti

        [extern main]
        call    main
        jmp     $
