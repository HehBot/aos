#ifndef PAGE_H
#define PAGE_H

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
#define PTE_HP 0x080
#define PTE_FRAME(x) ((x) & 0xfffff000)
#define PTE_FLAGS(x) ((x) & 0xfff)

#define PA_FRAME(x) ((x) & 0xfffff000)
#define PA_OFF(x) ((x) & 0xfff)

#endif // PAGE_H
