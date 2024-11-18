#ifndef MM_H
#define MM_H

#include <mem/page.h>
#include <stddef.h>
#include <stdint.h>

typedef uintptr_t phys_addr_t;
typedef void* virt_addr_t;

void switch_page_directory(size_t d);
size_t alloc_page_directory(void);
void free_page_directory(size_t d);

void map(uintptr_t pa, void* va, size_t len, uint8_t flags);
// void remap(uintptr_t pa, void* va, size_t len, uint8_t flags);
void unmap(void* va, size_t len);

void* map_phy(uintptr_t pa, size_t len, uint8_t flags);

void init_mm(void);

static inline phys_addr_t phys_addr_of_kernel_static(virt_addr_t v)
{
    extern void* KERN_BASE;
    virt_addr_t kb = &KERN_BASE;
    return (v - kb);
}
static inline virt_addr_t kernel_static_from_phys_addr(phys_addr_t p)
{
    extern void* KERN_BASE;
    virt_addr_t kb = &KERN_BASE;
    return (kb + p);
}

#endif // MM_H
