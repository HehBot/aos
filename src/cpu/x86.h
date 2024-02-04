#ifndef X86_H
#define X86_H

#ifndef __ASSEMBLER__
    #include <stddef.h>
    #include <stdint.h>
#endif // __ASSEMBLER__

#define KERN_BASE 0xc0000000

// General
#ifndef __ASSEMBLER__
static inline _Noreturn void PANIC()
{
    asm("int $0x1");
    __builtin_unreachable();
}
static inline _Noreturn void HALT()
{
    asm("cli; hlt");
    __builtin_unreachable();
}

// IDT
typedef struct {
    uint32_t edi, esi, ebp;
    uint32_t : 32; // useless stack pointer
    uint32_t ebx, edx, ecx, eax;

    uint16_t ds, es, fs, gs;

    uint32_t int_no, err_code;

    // pushed by cpu
    uint32_t eip;
    uint16_t cs;
    uint16_t : 16;
    uint32_t eflags;

    // pushed by cpu only on pl change
    uint32_t useresp;
    uint16_t ss;
    uint16_t : 16;
} __attribute__((packed)) cpu_state_t;
typedef void (*isr_t)(cpu_state_t*);

static inline void lidt(uintptr_t idt_reg_addr)
{
    asm("lidt (%0)" ::"r"(idt_reg_addr));
}
#endif // __ASSEMBLER__

#define T_IRQ0 32
#define T_IRQ1 33
#define T_IRQ2 34
#define T_IRQ3 35
#define T_IRQ4 36
#define T_IRQ5 37
#define T_IRQ6 38
#define T_IRQ7 39
#define T_IRQ8 40
#define T_IRQ9 41
#define T_IRQ10 42
#define T_IRQ11 43
#define T_IRQ12 44
#define T_IRQ13 45
#define T_IRQ14 46
#define T_IRQ15 47

#define T_SYSCALL 0x80

// GDT
#ifndef __ASSEMBLER__
typedef struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t accessed : 1;
    uint8_t read_write : 1;
    uint8_t conforming_expand_down : 1;
    uint8_t code : 1;
    uint8_t code_data_segment : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    uint8_t limit_high : 4;
    uint8_t available : 1;
    uint8_t long_mode : 1;
    uint8_t big : 1;
    uint8_t gran : 1;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;
typedef struct gdt_desc {
    uint16_t size;
    gdt_entry_t* gdt;
} __attribute__((packed)) gdt_desc_t;
    #define GDT_STRIDE (sizeof(gdt_entry_t))

static inline void ltr(uint16_t tss_seg)
{
    asm("ltr %w0" ::"r"(tss_seg));
}

#else
    #define GDT_STRIDE 8
#endif // __ASSEMBLER__

#define KERNEL_CODE_GDT_INDEX 1
#define KERNEL_DATA_GDT_INDEX 2
#define USER_CODE_GDT_INDEX 3
#define USER_DATA_GDT_INDEX 4
#define TSS_GDT_INDEX 5

#define KERNEL_PL 0
#define USER_PL 3

#define KERNEL_CODE_SEG (KERNEL_CODE_GDT_INDEX * GDT_STRIDE) | KERNEL_PL
#define KERNEL_DATA_SEG (KERNEL_DATA_GDT_INDEX * GDT_STRIDE) | KERNEL_PL
#define USER_CODE_SEG (USER_CODE_GDT_INDEX * GDT_STRIDE) | USER_PL
#define USER_DATA_SEG (USER_DATA_GDT_INDEX * GDT_STRIDE) | USER_PL
#define TSS_SEG (TSS_GDT_INDEX * GDT_STRIDE)

// Memory and Paging
#define CR4_PSE 0x10
#define CR0_WP 0x10000
#define CR0_PG 0x80000000

#include "page.h"

#ifndef __ASSEMBLER__
typedef struct mem_map {
    uint16_t present : 1;
    uint16_t mapped : 1;
    uint16_t perms : 14;
    uintptr_t virt;
    union {
        struct {
            uintptr_t phy_start;
            uintptr_t phy_end;
        };
        size_t nr_pages;
    };
} __attribute__((packed)) mem_map_t;

static inline void invlpg(uintptr_t va)
{
    asm("invlpg (%0)" ::"r"(va)
        : "memory");
}
static inline void lcr3(uintptr_t pgdir_pa)
{
    asm("movl %0, %%cr3" ::"r"(pgdir_pa)
        : "memory");
}
static inline uintptr_t rcr2(void)
{
    uintptr_t pa;
    asm("movl %%cr2, %0"
        : "=r"(pa)
        :);
    return pa;
}

    #define INIT_HEAP_SZ 16 // size of initial kernel heap in pages

#endif // __ASSEMBLER__

#endif // X86_H
