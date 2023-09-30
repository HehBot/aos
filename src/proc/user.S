#include <cpu/x86.h>

// void enter_usermode(cpu_state_t*)
.globl enter_usermode
enter_usermode:
        movl    4(%esp), %eax

        movl    0(%eax), %edi
        movl    4(%eax), %esi
        movl    8(%eax), %ebp
        movl    16(%eax), %ebx
        movl    20(%eax), %edx
        movl    24(%eax), %ecx

        movw    32(%eax), %ds
        movw    34(%eax), %es
        movw    36(%eax), %fs
        movw    38(%eax), %gs

        pushl   64(%eax)        // ss
        pushl   60(%eax)        // useresp
        pushf                   // eflags
        pushl   52(%eax)        // cs
        pushl   48(%eax)        // eip

        movl    28(%eax), %eax
        iret
