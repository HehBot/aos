#ifndef PIC_H
#define PIC_H

#include <stdint.h>

void PIC_send_EOI(uint8_t irq);
void PIC_remap(uint8_t offset1, uint8_t offset2);
void PIC_set_mask(uint16_t m);

#endif // PIC_H
