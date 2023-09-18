#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void* memcpy(void* dest, void const* src, size_t n);
void* memmove(void* dest, void const* src, size_t n);
void* memset(void* str, int c, size_t n);

char* strcpy(char* dest, char const* src);
size_t strlen(char const* s);
int strcmp(char const* s1, char const* s2);

#endif // STRING_H
