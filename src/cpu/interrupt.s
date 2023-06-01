[extern isr_excep_common]
[extern isr_irq_common]

%macro no_error_code_isr 2
[global isr_%1_%2]
isr_%1_%2:
        push    dword 0
        push    dword %1
        jmp     isr_%2_stub
%endmacro

%macro error_code_isr 2
[global isr_%1_%2]
isr_%1_%2:
        push    dword %1
        jmp     isr_%2_stub
%endmacro

; see https://wiki.osdev.org/Exceptions
isr_excep_stub:
        pusha   ; pushes eax, ecx, edx, ebx, esp, ebp, esi, edi in that order
        mov     ax, ds
        push    eax
        mov     ax, 0x10
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax

        push    esp
        call    isr_excep_common
        add     esp, 4

        pop     eax
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        popa
        add     esp, 8
        iret
no_error_code_isr 0,excep
no_error_code_isr 1,excep
no_error_code_isr 2,excep
no_error_code_isr 3,excep
no_error_code_isr 4,excep
no_error_code_isr 5,excep
no_error_code_isr 6,excep
no_error_code_isr 7,excep
error_code_isr 8,excep
no_error_code_isr 9,excep
error_code_isr 10,excep
error_code_isr 11,excep
error_code_isr 12,excep
error_code_isr 13,excep
error_code_isr 14,excep
no_error_code_isr 15,excep
no_error_code_isr 16,excep
error_code_isr 17,excep
no_error_code_isr 18,excep
no_error_code_isr 19,excep
no_error_code_isr 20,excep
error_code_isr 21,excep
no_error_code_isr 22,excep
no_error_code_isr 23,excep
no_error_code_isr 24,excep
no_error_code_isr 25,excep
no_error_code_isr 26,excep
no_error_code_isr 27,excep
no_error_code_isr 28,excep
error_code_isr 29,excep
error_code_isr 30,excep
no_error_code_isr 31,excep

; see https://wiki.osdev.org/Exceptions
isr_irq_stub:
        pusha   ; pushes eax, ecx, edx, ebx, esp, ebp, esi, edi in thst order
        mov     ax, ds
        push    eax
        mov     ax, 0x10
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax

        push    esp
        call    isr_irq_common
        add     esp, 4

        pop     eax
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        popa
        add     esp, 8
        iret
no_error_code_isr 32,irq
no_error_code_isr 33,irq
no_error_code_isr 34,irq
no_error_code_isr 35,irq
no_error_code_isr 36,irq
no_error_code_isr 37,irq
no_error_code_isr 38,irq
no_error_code_isr 39,irq
no_error_code_isr 40,irq
no_error_code_isr 41,irq
no_error_code_isr 42,irq
no_error_code_isr 43,irq
no_error_code_isr 44,irq
no_error_code_isr 45,irq
no_error_code_isr 46,irq
no_error_code_isr 47,irq
