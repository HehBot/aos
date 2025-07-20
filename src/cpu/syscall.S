#include <cpu/x86.h>

.globl init_syscall_sysret
init_syscall_sysret:
        push %rdx
        push %rcx
        push %rbx
        push %rax

        // set KERNEL_GS_BASE MSR
        // to parameter in %rdi
        mov %edi, %eax
        shr $32, %rdi
        mov %edi, %edx
        mov $0xc0000102, %rcx
        wrmsr

        // set STAR MSR
        mov $USER_CODE_SEG, %rdx
        sub $16, %rdx   // CS = val+16, SS = val+8
        shl $16, %rdx
        mov $KERNEL_CODE_SEG, %rbx
        or %rbx, %rdx
        mov $0xc0000081, %rcx
        wrmsr

        // set LSTAR MSR
        movabs $syscall_entry, %rbx
        mov %ebx, %eax
        shr $32, %rbx
        mov %ebx, %edx
        mov $0xc0000082, %rcx
        wrmsr

        // set SFMASK MSR
        // clear direction flag
        mov $0xc0000084, %rcx
        rdmsr
        or $(1 << 10), %eax
        wrmsr

        // set SYSCALL/SYSRET bit in EFER MSR
        mov $0xc0000080, %ecx
        rdmsr
        or $(1 << 0), %eax
        wrmsr

        pop %rax
        pop %rbx
        pop %rcx
        pop %rdx

        ret

.extern syscall_handler
syscall_entry:
        swapgs
        mov %rsp, %gs:0
        mov %gs:8, %rsp
        pushq %gs:0     // save old rsp

        // the callee-saved registers
        // rbx, rbp, r12-15 will anyways
        // be saved and restored by
        // syscall_handler
        push %rcx       // save old rip
        push %r11       // save old rflags

        push %r9
        push %r8
        push %r10
        push %rdx
        push %rsi
        push %rdi
        push %rax

        mov %rsp, %rdi
        call syscall_handler

        add $56, %rsp

        pop %r11
        pop %rcx

        pop %rsp
        swapgs
        sysretq
