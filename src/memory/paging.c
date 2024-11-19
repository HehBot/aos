#include "frame_allocator.h"
#include "kalloc.h"
#include "page.h"

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct page_table {
    pte_t pages[NR_PTE_ENTRIES];
} page_table_t;

void init_mm(void)
{
}
