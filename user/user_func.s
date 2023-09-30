[bits 32]
user_func:
        mov     dword [data], 0xcafebabe
        mov     eax, 0
        mov     ebx, message
        mov     ecx, message_end-message
        int     0x80
        mov     eax, 1
        int     0x80
        mov     eax, 2
        int     0x80
        mov     eax, 20
        mov     ebx, 30
        push    eax
        push    ebx
        pop     eax
        pop     ebx
        jmp     $
data:
dd 0x0
message:
db "hello from user program!", 0xa
message_end:
