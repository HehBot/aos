#ifndef PMM_H
#define PMM_H

#include <stdint.h>

typedef struct multiboot_info multiboot_info_t;

void init_pmm(multiboot_info_t const* mboot_info);
int pmm_reserve_frame(uintptr_t phys_addr);
int pmm_reserve_large_frame(uintptr_t phys_addr);
uintptr_t pmm_get_frame();
uintptr_t pmm_get_large_frame();
int pmm_free_frame(uintptr_t phys_addr);
int pmm_free_large_frame(uintptr_t phys_addr);

#endif // PMM_H
