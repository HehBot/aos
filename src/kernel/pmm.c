// Physical memory manager

#include "kmalloc.h"
#include "multiboot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <x86.h>

struct section {
    uintptr_t addr, ul;
    // 0 -> free, 1->occupied
    uint32_t* bitmap;
    size_t bitmap_size;
    struct section* next;
} __attribute__((packed));
typedef struct section section_t;

static section_t* sections;

// tell physical memory manager about availabe regions of RAM
static void pmm_add_physical(uintptr_t addr, uint32_t len)
{
    section_t* z = kmalloc(sizeof(*z));
    z->addr = (addr >> PAGE_ORDER) << PAGE_ORDER;
    if (z->addr < addr)
        z->addr += (1 << PAGE_ORDER);
    z->ul = ((addr + len) >> PAGE_ORDER) << PAGE_ORDER;
    z->bitmap_size = (z->ul - z->addr) >> (PAGE_ORDER + 5);
    if (z->bitmap_size << (PAGE_ORDER + 5) != z->ul - z->addr)
        z->bitmap_size += 1;
    z->bitmap = kmalloc(sizeof(z->bitmap[0]) * (z->bitmap_size));
    if (sections == NULL)
        sections = z->next = z;
    else {
        z->next = sections->next;
        sections->next = z;
    }
}
void pmm_init(multiboot_info_t mboot_info)
{
    for (size_t i = 0; i < mboot_info.mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mboot_info.mmap_addr + KERN_BASE + i);
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
            pmm_add_physical(mmmt->addr_low, mmmt->len_low);
    }
}

#define INDEX(x) ((x) >> (PAGE_ORDER + 5))
#define LPG_IW INDEX(LPAGE_SIZE)

// set frame containing this physical address to occupied
bool pmm_reserve_frame(uintptr_t addr)
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
// set large frame containing this physical address to occupied
bool pmm_reserve_large_frame(uintptr_t addr)
{
    addr = LPG_ROUND_DOWN(addr);
    section_t* p = sections;
    do {
        uint32_t* bm = p->bitmap;
        if (p->addr <= addr && p->ul > addr) {
            size_t start_index = INDEX(addr - p->addr);
            for (size_t i = start_index; i < start_index + LPG_IW; ++i)
                if (bm[i])
                    return false;
            for (size_t i = start_index; i < start_index + LPG_IW; ++i)
                bm[i] = 0xffffffff;
            return true;
        }
        p = p->next;
    } while (p != sections);
    return false;
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
    //     PANIC("Out of frames");
    return 0;
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
    //     PANIC("Out of frames");
    return 0;
}
bool pmm_free_frame(uintptr_t addr)
{
    section_t* p = sections;
    do {
        uint32_t* bm = p->bitmap;
        if (p->addr <= addr && p->ul > addr) {
            size_t a = (addr - p->addr) >> PAGE_ORDER;
            uint32_t h = ((bm[a >> 5]) >> (a % 32)) & 1;
            if (!h)
                return false;
            bm[a >> 5] ^= (1 << (a % 32));
            // optimisation to prevent having to search from start next time
            //             sections = p;
            return true;
        }
        p = p->next;
    } while (p != sections);
    return false;
}
bool pmm_free_large_frame(uintptr_t addr)
{
    addr = LPG_ROUND_DOWN(addr);
    section_t* p = sections;
    do {
        uint32_t* bm = p->bitmap;
        if (p->addr <= addr && p->ul > addr) {
            if (p->ul < addr + LPAGE_SIZE)
                return false;
            size_t base = INDEX(addr - p->addr);
            for (size_t i = base; i < base + LPG_IW; ++i)
                if (!bm[i])
                    return false;
            for (size_t i = base; i < base + LPG_IW; ++i)
                bm[i] = 0;
            // optimisation to prevent having to search from start next time
            //             sections = p;
            return true;
        }
        p = p->next;
    } while (p != sections);
    return false;
}
