#include <cpu/x86.h>

.altmacro

.section .text
.macro isr n
isr_\n:
        pushq   $0              // dummy error_code
        pushq   $\n
        jmp     isr_stub
.endm

.macro errcode_isr n
isr_\n:
        pushq   $\n
        jmp     isr_stub
.endm

// see Intel Manual Vol3 6.14.2
.extern isr_common
isr_stub:
        push    %rax
        push    %rcx
        push    %rdx
        push    %rbx
        push    %rbp
        push    %rsi
        push    %rdi
        push    %r8
        push    %r9
        push    %r10
        push    %r11
        push    %r12
        push    %r13
        push    %r14
        push    %r15

        mov     %rsp, %rdi      // param to isr_common
        cld
        call    isr_common

        test    %rax, %rax
        jz      .dont_replace_stack
        mov     %rax, %rsp
.dont_replace_stack:

        pop     %r15
        pop     %r14
        pop     %r13
        pop     %r12
        pop     %r11
        pop     %r10
        pop     %r9
        pop     %r8
        pop     %rdi
        pop     %rsi
        pop     %rbp
        pop     %rbx
        pop     %rdx
        pop     %rcx
        pop     %rax

        addq    $0x10, %rsp     // remove error_code and int_no
        iretq

isr 0
isr 1
isr 2
isr 3
isr 4
isr 5
isr 6
isr 7
errcode_isr 8
isr 9
errcode_isr 10
errcode_isr 11
errcode_isr 12
errcode_isr 13
errcode_isr 14
isr 15
isr 16
errcode_isr 17
isr 18
isr 19
isr 20
errcode_isr 21
isr 22
isr 23
isr 24
isr 25
isr 26
isr 27
isr 28
errcode_isr 29
errcode_isr 30
isr 31
.set i,32
.rept 224
    isr %i
    .set i,i+1
.endr


.section .data
.global isrs

isrs:
.macro isr_addr n
    .quad isr_\n
.endm
.set i,0
.rept 256
    isr_addr %i
    .set i,i+1
.endr
