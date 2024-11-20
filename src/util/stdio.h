#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>

/*
            %[length]specifier

Supports following lengths
                d/i             o u x
    hh      char            unsigned char
    h       short int       unsigned short int
    l       long int        unsigned long int
    ll      long long int   unsigned long long int
Supports following specifiers
    %   literal '%'
    s   char const*
    c   char
    d/i int, base 10
    o   unsigned int, base 8
    p   pointers, base 16 [with 0x prefix]
    u   unsigned int, base 10
    x   unsigned int, base 16
*/

#include <stdarg.h>
int __attribute__((format(printf, 1, 2))) printf(char const* fmt, ...);
int __attribute__((format(printf, 3, 4))) snprintf(char* buf, size_t sz, char const* fmt, ...);
int vprintf(char const* fmt, va_list ap);

#endif // STDIO_H
