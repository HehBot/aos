#include "mm_mem.h"
#include "pmm.h"

#include <stddef.h>
#include <stdint.h>

uint32_t* page_dir;
uint32_t* page_tables[1024];

void mm_init(uint32_t* pgd, uint32_t* temp_pgt)
{
    mm_minit();
    page_dir = pgd;
    page_tables[0] = temp_pgt;
}

void mm_add_physical(uintptr_t phyaddr, uint32_t len)
{
    pmm_add_physical(phyaddr, len);
}

void mm_reserve_physical_page(uintptr_t phyaddr)
{
    pmm_reserve_page(phyaddr);
}

void mm_reserve_page(uintptr_t)
{
}

// void* mm_get_page()
// {
// }
// void mm_free_page(void*)
// {
// }
