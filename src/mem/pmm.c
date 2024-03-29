// Physical memory manager

#include "kwmalloc.h"

#include <cpu/x86.h>
#include <multiboot2.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct section {
    uintptr_t addr, ul;
    // 0 -> free, 1->occupied
    uint32_t* bitmap;
    size_t bitmap_size;
    struct section* next;
} __attribute__((packed));
typedef struct section section_t;

static section_t* sections = NULL;

// tell physical memory manager about availabe regions of RAM
static void pmm_add_physical(uintptr_t addr, uint32_t len)
{
    section_t* z = kwmalloc(sizeof(*z));
    z->addr = (addr >> PAGE_ORDER) << PAGE_ORDER;
    if (z->addr < addr)
        z->addr += (1 << PAGE_ORDER);
    z->ul = ((addr + len) >> PAGE_ORDER) << PAGE_ORDER;
    z->bitmap_size = (z->ul - z->addr) >> (PAGE_ORDER + 5);
    if (z->bitmap_size << (PAGE_ORDER + 5) != z->ul - z->addr)
        z->bitmap_size += 1;
    z->bitmap = kwmalloc(sizeof(z->bitmap[0]) * (z->bitmap_size));
    memset(z->bitmap, 0, sizeof(z->bitmap[0]) * (z->bitmap_size));
    if (sections == NULL)
        sections = z->next = z;
    else {
        z->next = sections->next;
        sections->next = z;
    }
}
void init_pmm(struct multiboot_tag_mmap const* mmap_info)
{
    struct multiboot_mmap_entry const* mmap_entries = &mmap_info->entries[0];
    size_t nr_entries = (mmap_info->size - sizeof(*mmap_info)) / sizeof(mmap_info->entries[0]);

    for (size_t i = 0; i < nr_entries; i++) {
        if (mmap_entries[i].type == MULTIBOOT_MEMORY_AVAILABLE)
            pmm_add_physical(mmap_entries[i].addr_low, mmap_entries[i].len_low);
    }
}

#define INDEX(x) ((x) >> (PAGE_ORDER + 5))
#define LPG_IW INDEX(LPAGE_SIZE)

// set frame containing this physical address to occupied
int pmm_reserve_frame(uintptr_t addr)
{
    section_t* p = sections;
    do {
        if (p->addr <= addr && p->ul > addr) {
            size_t a = (addr - p->addr) >> PAGE_ORDER;
            uint32_t h = ((p->bitmap[a >> 5]) >> (a % 32)) & 1;
            if (h)
                return 0;
            p->bitmap[a >> 5] |= (1 << (a % 32));
            return 1;
        }
        p = p->next;
    } while (p != sections);
    return 0;
}
// set large frame containing this physical address to occupied
int pmm_reserve_large_frame(uintptr_t addr)
{
    addr = LPG_ROUND_DOWN(addr);
    section_t* p = sections;
    do {
        uint32_t* bm = p->bitmap;
        if (p->addr <= addr && p->ul > addr) {
            size_t start_index = INDEX(addr - p->addr);
            for (size_t i = start_index; i < start_index + LPG_IW; ++i)
                if (bm[i])
                    return 0;
            for (size_t i = start_index; i < start_index + LPG_IW; ++i)
                bm[i] = 0xffffffff;
            return 1;
        }
        p = p->next;
    } while (p != sections);
    return 0;
}
// get frame
uintptr_t pmm_get_frame()
{
    section_t* p = sections;
    do {
        uint32_t* bm = p->bitmap;
        for (size_t z = 0; z < p->bitmap_size; ++z) {
            if ((~(bm[z])) & 0xffffffff) {
                size_t i = 0;
                uint32_t h = bm[z];
                while (h & 1) {
                    ++i;
                    h >>= 1;
                }
                uintptr_t ans = p->addr + (((z << 5) + i) << PAGE_ORDER);
                if (ans < p->ul) {
                    bm[z] |= (1 << i);
                    // optimisation to prevent having to search from start next time
                    //                     sections = p;
                    return ans;
                }
            }
        }
        p = p->next;
    } while (p != sections);
    PANIC("Out of frames!");
}
// get large frame
uintptr_t pmm_get_large_frame()
{
    section_t* p = sections;
    do {
        uint32_t* bm = p->bitmap;
        size_t base = INDEX(LPG_ROUND_UP(p->addr) - p->addr);
        for (; base + LPG_IW < p->bitmap_size; base += LPG_IW) {
            size_t i = base;
            for (; i < base + LPG_IW; ++i)
                if (bm[i])
                    break;
            if (i == base + LPG_IW) {
                for (size_t j = base; j < base + LPG_IW; ++j)
                    bm[j] = 0xffffffff;
                // optimisation to prevent having to search from start next time
                //                 sections = p;
                return p->addr + (base << (PAGE_ORDER + 5));
            }
        }
        p = p->next;
    } while (p != sections);
    PANIC("Out of large frames!");
}
int pmm_free_frame(uintptr_t addr)
{
    section_t* p = sections;
    do {
        uint32_t* bm = p->bitmap;
        if (p->addr <= addr && p->ul > addr) {
            size_t a = (addr - p->addr) >> PAGE_ORDER;
            uint32_t h = ((bm[a >> 5]) >> (a % 32)) & 1;
            if (!h)
                return 0;
            bm[a >> 5] ^= (1 << (a % 32));
            // optimisation to prevent having to search from start next time
            //             sections = p;
            return 1;
        }
        p = p->next;
    } while (p != sections);
    return 0;
}
int pmm_free_large_frame(uintptr_t addr)
{
    addr = LPG_ROUND_DOWN(addr);
    section_t* p = sections;
    do {
        uint32_t* bm = p->bitmap;
        if (p->addr <= addr && p->ul > addr) {
            if (p->ul < addr + LPAGE_SIZE)
                return 0;
            size_t base = INDEX(addr - p->addr);
            for (size_t i = base; i < base + LPG_IW; ++i)
                if (!bm[i])
                    return 0;
            for (size_t i = base; i < base + LPG_IW; ++i)
                bm[i] = 0;
            // optimisation to prevent having to search from start next time
            //             sections = p;
            return 1;
        }
        p = p->next;
    } while (p != sections);
    return 0;
}
