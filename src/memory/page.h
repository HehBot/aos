#ifndef PAGE_H
#define PAGE_H

#ifdef __ASSEMBLER__

    #define PTE_P 0x001
    #define PTE_W 0x002
    #define PTE_HP 0x080

#else

    #define noignore __attribute__((warn_unused_result))

    #include <stddef.h>
    #include <stdint.h>

typedef uintptr_t phys_addr_t;
typedef void* virt_addr_t;

typedef uint64_t pte_t;
typedef enum pte_flags {
    PTE_P = 0x001,
    PTE_W = 0x002,
    PTE_U = 0x004,
    PTE_HP = 0x080,
    PTE_NX = 0x8000000000000000,
} pte_flags_t;

    #define PTE_FRAME(x) ((x) & 0xfffffffffffff000)

typedef enum page_table_level {
    PAGE_TABLE_P1 = 1,
    PAGE_TABLE_P2 = 2,
    PAGE_TABLE_P3 = 3,
    PAGE_TABLE_P4 = 4,
} page_table_level_t;

    #define PAGE_OFF(x) (((uintptr_t)(x)) & 0xfff)
static inline uint16_t VA_PT_INDEX(virt_addr_t addr, page_table_level_t level)
{
    return (((uintptr_t)addr) >> (12 + 9 * (level - 1))) & 0x1ff;
}
    #define VA_P1_INDEX(x) VA_PT_INDEX((x), PAGE_TABLE_P1)
    #define VA_P2_INDEX(x) VA_PT_INDEX((x), PAGE_TABLE_P2)
    #define VA_P3_INDEX(x) VA_PT_INDEX((x), PAGE_TABLE_P3)
    #define VA_P4_INDEX(x) VA_PT_INDEX((x), PAGE_TABLE_P4)

typedef enum page_type {
    PAGE_4KiB = 1,
    PAGE_2MiB = 2,
    PAGE_1GiB = 3,
} page_type_t;

    #define PAGE_ORDER 12
    #define HUGE_PAGE_ORDER (12 + 9)
    #define GIANT_PAGE_ORDER (12 + 9 + 9)

static inline page_table_level_t page_type_parent_table_level(page_type_t type)
{
    return (page_table_level_t)(int)type;
}
static inline size_t page_size(page_type_t type)
{
    return (1 << (PAGE_ORDER + ((int)type - 1) * 9));
}

    #define PAGE_SIZE (1 << PAGE_ORDER)
    #define HUGE_PAGE_SIZE (1 << HUGE_PAGE_ORDER)
    #define GIANT_PAGE_SIZE (1 << GIANT_PAGE_ORDER)

    #define PTE(pa, flags) ((pa) | (flags))
    #define NR_PTE_ENTRIES (PAGE_SIZE / sizeof(pte_t))

    #define PAGE_ROUND_DOWN(a) (((a) >> PAGE_ORDER) << PAGE_ORDER)
    #define PAGE_ROUND_UP(a) PAGE_ROUND_DOWN((a) + PAGE_SIZE - 1)

#endif

#endif // PAGE_H
