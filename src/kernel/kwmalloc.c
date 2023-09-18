#include <stddef.h>
#include <stdint.h>

static uint8_t* base;
static uintptr_t top;

void kwmallocinit(void)
{
#define KWHEAP_SIZE 0x10000
    static uint8_t kwheap[KWHEAP_SIZE];
    base = kwheap;
    top = (uintptr_t)(kwheap + KWHEAP_SIZE);
#undef KWHEAP_SIZE
}

void* kwmalloc(size_t sz)
{
    if ((uintptr_t)&base[sz] > top)
        return NULL;
    void* ans = base;
    base += sz;
    return ans;
}
