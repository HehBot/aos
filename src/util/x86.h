#ifndef X86_H
#define X86_H

#define KERN_BASE 0xc0000000

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

// Paging
typedef uint32_t pde_t;
typedef uint32_t pte_t;

#define PAGE_SIZE 0x1000
#define LPAGE_SIZE 0x400000
#define PAGE_ORDER 12
#define LPAGE_ORDER 22

#define PG_ROUND_DOWN(a) (((a) >> PAGE_ORDER) << PAGE_ORDER)
#define LPG_ROUND_DOWN(a) (((a) >> LPAGE_ORDER) << LPAGE_ORDER)
#define LPG_ROUND_UP(a) LPG_ROUND_DOWN(a + LPAGE_SIZE - 1)

#define PTE_P 0x001
#define PTE_W 0x002
#define PTE_U 0x004
#define PTE_LP 0x080

typedef struct page_table {
    pte_t pages[1024];
} page_table_t;

typedef struct page_directory {
    page_table_t* tables[1024];
    pde_t tables_physical[1024];
    uintptr_t physical_addr;
} page_directory_t;

#endif // X86_H
