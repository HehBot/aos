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
uint8_t memsum(void const* addr, size_t n)
{
    uint8_t const* s = addr;
    uint8_t sum = 0;
    for (size_t i = 0; i < n; ++i)
        sum += s[i];
    return sum;
}

char* strcpy(char* dest, char const* src)
{
    while ((*(dest++) = *(src++)))
        ;
    return dest;
}
size_t strlen(char const* s)
{
    size_t i = 0;
    while (*(s++))
        i++;
    return i;
}
int strcmp(char const* s1, char const* s2)
{
    size_t i = 0;
    while (s1[i] == s2[i] && s1[i] && s2[i])
        ++i;
    return (s1[i] - s2[i]);
}

int memcmp(void const* s1, void const* s2, size_t n)
{
    uint8_t* m1 = (uint8_t*)s1;
    uint8_t* m2 = (uint8_t*)s2;
    for (size_t i = 0; i < n; ++i)
        if (m1[i] != m2[i])
            return ((int)m1[i]) - ((int)m2[i]);
    return 0;
}
