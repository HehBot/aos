#include <cpu/x86.h>
#define ASM_FILE
#include <multiboot2.h>

.section .multiboot.data, "ax"
.extern load_addr
.extern load_end_addr
.extern bss_end_addr
.align 16
multiboot_header:
.long MULTIBOOT2_HEADER_MAGIC
.long MULTIBOOT_ARCHITECTURE_I386
.long (multiboot_header_end - multiboot_header)
.long (-(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header)))

.align 8
address_tag_start:
.short MULTIBOOT_HEADER_TAG_ADDRESS
.short MULTIBOOT_HEADER_TAG_OPTIONAL
.long address_tag_end-address_tag_start
.long multiboot_header
.long load_addr
.long load_end_addr
.long bss_end_addr
address_tag_end:

.align 8
entry_address_tag_start:
.short MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS
.short MULTIBOOT_HEADER_TAG_OPTIONAL
.long entry_address_tag_end-entry_address_tag_start
.long _start
entry_address_tag_end:

.align 8
.short MULTIBOOT_HEADER_TAG_END
.short 0
.long 8
multiboot_header_end:

.section .bss
.align 16
stack_top:
.skip 0x1000
stack_bot:

.section .multiboot.text, "ax"
.globl _start
.extern entry_pgdir
_start:
        cli

        // store mboot header
        movl    $mboot_magic, %ecx
        movl    %eax, -KERN_BASE(%ecx)
        movl    $__mboot_info, %edx
        leal    -KERN_BASE(%edx), %edi
        movl    %ebx, %esi
        movl    (%ebx), %ecx
        rep     movsb

        // enable paging
        movl    $entry_pgdir, %eax
        subl    $KERN_BASE, %eax
        movl    %eax, %cr3
        movl    %cr4, %ebx
        orl     $CR4_PSE, %ebx
        movl    %ebx, %cr4
        movl    %cr0, %ebx
        orl     $(CR0_WP | CR0_PG), %ebx
        movl    %ebx, %cr0

        jmp     start
_end:

.section .text
mboot_magic:
.long 0
.align 8
__mboot_info:
.fill 500, 1, 0

.extern entry_gdt_desc
.extern isr_install
.extern main
start:
        // load GDT
        lgdt    entry_gdt_desc
        // fix segments
        ljmp    $KERNEL_CODE_SEG, $1f
1:
        movw    $KERNEL_DATA_SEG, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs
        // set up stack
        movl    $stack_bot, %ebp
        movw    %ax, %ss
        movl    %ebp, %esp

        pushl   $__mboot_info
        movl    (mboot_magic), %eax
        pushl   %eax
        cld
        call    main
2:
        jmp     2b
