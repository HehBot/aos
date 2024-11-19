#ifndef KALLOC_H
#define KALLOC_H

#include "paging.h"
#include "util/liballoc/liballoc.h"

#include <stddef.h>
#include <stdint.h>

// water level allocator for kernel
void* kwmalloc(size_t sz);

// allocate pages for kernel use
void init_kpalloc(virt_addr_t heap_start, size_t init_heap_sz);
void* kpalloc(size_t n);
int kpfree(void* ptr);

void test_kpalloc(void);

#endif // KALLOC_H
