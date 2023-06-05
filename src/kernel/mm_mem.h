#ifndef MM_MEM_H
#define MM_MEM_H

#include <stddef.h>

void mm_minit();
void* mm_malloc(size_t z);
void mm_free(void* ptr);

#endif // MM_MEM_H
