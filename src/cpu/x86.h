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
static inline void hlt()
{
    asm volatile("hlt");
}

_Noreturn __attribute__((format(printf, 1, 2))) void panic(char const* fmt, ...);
    #define PANIC(fmt, ...)                                                                     \
        do {                                                                                    \
            panic("Panicked at " __FILE__ ":%d: %s:\n" fmt, __LINE__, __func__, ##__VA_ARGS__); \
        } while (0)
    #define ASSERT(x)                                  \
        do {                                           \
            if (!(x))                                  \
                PANIC("Assertion `%s' failed.\n", #x); \
        } while (0)

static inline uint64_t read_rflags(void)
{
    uint64_t rflags;
    asm volatile("pushfq; pop %q0" : "=r"(rflags));
    return rflags;
}
    #define RFLAGS_INT 0x00000200

// IDT
typedef struct {
    uint64_t r15, r14, r13, r12;
    uint64_t r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp;
    uint64_t rbx, rdx, rcx, rax;

    uint64_t int_no, err_code;

    // pushed by cpu
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) cpu_state_t;
typedef void (*isr_t)(cpu_state_t*);

typedef struct {
    uint16_t low_offset;
    uint16_t seg;
    uint8_t ist : 3;
    uint8_t : 5;
    uint8_t gate_type : 4;
    uint8_t : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    uint16_t mid_offset;
    uint32_t high_offset;
    uint32_t : 32;
} __attribute__((packed)) idt_entry_t;

    #define IDT_ENTRIES 256
static inline void lidt(idt_entry_t* idt, uint16_t size)
{
    typedef struct {
        uint16_t size_m_1;
        uint64_t base;
    } __attribute__((packed)) idt_register_t;
    volatile idt_register_t reg = { size - 1, (uintptr_t)idt };
    asm volatile("lidt (%0)" ::"r"(&reg));
}
#endif // __ASSEMBLER__

#define T_BREAKPOINT 0x03
#define T_DOUBLE_FAULT 0x08
#define T_IRQ0 0x20

#define IRQ_TIMER 0
#define IRQ_KBD 1
#define IRQ_IDE 14
#define IRQ_ERR 19
#define IRQ_SPUR 31

// GDT and TSS
#ifndef __ASSEMBLER__
typedef union gdt_entry {
    struct {
        union {
            uint16_t APP_limit_low;
            uint16_t SYS_tss_size;
        };
        uint16_t base_low;
        uint8_t base_mid;
        union {
            struct {
                uint8_t segment_type : 4;
                uint8_t SYS_sys_or_app_seg : 1;
                uint8_t SYS_dpl : 2;
                uint8_t SYS_present : 1;
            } __attribute__((packed));
            struct {
                uint8_t accessed : 1;
                uint8_t readable_writeable : 1;
                uint8_t conforming_expand_down : 1;
                uint8_t data_or_code : 1;
                uint8_t APP_sys_or_app_seg : 1;
                uint8_t APP_dpl : 2;
                uint8_t APP_present : 1;
            } __attribute__((packed));
        } __attribute__((packed));
        uint8_t : 4; // limit_high
        uint8_t : 1; // reserved
        uint8_t long_mode : 1;
        uint8_t : 2; // type_16b_or_32b, gran
        uint8_t base_high;
    } __attribute__((packed));
    struct {
        uint32_t base_upper_32;
        uint32_t : 32;
    } __attribute__((packed));
} __attribute__((packed)) gdt_entry_t;
    #define GDT_STRIDE (sizeof(gdt_entry_t))

    #define INTERRUPT_IST 0

typedef struct tss {
    uint32_t : 32;
    uint64_t pvt[3];
    uint64_t : 64;
    uint64_t ist[7];
    uint64_t : 64;
    uint16_t : 16;
    uint16_t iomap_base;
} __attribute__((packed, __aligned__(16))) tss_t;

    #define SEG(pl, d_or_c, rw)          \
        (gdt_entry_t)                    \
        {                                \
            .accessed = 1,               \
            .readable_writeable = (rw),  \
            .conforming_expand_down = 0, \
            .data_or_code = (d_or_c),    \
            .APP_sys_or_app_seg = 1,     \
            .APP_dpl = (pl),             \
            .APP_present = 1,            \
            .long_mode = (1 & (d_or_c)), \
        }
    #define SEG_TSS1(pl, tss, tss_size)                       \
        (gdt_entry_t)                                         \
        {                                                     \
            .SYS_tss_size = (tss_size),                       \
            .base_low = (((uintptr_t)(tss)) & 0xffff),        \
            .base_mid = ((((uintptr_t)(tss)) >> 16) & 0xff),  \
            .base_high = ((((uintptr_t)(tss)) >> 24) & 0xff), \
            .segment_type = 0x9,                              \
            .SYS_sys_or_app_seg = 0,                          \
            .SYS_dpl = pl,                                    \
            .SYS_present = 1,                                 \
            .long_mode = 1,                                   \
        }
    #define SEG_TSS2(tss)                                \
        (gdt_entry_t)                                    \
        {                                                \
            .base_upper_32 = (((uintptr_t)(tss)) >> 32), \
        }

static inline void lgdt(gdt_entry_t* gdt, uint16_t size, uint16_t cs, uint16_t ds)
{
    typedef struct gdt_desc {
        uint16_t size_m_1;
        gdt_entry_t* gdt;
    } __attribute__((packed)) gdt_desc_t;
    volatile gdt_desc_t gdtd = { size - 1, gdt };
    asm volatile(
        "lgdt (%1); "
        "push %q0; "
        "mov $1f, %q2; "
        "push %q2; "
        "retfq; "
        "1:"
        "mov %w3, %%ds; "
        "mov %w3, %%es; "
        "mov %w3, %%fs; "
        "mov %w3, %%gs; "
        "mov %w3, %%ss; "
        : : "r"(cs), "r"(&gdtd), "r"(0), "r"(ds));
}
static inline void ltr(uint16_t tss_seg)
{
    asm volatile("ltr %w0" ::"r"(tss_seg));
}

#else
    #define GDT_STRIDE 8
#endif // __ASSEMBLER__

#define NR_GDT_ENTRIES 7

#define KERNEL_CODE_GDT_INDEX 1
#define KERNEL_DATA_GDT_INDEX 2
#define USER_DATA_GDT_INDEX 3
#define USER_CODE_GDT_INDEX 4
#define TSS_GDT_INDEX 5

#define KERNEL_PL 0
#define USER_PL 3

#define KERNEL_CODE_SEG ((KERNEL_CODE_GDT_INDEX * GDT_STRIDE) | KERNEL_PL)
#define KERNEL_DATA_SEG ((KERNEL_DATA_GDT_INDEX * GDT_STRIDE) | KERNEL_PL)
#define USER_DATA_SEG ((USER_DATA_GDT_INDEX * GDT_STRIDE) | USER_PL)
#define USER_CODE_SEG ((USER_CODE_GDT_INDEX * GDT_STRIDE) | USER_PL)
#define TSS_SEG ((TSS_GDT_INDEX * GDT_STRIDE) | KERNEL_PL)

// Memory and Paging
#define CR4_PAE 0x20
#define CR0_WP 0x10000
#define CR0_PG 0x80000000

#ifndef __ASSEMBLER__

_Static_assert(USER_DATA_GDT_INDEX + 1 == USER_CODE_GDT_INDEX, "Bad user segments");

    #include <memory/page.h>

static inline void write_cr3(phys_addr_t p4_phys_addr)
{
    asm volatile("mov %q0, %%cr3" ::"r"(p4_phys_addr) : "memory");
}
static inline phys_addr_t read_cr3()
{
    phys_addr_t p4_phys_addr;
    asm volatile("mov %%cr3, %q0" : "=r"(p4_phys_addr)::"memory");
    return p4_phys_addr;
}
static inline virt_addr_t read_cr2(void)
{
    virt_addr_t pa = NULL;
    asm volatile("movq %%cr2, %0"
                 : "=r"(pa)
                 :);
    return pa;
}
static inline void flush_tlb(virt_addr_t va)
{
    asm volatile("invlpg (%q0)" ::"r"(va) : "memory");
}
static inline void flush_tlb_all()
{
    write_cr3(read_cr3());
}
static inline void disable_interrupts()
{
    asm volatile("cli");
}
static inline void enable_interrupts()
{
    asm volatile("sti");
}

#endif // __ASSEMBLER__

#endif // X86_H
