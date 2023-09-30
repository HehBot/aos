#ifndef PIC_H
#define PIC_H

#include <stdint.h>

void pic_send_eoi(uint8_t irq);
void pic_remap(uint8_t offset1, uint8_t offset2);
void pic_set_mask(uint16_t m);

#endif // PIC_H
