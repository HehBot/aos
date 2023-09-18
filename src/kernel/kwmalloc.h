#ifndef KWMALLOC_H
#define KWMALLOC_H

#include <stddef.h>

// water level allocator for kernel
// guaranteed to be at physical address pa=va-KERN_BASE
void kwmallocinit(void);
void* kwmalloc(size_t sz);

#endif // KWMALLOC_H
