#ifndef MM_H
#define MM_H

#include <stdint.h>

void switch_page_directory(size_t d);
size_t alloc_page_directory(void);
void free_page_directory(size_t d);

int map_page(uintptr_t pa, uintptr_t va, uint8_t flags);
int remap_page(uintptr_t pa, uintptr_t va, uint8_t flags);
int unmap_page(uintptr_t va);

void init_mm(void);

#endif // MM_H
