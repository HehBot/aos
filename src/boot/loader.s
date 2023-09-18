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

[section .bss]
alignb 16
STACK_SIZE equ 0x4000
stack_b:
resb STACK_SIZE
stack_t:
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
        mov     ebp, stack_t
        mov     ss, ax
        mov     esp, ebp

        ; remove identity map entry
        mov     dword [entry_pgdir+0], 0x00000000

        ; install interrupt handlers and enable interrupts
        [extern isr_install]
        call    isr_install
        sti

        [extern _kernel_start]
        [extern _kernel_end]
        [extern _kernel_physical_start]
        [extern _kernel_physical_end]
        [extern kernel_start]
        [extern kernel_end]
        [extern kernel_physical_start]
        [extern kernel_physical_end]
        [extern stack_bottom]
        [extern stack_size]
        mov     dword [kernel_start], _kernel_start
        mov     dword [kernel_end], _kernel_end
        mov     dword [kernel_physical_start], _kernel_physical_start
        mov     dword [kernel_physical_end], _kernel_physical_end
        mov     dword [stack_bottom], stack_b
        mov     dword [stack_size], STACK_SIZE

        [extern main]
        call    main
        jmp     $
