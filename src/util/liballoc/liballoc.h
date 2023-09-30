#ifndef LIBALLOC_H
#define LIBALLOC_H

#include <stddef.h>

void* kmalloc(size_t);
void* krealloc(void*, size_t);
void* kcalloc(size_t, size_t);
void kfree(void*);

#endif // LIBALLOC_H
