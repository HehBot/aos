#ifndef PORT_H
#define PORT_H

#include <stdint.h>

#define PORT_SCREEN 0x3d4
#define PORT_SCREEN_CTRL (PORT_SCREEN)
#define PORT_SCREEN_DATA (PORT_SCREEN + 1)

#define PORT_PIC1 0x20
#define PORT_PIC1_CTRL (PORT_PIC1)
#define PORT_PIC1_DATA (PORT_PIC1 + 1)

#define PORT_PIC2 0xa0
#define PORT_PIC2_CTRL (PORT_PIC2)
#define PORT_PIC2_DATA (PORT_PIC2 + 1)

#define PORT_PIT_CTRL 0x43
#define PORT_PIT_CHANNEL0_DATA 0x40
#define PORT_PIT_CHANNEL1_DATA 0x41
#define PORT_PIT_CHANNEL2_DATA 0x42

#define PORT_KEYBOARD_DATA 0x60

uint8_t port_read_byte(uint16_t port);
uint16_t port_read_word(uint16_t port);

void port_write_byte(uint16_t port, uint8_t data);
void port_write_word(uint16_t port, uint16_t data);

#endif // PORT_H
