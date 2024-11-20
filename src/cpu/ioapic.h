#ifndef IOAPIC_H
#define IOAPIC_H

#include <memory/page.h>
#include <stdint.h>

void init_ioapic(virt_addr_t* mapping_addr_ptr, phys_addr_t ioapic_addr, uint8_t id);
void ioapic_enable(uint8_t irq, uint8_t cpu_lapic_id);

#endif // IOAPIC_H
