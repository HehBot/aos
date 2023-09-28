#ifndef KWMALLOC_H
#define KWMALLOC_H

#include <stddef.h>

// water level allocator for kernel
void* kwmalloc(size_t sz);

#endif // KWMALLOC_H
