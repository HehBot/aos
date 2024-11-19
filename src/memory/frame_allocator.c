#include "frame_allocator.h"
#include "kalloc.h"
#include "page.h"

#include <cpu/x86.h>
#include <multiboot2.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct section {
    phys_addr_t first_frame, last_frame;
    size_t* bitmap; // 0 -> free, 1->occupied
    size_t nr_entries;
    struct section* next;
} section_t;

#define LOG_ENTRY_BITWIDTH 6
#define ENTRY_BITWIDTH (8 * sizeof(((section_t*)0)->bitmap[0]))

static section_t* sections = NULL;

/*
 * tell physical memory manager about available regions of RAM
 */
static void add_physical_memory_region(phys_addr_t addr, uint64_t len)
{
    section_t* z = kwmalloc(sizeof(*z));
    _Static_assert((1 << (LOG_ENTRY_BITWIDTH)) == ENTRY_BITWIDTH, "Bad bitmap entry width");

    z->first_frame = PAGE_ROUND_UP(addr);
    z->last_frame = PAGE_ROUND_DOWN(addr + len - PAGE_SIZE);
    size_t nr_frames = (z->last_frame - z->first_frame + PAGE_SIZE) >> PAGE_ORDER;

    z->nr_entries = (nr_frames + ENTRY_BITWIDTH - 1) >> LOG_ENTRY_BITWIDTH;
    z->bitmap = kwmalloc(sizeof(z->bitmap[0]) * (z->nr_entries));
    memset(z->bitmap, 0, sizeof(z->bitmap[0]) * (z->nr_entries));

    if (sections == NULL)
        sections = z->next = z;
    else {
        z->next = sections->next;
        sections->next = z;
    }
}

void init_frame_allocator(struct multiboot_tag_mmap const* mmap_info)
{
    struct multiboot_mmap_entry const* mmap_entries = &mmap_info->entries[0];
    size_t nr_entries = (mmap_info->size - sizeof(*mmap_info)) / sizeof(mmap_info->entries[0]);

    for (size_t i = 0; i < nr_entries; ++i)
        if (mmap_entries[i].type == MULTIBOOT_MEMORY_AVAILABLE)
            add_physical_memory_region(mmap_entries[i].addr, mmap_entries[i].len);
}

/*
 * set frame containing this physical address to occupied
 */
int frame_allocator_reserve_frame(phys_addr_t addr)
{
    section_t* p = sections;
    phys_addr_t frame = PAGE_ROUND_DOWN(addr);
    do {
        if (p->first_frame <= frame && p->last_frame >= frame) {
            size_t bit_index = (frame - p->first_frame) >> PAGE_ORDER;

            size_t entry_index = (bit_index >> LOG_ENTRY_BITWIDTH);
            uint64_t* entry = &p->bitmap[entry_index];

            /*
             * bit_index_in_entry = (bit_index % ENTRY_BITWIDTH);
             */
            size_t bit_index_in_entry = (bit_index & (ENTRY_BITWIDTH - 1));

            uint8_t bit = ((*entry) >> bit_index_in_entry) & 1;
            if (bit)
                return FRAME_ALLOCATOR_ERROR_ALREADY_TAKEN;

            *entry |= (((uint64_t)1) << bit_index_in_entry);
            return FRAME_ALLOCATOR_OK;
        }
        p = p->next;
    } while (p != sections);
    return FRAME_ALLOCATOR_ERROR_NO_SUCH_FRAME;
}

/*
 * set large frame containing this physical address to occupied
 */
// int frame_allocator_reserve_large_frame(phys_addr_t addr)
// {
//     addr = LPG_ROUND_DOWN(addr);
//     section_t* p = sections;
//     do {
//         uint32_t* bm = p->bitmap;
//         if (p->addr <= addr && p->ul > addr) {
//             size_t start_index = INDEX(addr - p->addr);
//             for (size_t i = start_index; i < start_index + LPG_IW; ++i)
//                 if (bm[i])
//                     return 0;
//             for (size_t i = start_index; i < start_index + LPG_IW; ++i)
//                 bm[i] = 0xffffffff;
//             return 1;
//         }
//         p = p->next;
//     } while (p != sections);
//     return 0;
// }

/*
 * free frame
 */
phys_addr_t frame_allocator_get_frame()
{
    section_t* p = sections;
    do {
        uint64_t* bitmap = p->bitmap;
        for (size_t entry_index = 0; entry_index < p->nr_entries; ++entry_index) {
            if ((~bitmap[entry_index]) != 0) {
                size_t bit_index_in_entry = 0;
                uint64_t entry = bitmap[entry_index];
                while (entry & 1) {
                    ++bit_index_in_entry;
                    entry >>= 1;
                }

                size_t bit_index = ((entry_index << LOG_ENTRY_BITWIDTH) + bit_index_in_entry);

                phys_addr_t frame = p->first_frame + (bit_index << PAGE_ORDER);
                if (frame > p->last_frame)
                    continue;

                bitmap[entry_index] |= (((uint64_t)1) << bit_index_in_entry);

                return frame;
            }
        }
        p = p->next;
    } while (p != sections);
    return FRAME_ALLOCATOR_ERROR_NO_FRAME_AVAILABLE;
}

/*
 * get large frame
 */
// phys_addr_t frame_allocator_get_large_frame()
// {
//     section_t* p = sections;
//     do {
//         uint32_t* bm = p->bitmap;
//         size_t base = INDEX(LPG_ROUND_UP(p->addr) - p->addr);
//         for (; base + LPG_IW < p->bitmap_size; base += LPG_IW) {
//             size_t i = base;
//             for (; i < base + LPG_IW; ++i)
//                 if (bm[i])
//                     break;
//             if (i == base + LPG_IW) {
//                 for (size_t j = base; j < base + LPG_IW; ++j)
//                     bm[j] = 0xffffffff;
//                 // optimisation to prevent having to search from start next time
//                 //                 sections = p;
//                 return p->addr + (base << (PAGE_ORDER + 5));
//             }
//         }
//         p = p->next;
//     } while (p != sections);
//     PANIC("Out of large frames!");
// }

/*
 * free large frame
 */
int frame_allocator_free_frame(phys_addr_t addr)
{
    section_t* p = sections;
    phys_addr_t frame = PAGE_ROUND_DOWN(addr);
    do {
        if (p->first_frame <= frame && p->last_frame >= frame) {
            size_t bit_index = (frame - p->first_frame) >> PAGE_ORDER;

            size_t entry_index = (bit_index >> LOG_ENTRY_BITWIDTH);
            uint64_t* entry = &p->bitmap[entry_index];

            /*
             * bit_index_in_entry = (bit_index % ENTRY_BITWIDTH);
             */
            size_t bit_index_in_entry = (bit_index & (ENTRY_BITWIDTH - 1));

            uint8_t bit = ((*entry) >> bit_index_in_entry) & 1;
            if (!bit)
                return FRAME_ALLOCATOR_ERROR_ALREADY_FREED;

            *entry ^= (((uint64_t)1) << bit_index_in_entry);
            return 1;
        }
        p = p->next;
    } while (p != sections);
    return FRAME_ALLOCATOR_OK;
}

// int frame_allocator_free_large_frame(phys_addr_t addr)
// {
//     addr = LPG_ROUND_DOWN(addr);
//     section_t* p = sections;
//     do {
//         uint32_t* bm = p->bitmap;
//         if (p->addr <= addr && p->ul > addr) {
//             if (p->ul < addr + LPAGE_SIZE)
//                 return 0;
//             size_t base = INDEX(addr - p->addr);
//             for (size_t i = base; i < base + LPG_IW; ++i)
//                 if (!bm[i])
//                     return 0;
//             for (size_t i = base; i < base + LPG_IW; ++i)
//                 bm[i] = 0;
//             // optimisation to prevent having to search from start next time
//             //             sections = p;
//             return 1;
//         }
//         p = p->next;
//     } while (p != sections);
//     return 0;
// }
