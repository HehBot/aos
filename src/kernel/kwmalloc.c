#include <stddef.h>
#include <stdint.h>

#define WHEAP_SIZE 0x10000
static uint8_t wheap[WHEAP_SIZE];

static uint8_t* base = (void*)wheap;
static uintptr_t top = (uintptr_t)&wheap[WHEAP_SIZE];

void* kwmalloc(size_t sz)
{
    if ((uintptr_t)&base[sz] > top)
        return NULL;
    void* ans = base;
    base += sz;
    return ans;
}
