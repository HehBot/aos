#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void* memcpy(void* dest, void const* src, size_t n);
void* memmove(void* dest, void const* src, size_t n);
void* memset(void* str, int c, size_t n);
uint8_t memsum(void const* addr, size_t n);

char* strcpy(char* dest, char const* src);
size_t strlen(char const* s);
int strcmp(char const* s1, char const* s2);

int memcmp(void const* s1, void const* s2, size_t n);

#endif // STRING_H
