#include <cpu/x86.h>

.altmacro

.section .text
.macro isr n
isr_\n:
        pushl   $0              // dummy error code
        pushl   $\n
        jmp     isr_stub
.endm

.macro errcode_isr n
isr_\n:
        pushl   $\n
        jmp     isr_stub
.endm

// see https://wiki.osdev.org/Exceptions
.extern isr_common
isr_stub:
        pushw   %gs
        pushw   %fs
        pushw   %es
        pushw   %ds
        pusha                   // pushes eax, ecx, edx, ebx, esp, ebp, esi, edi in that order

        movw    $KERNEL_DATA_SEG, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs

        pushl   %esp
        call    isr_common
        addl    $0x4, %esp

        popa
        popw    %ds
        popw    %es
        popw    %fs
        popw    %gs
        addl    $0x8, %esp
        iret

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
    .long isr_\n
.endm
.set i,0
.rept 256
    isr_addr %i
    .set i,i+1
.endr
