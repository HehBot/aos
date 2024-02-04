#ifndef MM_H
#define MM_H

#include <cpu/page.h>
#include <stddef.h>
#include <stdint.h>

void switch_page_directory(size_t d);
size_t alloc_page_directory(void);
void free_page_directory(size_t d);

void map(uintptr_t pa, void* va, size_t len, uint8_t flags);
void remap(uintptr_t pa, void* va, size_t len, uint8_t flags);
void unmap(void* va, size_t len);

void* map_phy(uintptr_t pa, size_t len, uint8_t flags);

void init_mm(void);

#endif // MM_H
