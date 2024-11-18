#ifndef KPALLOCINIT_H
#define KPALLOCINIT_H

#include <mem/mm.h>
#include <stddef.h>
#include <stdint.h>

// allocate pages for kernel use

void init_kpalloc(virt_addr_t heap_start, size_t init_heap_sz);
void* kpalloc(size_t n);
int kpfree(void* ptr);

void test_kpalloc(void);

#endif // KPALLOCINIT_H
