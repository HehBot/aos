#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <cpu/x86.h>
#include <stddef.h>

void register_irq_handler(size_t n, isr_t handler);

#endif // INTERRUPT_H
