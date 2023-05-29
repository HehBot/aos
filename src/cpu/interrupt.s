[extern isr_common]

%macro no_error_code_isr 1
[global isr_%1]
isr_%1:
        push    dword 0
        push    dword %1
        jmp     isr_stub
%endmacro

%macro error_code_isr 1
[global isr_%1]
isr_%1:
        push    dword %1
        jmp     isr_stub
%endmacro

isr_stub:
        pusha   ; pushes eax, ecx, edx, ebx, esp, ebp, esi, edi in thst order
        mov     ax, ds
        push    eax
        mov     ax, 0x10
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax

        push    esp
        call    isr_common
        add     esp, 4

        pop     eax
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        popa
        add     esp, 8
        iret

no_error_code_isr 0
no_error_code_isr 1
no_error_code_isr 2
no_error_code_isr 3
no_error_code_isr 4
no_error_code_isr 5
no_error_code_isr 6
no_error_code_isr 7
no_error_code_isr 8
no_error_code_isr 9
error_code_isr 10
error_code_isr 11
error_code_isr 12
error_code_isr 13
error_code_isr 14
no_error_code_isr 15
no_error_code_isr 16
error_code_isr 17
no_error_code_isr 18
no_error_code_isr 19
no_error_code_isr 20
no_error_code_isr 21
no_error_code_isr 22
no_error_code_isr 23
no_error_code_isr 24
no_error_code_isr 25
no_error_code_isr 26
no_error_code_isr 27
no_error_code_isr 28
no_error_code_isr 29
no_error_code_isr 30
no_error_code_isr 31
