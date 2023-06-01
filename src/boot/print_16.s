; print character in si
putc_16:
        push    ax
        mov     ax, si
        mov     ah, 0x0e
        int     0x10
        pop     ax
        ret

; print null-ended string at address si
puts_16:
        push    ax
        push    bx
        push    si
        mov     ah, 0x0e
.loop:
        mov     al, byte [si]
        cmp     al, 0
        je      .end_loop
        int     0x10
        inc     si
        jmp     .loop
.end_loop:
        pop     si
        pop     bx
        pop     ax
        ret

; print hex number in dx
print_hex_16:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     ah, 0x0e
        xor     cx, cx
.loop:
        rol     dx, 4
        mov     al, dl
        and     al, 0xf
        cmp     al, 0xa
        jge     .else
.if:
        add     al, 0x30        ; '0' - 0x0 = 0x30
        jmp     .endif
.else:
        add     al, 0x37        ; 'A' - 0xA = 0x37
.endif:
        int     0x10
        inc     cx
        cmp     cx, 4
        jne     .loop

        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
