#ifndef PORT_H
#define PORT_H

#include <stddef.h>
#include <stdint.h>

static inline uint8_t port_read_byte(uint16_t port)
{
    uint8_t result;
    asm("in %%dx, %%al"
        : "=a"(result)
        : "d"(port));
    return result;
}
static inline uint16_t port_read_word(uint16_t port)
{
    uint16_t result;
    asm("in %%dx, %%ax"
        : "=a"(result)
        : "d"(port));
    return result;
}
static inline void port_read_byte_rep(uint16_t port, void* buf, size_t count)
{
    asm("cld; rep insb"
        : "=D"(buf), "=c"(count)
        : "d"(port), "0"(buf), "1"(count)
        : "memory");
}
static inline void port_read_word_rep(uint16_t port, void* buf, size_t count)
{
    asm("cld; rep insw"
        : "=D"(buf), "=c"(count)
        : "d"(port), "0"(buf), "1"(count)
        : "memory");
}

static inline void port_write_byte(uint16_t port, uint8_t data)
{
    asm("out %%al, %%dx" ::"a"(data), "d"(port));
}
static inline void port_write_word(uint16_t port, uint16_t data)
{
    asm("out %%ax, %%dx" ::"a"(data), "d"(port));
}
static inline void port_write_byte_rep(uint16_t port, void const* buf, size_t count)
{
    asm("cld; rep outsb"
        : "=S"(buf), "=c"(count)
        : "d"(port), "0"(buf), "1"(count)
        : "memory");
}
static inline void port_write_word_rep(uint16_t port, void const* buf, size_t count)
{
    asm("cld; rep outsw"
        : "=S"(buf), "=c"(count)
        : "d"(port), "0"(buf), "1"(count)
        : "memory");
}

#endif // PORT_H
