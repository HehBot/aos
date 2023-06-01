; loads dh sectors from sector cl onwards to address es:bx from disk drive dl
disk_load:
        push    dx
        mov     ah, 0x02
        mov     al, dh          ; number of sectors
        mov     ch, 0x0         ; cylinder
        mov     dh, 0x0         ; head
                                ; start reading from sector cl (boot sector is sector 1)

        int     0x13
        jc      .disk_error

        pop     dx
        cmp     dh, al
        jne     .disk_error
        ret

.disk_error:
        mov     si, DISK_ERROR_MSG
        call    puts_16
        jmp     $

DISK_ERROR_MSG db "Disk read error!", 0
