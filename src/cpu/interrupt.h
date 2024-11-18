#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

void init_idt(uint16_t kernel_cs);

#endif // INTERRUPT_H
