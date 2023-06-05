#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define BLOCK_ORDER 4

struct block_hdr {
    size_t size;
    struct block_hdr* prev;
    struct block_hdr* next;
};
typedef struct block_hdr block_hdr_t;

#define MEMPOOL_SIZE 16384
static uint8_t mempool[MEMPOOL_SIZE];
static uintptr_t kheap_end = (uintptr_t)&mempool[MEMPOOL_SIZE];
#undef MEMPOOL_SIZE
static block_hdr_t* baseptr = (block_hdr_t*)mempool;

void mm_minit()
{
    baseptr->size = 0;
    baseptr->prev = baseptr;
    baseptr->next = baseptr;
}
void* mm_malloc(size_t z)
{
    if (z & ((1 << BLOCK_ORDER) - 1))
        z = (1 + (z >> BLOCK_ORDER)) << BLOCK_ORDER;

    block_hdr_t* p = baseptr;
    for (; p->next != baseptr; p = p->next) {
        if ((uintptr_t)(p->next) - (uintptr_t)p > z + sizeof(block_hdr_t))
            break;
    }
    if (p->next != baseptr || kheap_end - (uintptr_t)p > z + sizeof(block_hdr_t)) {
        block_hdr_t* new = (block_hdr_t*)((uintptr_t)(p + 1) + p->size);
        new->size = z;
        new->next = p->next;
        new->prev = p;
        p->next->prev = new;
        p->next = new;
        return (new + 1);
    }
    return NULL;
}
void mm_free(void* ptr)
{
    block_hdr_t* p = ptr;
    p = p - 1;
    p->prev->next = p->next;
    p->next->prev = p->prev;
    p->size = 0;
}
