#include "string.h"

void* memcpy(void* dest, void const* src, size_t n)
{
    char* d = (char*)dest;
    char const* s = (char*)src;
    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];
    return dest;
}
void* memmove(void* dest, void const* src, size_t n)
{
    char* d = (char*)dest;
    char const* s = (char*)src;
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
    char* s = (char*)str;
    char C = (char)c;
    for (size_t i = 0; i < n; ++i)
        s[i] = C;
    return str;
}
