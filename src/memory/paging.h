#ifndef PAGING_H
#define PAGING_H

#include "page.h"

#include <stddef.h>
#include <stdint.h>

// void map(uintptr_t pa, void* va, size_t len, uint8_t flags);
// void remap(uintptr_t pa, void* va, size_t len, uint8_t flags);
// void unmap(void* va, size_t len);

// void* map_phy(uintptr_t pa, size_t len, uint8_t flags);

void init_paging();

int map_to_with_table_flags(virt_addr_t page, phys_addr_t frame, pte_flags_t flags, pte_flags_t parent_table_flags);
int map_to(virt_addr_t page, phys_addr_t frame, pte_flags_t flags);

static inline phys_addr_t phys_addr_of_kernel_static(virt_addr_t v)
{
    extern void* KERN_BASE;
    virt_addr_t kb = &KERN_BASE;
    return (v - kb);
}
static inline virt_addr_t kernel_static_from_phys_addr(phys_addr_t p)
{
    extern void* KERN_BASE;
    virt_addr_t kb = &KERN_BASE;
    return (kb + p);
}

#endif // PAGING_H
