#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <x86.h>

#define BLOCK_ORDER 4

struct block_hdr {
    size_t size;
    struct block_hdr* prev;
    struct block_hdr* next;
};
typedef struct block_hdr block_hdr_t;

static block_hdr_t* baseptr;
static uintptr_t kheap_end;

void kmallocinit(void)
{
#define KHEAP_SIZE 0x60000
    static __attribute__((__aligned__(PAGE_SIZE))) uint8_t kheap[KHEAP_SIZE];
    kheap_end = (uintptr_t)(kheap + KHEAP_SIZE);
#undef KHEAP_SIZE
    baseptr = (void*)kheap;
    baseptr->size = 0;
    baseptr->prev = baseptr;
    baseptr->next = baseptr;
}
void* kmalloc(size_t sz)
{
    if (sz & ((1 << BLOCK_ORDER) - 1))
        sz = (1 + (sz >> BLOCK_ORDER)) << BLOCK_ORDER;

    block_hdr_t* p = baseptr;
    for (; p->next != baseptr; p = p->next) {
        if ((uintptr_t)(p->next) - (uintptr_t)p > sz + sizeof(block_hdr_t))
            break;
    }
    if (p->next != baseptr || kheap_end - (uintptr_t)p > sz + sizeof(block_hdr_t)) {
        block_hdr_t* new = (block_hdr_t*)((uintptr_t)(p + 1) + p->size);
        new->size = sz;
        new->next = p->next;
        new->prev = p;
        p->next->prev = new;
        p->next = new;
        return (new + 1);
    }
    return NULL;
}
void kfree(void* ptr)
{
    block_hdr_t* p = ptr;
    p = p - 1;
    p->prev->next = p->next;
    p->next->prev = p->prev;
    p->size = 0;
}
