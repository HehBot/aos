// Physical memory manager

#include "mm_mem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAGE_ORDER 12
#define LARGE_PAGE_ORDER

struct section {
    uintptr_t addr, ul;
    // 0 -> free, 1->occupied
    uint32_t* bitmap;
    size_t bitmap_size;
    struct section* next;
} __attribute__((packed));
typedef struct section section_t;

static section_t* sections;
void pmm_add_physical(uintptr_t addr, uint32_t len)
{
    section_t* z = mm_malloc(sizeof(*z));
    z->addr = (addr >> PAGE_ORDER) << PAGE_ORDER;
    if (z->addr < addr)
        z->addr += (1 << PAGE_ORDER);
    z->ul = ((addr + len) >> PAGE_ORDER) << PAGE_ORDER;
    z->bitmap_size = (z->ul - z->addr) >> (PAGE_ORDER + 5);
    if (z->bitmap_size << (PAGE_ORDER + 5) != z->ul - z->addr)
        z->bitmap_size += 1;
    z->bitmap = mm_malloc(sizeof(z->bitmap[0]) * (z->bitmap_size));
    if (sections == NULL)
        sections = z->next = z;
    else {
        z->next = sections->next;
        sections->next = z;
    }
}
// sets page containing this physical address to occupied
bool pmm_reserve_page(uintptr_t addr)
{
    section_t* p = sections;
    do {
        if (p->addr <= addr && p->ul > addr) {
            size_t a = (addr - p->addr) >> PAGE_ORDER;
            uint32_t h = ((p->bitmap[a >> 5]) >> (a % 32)) & 1;
            if (h)
                return false;
            p->bitmap[a >> 5] |= (1 << (a % 32));
            return true;
        }
        p = p->next;
    } while (p != sections);
    return false;
}
uintptr_t pmm_get_page()
{
    section_t* p = sections;
    do {
        for (size_t z = 0; z < p->bitmap_size; ++z) {
            if ((~(p->bitmap[z])) & 0xffffffff) {
                size_t i = 0;
                uint32_t h = p->bitmap[z];
                while (h & 1) {
                    ++i;
                    h >>= 1;
                }
                uintptr_t ans = p->addr + (((z << 5) + i) << PAGE_ORDER);
                if (ans < p->ul) {
                    p->bitmap[z] |= (1 << i);
                    // optimisation to prevent having to search from start next time
                    sections = p;
                    return ans;
                }
            }
        }
        p = p->next;
    } while (p != sections);
    return 0;
}
// uintptr_t pmm_get_large_page()
// {
// }
bool pmm_free_page(uintptr_t addr)
{
    section_t* p = sections;
    do {
        if (p->addr <= addr && p->ul > addr) {
            size_t a = (addr - p->addr) >> PAGE_ORDER;
            uint32_t h = ((p->bitmap[a >> 5]) >> (a % 32)) & 1;
            if (!h)
                return false;
            p->bitmap[a >> 5] ^= (1 << (a % 32));
            // optimisation to prevent having to search from start next time
            sections = p;
            return true;
        }
        p = p->next;
    } while (p != sections);
    return false;
}
// bool pmm_free_large_page(uintptr_t addr)
// {
// }
