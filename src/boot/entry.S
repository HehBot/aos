#include <cpu/x86.h>
#include <multiboot.h>

#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE)
#define MULTIBOOT_CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

.section .multiboot.data, "ax"
.align MULTIBOOT_HEADER_ALIGN
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long MULTIBOOT_CHECKSUM
.long 0x0       // header_addr
.long 0x0       // load_addr
.long 0x0       // load_end_addr
.long 0x0       // bss_end_addr
.long 0x0       // entry_addr
// GFX requests
.long 0x1       // mode_type
.long 0x0       // width
.long 0x0       // height
.long 0x0       // depth (bpp)

.section .bss
.align 16
stack_top:
.skip 0x1000
stack_bot:

.section .multiboot.text, "ax"
.globl _start
.extern entry_pgdir
.extern mboot_magic
.extern mboot_info
_start:
        cli

        // store mboot header
        movl    $mboot_magic, %ecx
        movl    %eax, -KERN_BASE(%ecx)
        movl    $mboot_info, %edx
        leal    -KERN_BASE(%edx), %edi
        movl    %ebx, %esi
        movl    $SIZEOF_STRUCT_MULTIBOOT_INFO, %ecx
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

.section .text
.extern gdt_desc
.extern isr_install
.extern main
start:
        // load GDT
        lgdt    gdt_desc
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

        // remove identity map entry
        movl    $0x0, entry_pgdir

        // install interrupt handlers and enable interrupts
        call    isr_install
        sti

        call    main
2:
        jmp     2b
