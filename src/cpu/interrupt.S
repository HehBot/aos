#include <cpu/x86.h>

.macro NO_ERR_ISR n type name
.globl isr_\name
isr_\name:
        pushl   $0              // dummy error code
        pushl   $\n
        jmp     \type\()_stub
.endm

.macro ERR_ISR n type name
.globl isr_\name
isr_\name:
        pushl   $\n
        jmp     \type\()_stub
.endm

// see https://wiki.osdev.org/Exceptions
.macro ISR_STUB type
.extern \type
\type\()_stub:
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
        call    \type
        addl    $0x4, %esp

        popa
        popw    %ds
        popw    %es
        popw    %fs
        popw    %gs
        addl    $0x8, %esp
        iret
.endm

ISR_STUB excep
ISR_STUB irq
ISR_STUB syscall

NO_ERR_ISR 0,excep,excep0
NO_ERR_ISR 1,excep,excep1
NO_ERR_ISR 2,excep,excep2
NO_ERR_ISR 3,excep,excep3
NO_ERR_ISR 4,excep,excep4
NO_ERR_ISR 5,excep,excep5
NO_ERR_ISR 6,excep,excep6
NO_ERR_ISR 7,excep,excep7
ERR_ISR 8,excep,excep8
NO_ERR_ISR 9,excep,excep9
ERR_ISR 10,excep,excep10
ERR_ISR 11,excep,excep11
ERR_ISR 12,excep,excep12
ERR_ISR 13,excep,excep13
ERR_ISR 14,excep,excep14
NO_ERR_ISR 15,excep,excep15
NO_ERR_ISR 16,excep,excep16
ERR_ISR 17,excep,excep17
NO_ERR_ISR 18,excep,excep18
NO_ERR_ISR 19,excep,excep19
NO_ERR_ISR 20,excep,excep20
ERR_ISR 21,excep,excep21
NO_ERR_ISR 22,excep,excep22
NO_ERR_ISR 23,excep,excep23
NO_ERR_ISR 24,excep,excep24
NO_ERR_ISR 25,excep,excep25
NO_ERR_ISR 26,excep,excep26
NO_ERR_ISR 27,excep,excep27
NO_ERR_ISR 28,excep,excep28
ERR_ISR 29,excep,excep29
ERR_ISR 30,excep,excep30
NO_ERR_ISR 31,excep,excep31
NO_ERR_ISR T_IRQ0,irq,irq0
NO_ERR_ISR T_IRQ1,irq,irq1
NO_ERR_ISR T_IRQ2,irq,irq2
NO_ERR_ISR T_IRQ3,irq,irq3
NO_ERR_ISR T_IRQ4,irq,irq4
NO_ERR_ISR T_IRQ5,irq,irq5
NO_ERR_ISR T_IRQ6,irq,irq6
NO_ERR_ISR T_IRQ7,irq,irq7
NO_ERR_ISR T_IRQ8,irq,irq8
NO_ERR_ISR T_IRQ9,irq,irq9
NO_ERR_ISR T_IRQ10,irq,irq10
NO_ERR_ISR T_IRQ11,irq,irq11
NO_ERR_ISR T_IRQ12,irq,irq12
NO_ERR_ISR T_IRQ13,irq,irq13
NO_ERR_ISR T_IRQ14,irq,irq14
NO_ERR_ISR T_IRQ15,irq,irq15
NO_ERR_ISR T_SYSCALL,syscall,syscall
