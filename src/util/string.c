#include <stddef.h>
#include <stdint.h>

void* memcpy(void* dest, void const* src, size_t n)
{
    uint8_t* d = (uint8_t*)dest;
    uint8_t const* s = (uint8_t const*)src;
    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];
    return dest;
}
void* memmove(void* dest, void const* src, size_t n)
{
    uint8_t* d = (uint8_t*)dest;
    uint8_t const* s = (uint8_t const*)src;
    if (src > dest) {
        for (size_t i = 0; i < n; ++i)
            d[i] = s[i];
    } else if (src < dest) {
        for (size_t i = n - 1; i > 0; --i)
            d[i] = s[i];
        d[0] = s[0];
    }
    return dest;
}
void* memset(void* str, int c, size_t n)
{
    uint8_t* s = (uint8_t*)str;
    uint8_t C = (uint8_t)c;
    for (size_t i = 0; i < n; ++i)
        s[i] = C;
    return str;
}
