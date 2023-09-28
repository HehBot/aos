#ifndef X86_H
#define X86_H

#include <stddef.h>
#include <stdint.h>

#define KERN_BASE 0xc0000000

// Panic
static inline _Noreturn void PANIC()
{
    asm volatile("int 0x1"
                 :
                 :);
    __builtin_unreachable();
}

// IDT
static inline void lidt(uintptr_t idt_reg_addr)
{
    asm("lidt [%0]"
        :
        : "r"(idt_reg_addr));
}

// GDT
typedef struct gdt_entry {
    uint16_t limit;
    uint16_t base1;
    uint8_t base2;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t base3;
} __attribute__((packed)) gdt_entry_t;
typedef struct gdt_desc {
    uint16_t size;
    gdt_entry_t* gdt;
} __attribute__((packed)) gdt_desc_t;

// Memory and Paging
typedef uint32_t pde_t;
typedef uint32_t pte_t;

#define PAGE_SIZE 0x1000
#define LPAGE_SIZE 0x400000
#define PAGE_ORDER 12
#define LPAGE_ORDER 22

#define PTX(a) (((a) >> PAGE_ORDER) & 0x3ff)
#define PDX(a) ((a) >> LPAGE_ORDER)

#define PG_ROUND_DOWN(a) (((a) >> PAGE_ORDER) << PAGE_ORDER)
#define PG_ROUND_UP(a) PG_ROUND_DOWN((a) + PAGE_SIZE - 1)
#define LPG_ROUND_DOWN(a) (((a) >> LPAGE_ORDER) << LPAGE_ORDER)
#define LPG_ROUND_UP(a) LPG_ROUND_DOWN((a) + LPAGE_SIZE - 1)

#define PTE_P 0x001
#define PTE_W 0x002
#define PTE_U 0x004
#define PTE_LP 0x080

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

typedef struct page_table {
    pte_t pages[1024];
} page_table_t;

typedef struct page_directory {
    page_table_t* tables[1024];
    pde_t tables_physical[1024];
    uintptr_t physical_addr;
} page_directory_t;

static inline void invlpg(uintptr_t va)
{
    asm volatile("invlpg [%0]" ::"r"(va)
                 : "memory");
}
static inline void lcr3(uintptr_t pa)
{
    asm volatile("mov cr3, %0"
                 :
                 : "r"(pa));
}
static inline uintptr_t rcr2(void)
{
    uintptr_t pa;
    asm volatile("mov %0, cr2"
                 : "=r"(pa)
                 :);
    return pa;
}

#endif // X86_H
