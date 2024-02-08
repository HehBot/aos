#ifndef IOAPIC_H
#define IOAPIC_H

#include <stdint.h>

void init_ioapic(uintptr_t addr, uint8_t id);
void ioapic_enable(uint8_t irq, uint8_t cpu_lapic_id);

#endif // IOAPIC_H
