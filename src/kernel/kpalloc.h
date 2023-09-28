#ifndef KPALLOCINIT_H
#define KPALLOCINIT_H

#include <stddef.h>
#include <stdint.h>

// allocate pages for kernel use

void init_kpalloc(void);
void* kpalloc(size_t n);
int kpfree(void* ptr);

void test_kpalloc(void);

#endif // KPALLOCINIT_H
