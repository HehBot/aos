#ifndef PORT_H
#define PORT_H

#define PORT_SCREEN_CTRL 0x3d4
#define PORT_SCREEN_DATA 0x3d5

unsigned char port_read_byte(unsigned short port);
unsigned short port_read_word(unsigned short port);

void port_write_byte(unsigned short port, unsigned char data);
void port_write_word(unsigned short port, unsigned short data);

#endif // PORT_H
