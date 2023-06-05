#ifndef PMM_H
#define PMM_H

#include <stdbool.h>
#include <stdint.h>

void pmm_add_physical(uintptr_t addr, uint32_t len);
bool pmm_reserve_page(uintptr_t addr);
uintptr_t pmm_get_page();
bool pmm_free_page(uintptr_t addr);

#endif // PMM_H
