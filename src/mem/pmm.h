#ifndef PMM_H
#define PMM_H

#include <stdint.h>

struct multiboot_mmap_entry;

void init_pmm(struct multiboot_mmap_entry const* mmap_entries, size_t nr_entries);
int pmm_reserve_frame(uintptr_t phys_addr);
int pmm_reserve_large_frame(uintptr_t phys_addr);
uintptr_t pmm_get_frame();
uintptr_t pmm_get_large_frame();
int pmm_free_frame(uintptr_t phys_addr);
int pmm_free_large_frame(uintptr_t phys_addr);

#endif // PMM_H
