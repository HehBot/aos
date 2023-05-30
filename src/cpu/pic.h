#ifndef PIC_H
#define PIC_H

void PIC_send_EOI(unsigned char irq);
void PIC_remap(unsigned char offset1, unsigned char offset2);
void PIC_set_mask(unsigned short int m);

#endif // PIC_H
