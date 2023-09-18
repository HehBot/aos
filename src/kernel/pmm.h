#ifndef PMM_H
#define PMM_H

#include "multiboot.h"

#include <stdbool.h>
#include <stdint.h>

void pmm_init(multiboot_info_t mboot_info);
bool pmm_reserve_frame(uintptr_t phys_addr);
bool pmm_reserve_large_frame(uintptr_t phys_addr);
uintptr_t pmm_get_frame();
uintptr_t pmm_get_large_frame();
bool pmm_free_frame(uintptr_t phys_addr);
bool pmm_free_large_frame(uintptr_t phys_addr);

#endif // PMM_H
