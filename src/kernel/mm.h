#ifndef MM_H
#define MM_H

#include <stdint.h>

void mm_init(uint32_t* pgd, uint32_t* temp_pgt);
void mm_add_physical(uintptr_t phyaddr, uint32_t len);
void mm_reserve_physical_page(uintptr_t phyaddr);
void mm_reserve_page(uintptr_t vaddr);

#endif // MM_H
