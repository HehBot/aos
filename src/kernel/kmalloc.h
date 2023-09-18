#ifndef MM_MEM_H
#define MM_MEM_H

#include <stddef.h>

void kmallocinit(void);
void* kmalloc(size_t sz);
void kfree(void* ptr);

#endif // MM_MEM_H
