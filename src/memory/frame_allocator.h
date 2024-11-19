#ifndef FRAME_ALLOCATOR_H
#define FRAME_ALLOCATOR_H

#include "page.h"

#include <stdint.h>

struct multiboot_tag_mmap;

void init_frame_allocator(struct multiboot_tag_mmap const* mmap_info);

enum {
    FRAME_ALLOCATOR_OK = 0,
    FRAME_ALLOCATOR_ERROR_ALREADY_TAKEN,
    FRAME_ALLOCATOR_ERROR_NO_SUCH_FRAME,
    FRAME_ALLOCATOR_ERROR_ALREADY_FREED,
    FRAME_ALLOCATOR_ERROR_NO_FRAME_AVAILABLE,
};

noignore int frame_allocator_reserve_frame(phys_addr_t phys_addr);
phys_addr_t frame_allocator_get_frame();
noignore int frame_allocator_free_frame(phys_addr_t phys_addr);

// int frame_allocator_reserve_large_frame(uintptr_t phys_addr);
// uintptr_t frame_allocator_get_large_frame();
// int frame_allocator_free_large_frame(uintptr_t phys_addr);

#endif // FRAME_ALLOCATOR_H
