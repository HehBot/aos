#ifndef MM_H
#define MM_H

#include <cpu/page.h>
#include <stddef.h>
#include <stdint.h>

void switch_page_directory(size_t d);
size_t alloc_page_directory(void);
void free_page_directory(size_t d);

// FIXME API is error-prone in usage
int map_page(uintptr_t pa, uintptr_t va, uint8_t flags);
int remap_page(uintptr_t pa, uintptr_t va, uint8_t flags);
int unmap_page(uintptr_t va);

void* map_pa(uintptr_t pa, size_t len, uint8_t flags);

void init_mm(void);

#endif // MM_H
