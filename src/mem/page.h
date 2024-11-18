#ifndef PAGE_H
#define PAGE_H

#ifdef __ASSEMBLER__

    #define PTE_P 0x001
    #define PTE_W 0x002
    #define PTE_HP 0x080

#else

    #include <stdint.h>

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

typedef uint64_t pte_t;
typedef enum pte_flags {
    PTE_P = 0x001,
    PTE_W = 0x002,
    PTE_U = 0x004,
    PTE_HP = 0x080,
    PTE_NX = 0x8000000000000000,
} pte_flags_t;

    #define PA_FRAME(x) ((x) & 0xfffffffffffff000)

    #define VA_PAGE_OFF(x) ((x) & 0xfff)
    #define VA_P1_INDEX(x) ((x >> 12) & 0o777)
    #define VA_P2_INDEX(x) ((x >> 21) & 0o777)
    #define VA_P3_INDEX(x) ((x >> 30) & 0o777)
    #define VA_P4_INDEX(x) ((x >> 39) & 0o777)

#endif

#endif // PAGE_H
