#ifndef PAGE_H
#define PAGE_H

#ifdef __ASSEMBLER__

    #define PTE_P 0x001
    #define PTE_W 0x002
    #define PTE_HP 0x080

#else

    #include <stdint.h>

typedef uint64_t pte_t;
typedef enum pte_flags {
    PTE_P = 0x001,
    PTE_W = 0x002,
    PTE_U = 0x004,
    PTE_HP = 0x080,
    PTE_NX = 0x8000000000000000,
} pte_flags_t;

    #define NR_PTE_ENTRIES (PAGE_SIZE / sizeof(pte_t))

    #define PA_FRAME(x) ((x) & 0xfffffffffffff000)

    #define PAGE_OFF(x) ((x) & 0xfff)
    #define VA_P1_INDEX(x) ((x >> 12) & 0o777)
    #define VA_P2_INDEX(x) ((x >> 21) & 0o777)
    #define VA_P3_INDEX(x) ((x >> 30) & 0o777)
    #define VA_P4_INDEX(x) ((x >> 39) & 0o777)

typedef enum page_table_level {
    PAGE_TABLE_P1,
    PAGE_TABLE_P2,
    PAGE_TABLE_P3,
    PAGE_TABLE_P4,
} page_table_level_t;

    #define PAGE_ORDER 12
    #define HUGE_PAGE_ORDER (12 + 9)
    #define GIANT_PAGE_ORDER (12 + 9 + 9)

    #define PAGE_SIZE (1 << PAGE_ORDER)
    #define HUGE_PAGE_SIZE (1 << HUGE_PAGE_ORDER)
    #define GIANT_PAGE_SIZE (1 << GIANT_PAGE_ORDER)

    #define PAGE_ROUND_DOWN(a) (((a) >> PAGE_ORDER) << PAGE_ORDER)
    #define PAGE_ROUND_UP(a) PAGE_ROUND_DOWN((a) + PAGE_SIZE - 1)

#endif

#endif // PAGE_H
