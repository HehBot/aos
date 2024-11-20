#ifndef KALLOC_H
#define KALLOC_H

#include "page.h"
#include "util/liballoc/liballoc.h"

#include <stddef.h>
#include <stdint.h>

// water level allocator for kernel
void* kwmalloc(size_t sz);

// allocate pages for kernel use
void init_kpalloc(virt_addr_t heap_start);
virt_addr_t kpalloc(size_t n);
int kpfree(virt_addr_t start_page);

void test_kpalloc(void);

#endif // KALLOC_H
