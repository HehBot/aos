[bits 64]
main:
        mov     dword [data], 0xcafebabe
        mov     rax, 0
        mov     rdi, message
        mov     rsi, message_end-message
        syscall

        mov     rax, 1000000000
.label:
        dec     rax
        test    rax, rax
        jnz     .label

        jmp     main
data:
dd 0x0
message:
db "hello from user program!", 0xa
message_end:
