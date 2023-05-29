#ifndef STRING_H
#define STRING_H

#define size_t unsigned int

void* memcpy(void* dest, void const* src, size_t n);
void* memmove(void* dest, void const* src, size_t n);
void* memset(void* str, int c, size_t n);

#endif // STRING_H
