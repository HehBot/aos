#include <cpu/x86.h>
#include <mem/page.h>

.globl long_mode_start
.extern main
.extern KERN_BASE


.section .bss
.align 4096
p4_table:
.skip 0x1000
p3_table:
.skip 0x1000
p2_table:
.skip 0x1000
.align 16
stack_end:
.skip 0x1000
stack:


.section .text
.code64
long_mode_start:
        mov $0, %ax
        mov %ax, %ds
        mov %ax, %es
        mov %ax, %fs
        mov %ax, %gs

        mov $stack, %rbp
        mov %ax, %ss
        mov %rbp, %rsp

        call set_up_page_tables
        mov $p4_table, %rax
        sub $KERN_BASE, %rax
        mov %rax, %cr3

        movabs $0x2f592f412f4b2f4f, %rax
        mov %rax, 0xb8000

        call main
        hlt


/*
 * sets up following mappings
 * 0x0 -> 64MiB                         to 0x0 -> 64MiB (identity mapping)
 * KERN_BASE -> KERN_BASE+64MiB         to 0x0 -> 64MiB (proper mapping)
 */
set_up_page_tables:
        // get offsets needed for proper mapping of KERN_BASE
        //      %ecx contains p4_index
        //      %ebx contains p3_index
        mov $KERN_BASE, %rbx
        shr $30, %rbx
        mov %rbx, %rcx
        and $0x1ff, %rbx
        shr $9, %rcx
        and $0x1ff, %rcx

        mov $p3_table, %rax
        sub $KERN_BASE, %rax
        or $(PTE_W | PTE_P), %rax
        // identity
        mov %rax, p4_table
        // proper
        mov %rax, p4_table(,%rcx,8)

        mov $p2_table, %rax
        sub $KERN_BASE, %rax
        or $(PTE_W | PTE_P), %rax
        // identity
        mov %rax, p3_table
        // proper
        mov %rax, p3_table(,%rbx,8)

        // map each p2 entry to a huge 2MiB page
        mov $0, %rcx
1:
        mov $0x200000, %rax
        mul %rcx
        or $(PTE_HP | PTE_W | PTE_P), %rax
        mov %rax, p2_table(,%rcx,8)

        inc %rcx
        cmp $32, %rcx
        jne 1b

        ret
