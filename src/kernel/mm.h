#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <x86.h>

void switch_page_directory(page_directory_t* new);
int map_page(page_directory_t* dir, uintptr_t pa, uintptr_t va, uint8_t flags);
int unmap_page(page_directory_t* dir, uintptr_t va);

#endif // MM_H
