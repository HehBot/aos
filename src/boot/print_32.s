VIDEO_MEMORY equ 0xb8000
WHITE_ON_BLACK equ 0x0f

; prints a null-terminated string at address esi
puts:
        push    esi
        push    edx
        mov     edx, VIDEO_MEMORY

.loop:
        mov     al, byte [esi]
        mov     ah, WHITE_ON_BLACK

        cmp     al, 0
        je      .loop_end

        mov     word [edx], ax
        inc     esi
        add     edx, 2

        jmp     .loop
.loop_end:
        pop     edx
        pop     esi
        ret
