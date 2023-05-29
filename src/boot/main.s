[bits 16]
[org 0x7c00]

        jmp     0x0000:start
start:
        ; initialise segment registers and stack
        xor     ax, ax
        mov     ds, ax
        mov     es, ax

        mov     bp, 0x8000
        mov     ss, ax
        mov     sp, bp

        mov     byte [BOOT_DRIVE], dl

        mov     bx, 0x9000
        mov     dh, 2
        mov     dl, [BOOT_DRIVE]
        call    disk_load

        mov     dx, [0x9000]
        call    print_hex

        mov     dx, [0x9000 + 512]
        call    print_hex

        jmp     $

%include "print_16bit.s"
%include "disk_load_16bit.s"

HELLO_WORLD db "Hello world!", 0x0d, 0x0a, 0
BOOT_DRIVE db 0

times 510-($-$$) db 0
dw 0xaa55

times 256 dw 0xdada
times 256 dw 0xface
