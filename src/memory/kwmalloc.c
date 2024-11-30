#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>

#define WHEAP_SIZE 0x2000
static uint8_t wheap[WHEAP_SIZE];

static uint8_t* base = (void*)wheap;
static uintptr_t top = (uintptr_t)&wheap[WHEAP_SIZE];

void* kwmalloc(size_t sz)
{
    ASSERT((uintptr_t)&base[sz] <= top);
    void* ans = base;
    base += sz;
    return ans;
}
