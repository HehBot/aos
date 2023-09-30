#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <cpu/x86.h>
#include <stdint.h>

typedef void (*isr_t)(cpu_state_t*);
void register_irq_handler(size_t n, isr_t handler);

#endif // INTERRUPT_H
