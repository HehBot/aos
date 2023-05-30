#include <stdint.h>

uint8_t port_read_byte(uint16_t port)
{
    uint8_t result;
    asm inline("in al, dx"
               : "=a"(result)
               : "d"(port));
    return result;
}
uint16_t port_read_word(uint16_t port)
{
    uint16_t result;
    asm inline("in ax, dx"
               : "=a"(result)
               : "d"(port));
    return result;
}

void port_write_byte(uint16_t port, uint8_t data)
{
    asm inline("out dx, al"
               :
               : "a"(data), "d"(port));
}
void port_write_word(uint16_t port, uint16_t data)
{
    asm inline("out dx, ax"
               :
               : "a"(data), "d"(port));
}
