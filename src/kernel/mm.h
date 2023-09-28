#ifndef MM_H
#define MM_H

#include <cpu/x86.h>
#include <stdint.h>

void switch_page_directory(page_directory_t* d);
int map_page(uintptr_t pa, uintptr_t va, uint8_t flags);
int remap_page(uintptr_t pa, uintptr_t va, uint8_t flags);
int unmap_page(uintptr_t va);

void init_mm(void);

#endif // MM_H
