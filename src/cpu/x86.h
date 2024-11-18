#ifndef X86_H
#define X86_H

#ifndef __ASSEMBLER__
    #include <stdarg.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>
#endif // __ASSEMBLER__

// General
#ifndef __ASSEMBLER__
static inline _Noreturn void HALT()
{
    asm("cli; hlt");
    __builtin_unreachable();
}
    #define PANIC(str)                                                   \
        do {                                                             \
            printf("%s:%d,%s: %s\n", __FILE__, __LINE__, __func__, str); \
            HALT();                                                      \
        } while (0)

    // EFLAGS
    // static inline uint32_t get_eflags(void)
    // {
    //     uint32_t eflags;
    //     asm volatile("pushfl; popl %0"
    //                  : "=r"(eflags));
    //     return eflags;
    // }
    #define EFLAGS_INT 0x00000200

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

typedef struct {
    uint16_t low_offset;
    uint16_t seg;
    uint8_t always0_1;
    uint8_t gate_type : 4;
    uint8_t always0_2 : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    uint16_t high_offset;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t size_m_1;
    uint32_t base;
} __attribute__((packed)) idt_register_t;

    #define IDT_ENTRIES 256
static inline void lidt(idt_entry_t* idt, uint16_t size)
{
    volatile idt_register_t reg = { size - 1, (uintptr_t)idt };
    asm volatile("lidt (%0)" ::"r"(&reg));
}
#endif // __ASSEMBLER__

#define T_SYSCALL 0x80
#define T_IRQ0 0x20

#define IRQ_TIMER 0
#define IRQ_KBD 1
#define IRQ_IDE 14
#define IRQ_ERR 19
#define IRQ_SPUR 31

// GDT and TSS
#ifndef __ASSEMBLER__
typedef struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t accessed : 1;
    uint8_t readable_writeable : 1;
    uint8_t conforming_expand_down : 1;
    uint8_t data_or_code : 1;
    uint8_t sys_or_app_seg : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    uint8_t limit_high : 4;
    uint8_t : 1;
    uint8_t long_mode : 1;
    uint8_t type_16b_or_32b : 1;
    uint8_t gran : 1;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;
    #define SEG(pl, d_or_c, rw, base, lim)              \
        (gdt_entry_t)                                   \
        {                                               \
            .limit_low = (uint16_t)(lim & 0xffff),      \
            .base_low = (uint16_t)(base & 0xffff),      \
            .base_mid = (uint8_t)((base >> 16) & 0xff), \
            .accessed = 1,                              \
            .readable_writeable = rw,                   \
            .conforming_expand_down = 0,                \
            .data_or_code = d_or_c,                     \
            .sys_or_app_seg = 1,                        \
            .dpl = pl,                                  \
            .present = 1,                               \
            .limit_high = ((lim >> 16) & 0xf),          \
            .long_mode = 0,                             \
            .type_16b_or_32b = 1,                       \
            .gran = 1,                                  \
            .base_high = ((base >> 24) & 0xff)          \
        }
    #define SEG_TSS(pl, base, lim)                      \
        (gdt_entry_t)                                   \
        {                                               \
            .limit_low = (uint16_t)(lim & 0xffff),      \
            .base_low = (uint16_t)(base & 0xffff),      \
            .base_mid = (uint8_t)((base >> 16) & 0xff), \
            .accessed = 1,                              \
            .readable_writeable = 0,                    \
            .conforming_expand_down = 0,                \
            .data_or_code = 1,                          \
            .sys_or_app_seg = 0,                        \
            .dpl = pl,                                  \
            .present = 1,                               \
            .limit_high = ((lim >> 16) & 0xf),          \
            .long_mode = 0,                             \
            .type_16b_or_32b = 0,                       \
            .gran = 0,                                  \
            .base_high = ((base >> 24) & 0xff)          \
        }

typedef struct tss {
    uint32_t : 32;
    uint32_t esp; // kernel stack pointer to load when changing to kernel mode.
    uint16_t ss; // kernel stack segment to load when changing to kernel mode.
    uint16_t useless[47];
} __attribute__((packed)) tss_t;
typedef struct gdt_desc {
    uint16_t size_m_1;
    gdt_entry_t* gdt;
} __attribute__((packed)) gdt_desc_t;
    #define GDT_STRIDE (sizeof(gdt_entry_t))

static inline void lgdt(gdt_entry_t* gdt, uint16_t size)
{
    volatile gdt_desc_t gdtd = { size - 1, gdt };
    asm volatile("lgdt (%0)" ::"r"(&gdtd));
}
static inline void ltr(uint16_t tss_seg)
{
    asm("ltr %w0" ::"r"(tss_seg));
}

#else
    #define GDT_STRIDE 8
#endif // __ASSEMBLER__

#define NR_GDT_ENTRIES 6

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
#define CR4_PAE 0x20
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
    asm("movq %0, %%cr3" ::"r"(pgdir_pa)
        : "memory");
}
static inline uintptr_t rcr2(void)
{
    uintptr_t pa = 0;
    asm("movq %%cr2, %0"
        : "=r"(pa)
        :);
    return pa;
}

    #define INIT_HEAP_SZ 16 // size of initial kernel heap in pages

#endif // __ASSEMBLER__

#endif // X86_H
