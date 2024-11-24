#include <cpu/x86.h>
#include <memory/page.h>
#define ASM_FILE
#include <multiboot2.h>

.globl _start
.extern KERN_BASE

.section .multiboot2.header, "a"
.align 16
multiboot_header:
.4byte MULTIBOOT2_HEADER_MAGIC
.4byte MULTIBOOT_ARCHITECTURE_I386
.4byte (multiboot_header_end - multiboot_header)
.4byte (-(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header)))

.align 8
.2byte MULTIBOOT_HEADER_TAG_END
.2byte 0
.4byte 8
multiboot_header_end:

.section .boot.bss, "aw"
// temporary paging hierarchy
.align 4096
p4_table:
.skip 0x1000
p3_table:
.skip 0x1000
p2_table:
.skip 0x1000
// temporary stack
stack_end:
.skip 64
stack:

.section .boot.rodata, "a"
// temporary gdt
gdt64:
NULL_SEG = 0
.8byte 0
CODE_SEG = . - gdt64
.8byte (1 << 43 | 1 << 44 | 1 << 47 | 1 << 53)
gdt64_ptr:
.2byte . - gdt64 - 1
.8byte gdt64
kern_base:
.8byte KERN_BASE

.section .boot.text, "ax"
.code32
_start:
        mov $stack, %esp
        mov %ebx, %edi

        call check_multiboot2
        call check_cpuid
        call check_long_mode

        call set_up_page_tables
        call enable_paging

        mov $gdt64_ptr, %eax
        lgdt (%eax)

        ljmp $CODE_SEG,$1f
.code64
1:
        movabs $long_mode_start, %rax
        jmp *%rax


.code32
/*
 * Check that %eax contains multiboot22 magic number
 */
check_multiboot2:
        cmp $MULTIBOOT2_BOOTLOADER_MAGIC, %eax
        jne 1f
        ret
1:
        mov $'0, %al
        jmp error


/*
 * Check if cpuid is supported by attempting to flip the id bit (bit 21) of %flags
 */
check_cpuid:
        // copy %flags into %eax (and %ecx) via stack
        pushf
        pop %eax
        mov %ecx, %eax

        // flip the id bit
        xor $(1 << 21), %eax

        // Copy eax to %flags via the stack
        push %eax
        popf

        // Copy %flags back to eax (with the flipped bit if CPUID is supported)
        pushf
        pop %eax

        // Restore %flags from the old version stored in %ecx (i.e. flipping the
        // ID bit back if it was ever flipped).
        push %ecx
        popf

        // Compare eax and %ecx. If they are equal then that means the bit
        // wasn't flipped, and CPUID isn't supported.
        cmp %eax, %ecx
        je 1f
        ret
1:
        mov $'1, %al
        jmp error


/*
 * Check if long mode is available
 */
check_long_mode:
        // test if extended processor info in available
        mov $0x80000000, %eax   // implicit argument for cpuid
        cpuid                   // get highest supported argument
        cmp $0x80000001, %eax   // it needs to be at least 0x80000001
        jb 1f                   // if it's less, the CPU is too old for long mode

        // use extended info to test if long mode is available
        mov 0x80000001, %eax    // argument for extended processor info
        cpuid                   // returns various feature bits in %ecx and %edx
        test $(1 << 29), %edx   // test if the LM-bit is set in the D-register
        jz 1f                   // If it's not set, there is no long mode
        ret
1:
        mov $'2, %al
        jmp error


/*
 * sets up following mappings
 * 0x0 -> 64MiB                         to 0x0 -> 64MiB (identity mapping)
 * KERN_BASE -> KERN_BASE+64MiB         to 0x0 -> 64MiB (proper mapping)
 */
set_up_page_tables:
        // get offsets needed for proper mapping of KERN_BASE
        //      %ecx contains p4_index
        //      %ebx contains p3_index
        mov kern_base, %ebx
        mov kern_base+4, %ecx
        and $0xffff, %ecx
        shl $2, %ecx
        shr $30, %ebx
        or %ecx, %ebx
        mov %ebx, %ecx
        and $0x1ff, %ebx
        shr $9, %ecx
        and $0x1ff, %ecx

        mov $p3_table, %eax
        or $(PTE_W | PTE_P), %eax
        // identity
        mov %eax, p4_table
        // proper
        mov %eax, p4_table(,%ecx,8)

        mov $p2_table, %eax
        or $(PTE_W | PTE_P), %eax
        // identity
        mov %eax, p3_table
        // proper
        mov %eax, p3_table(,%ebx,8)

        // map each p2 entry to a huge 2MiB page
        mov $0, %ecx
1:
        mov $0x200000, %eax
        mul %ecx
        or $(PTE_HP | PTE_W | PTE_P), %eax
        mov %eax, p2_table(,%ecx,8)

        inc %ecx
        cmp $32, %ecx
        jne 1b

        ret


enable_paging:
        // load p4_table to cr3
        mov $p4_table, %eax
        mov %eax, %cr3

        // enable PAE (Physical Address Extension) in cr4
        mov %cr4, %eax
        or $(CR4_PAE), %eax
        mov %eax, %cr4

        // set long mode, NXE bits in EFER MSR
        mov $0xc0000080, %ecx
        rdmsr
        or $(1 << 8 | 1 << 11), %eax
        wrmsr

        // enable paging and WP (Write Protect) in cr0
        mov %cr0, %eax
        or $(CR0_PG | CR0_WP), %eax
        mov %eax, %cr0

        ret


/*
 * prints `ERR: ` and error code in %al to screen and halts
 */
error:
        movl $0x4f524f45, 0xb8000
        movl $0x4f3a4f52, 0xb8004
        movl $0x4f20, 0xb8008
        mov %al, 0xb8010
        movb $0x4f, 0xb8011
        hlt
