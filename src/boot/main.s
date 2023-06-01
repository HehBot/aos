[bits 16]
[org 0x7c00]

        jmp     0x0000:start
start:
        ; initialise segment registers and stack
        xor     ax, ax
        mov     ds, ax
        mov     es, ax

        mov     bp, 0xfff0
        mov     ss, ax
        mov     sp, bp

        mov     byte [BOOT_DRIVE], dl

        mov     si, MSG_REAL_MODE
        call    puts_16

        mov     si, MSG_LOAD_KERNEL
        call    puts_16

        ; load initial paging structures
        mov     bx, 0x8000
        mov     dh, 16
        mov     dl, byte [BOOT_DRIVE]
        mov     cl, 2
        call    disk_load

        ; load kernel
        mov     bx, KERNEL_OFFSET
        mov     dh, KERNEL_SECTORS_SIZE
        mov     dl, byte [BOOT_DRIVE]
        mov     cl, 18
        call    disk_load

        ; disable interrupts and load GDT
        cli
        lgdt    [gdt_descriptor]

        ; switch to 32 bit protected mode
        mov     eax, cr0
        or      eax, 0x1
        mov     cr0, eax
        jmp     CODE_SEG:init_pm

        jmp     $

BOOT_DRIVE db 0
MSG_REAL_MODE db "In 16-bit Real Mode", 0x0d, 0x0a, 0
MSG_LOAD_KERNEL db "Loading kernel into memory", 0x0d, 0x0a, 0

%include "print_16.s"
%include "disk_load_16.s"
%include "gdt.s"

[bits 32]
%include "print_32.s"

init_pm:
        ; initialise segment registers and stack
        mov     ax, DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax

        mov     ebp, 0x90000
        mov     ss, ax
        mov     esp, ebp

        ; enable interrupts
        sti

        mov     esi, MSG_PROT_MODE
        call    puts

        jmp     KERNEL_OFFSET

        jmp     $

MSG_PROT_MODE db "In 32-bit Protected Mode", 0

times 510-($-$$) db 0
dw 0xaa55
