#ifndef MP_H
#define MP_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t acpi_proc_id;
    uint8_t lapic_id;
} cpu_t;
typedef struct {
    uint32_t reg;
    uint32_t : 32;
    uint32_t : 32;
    uint32_t : 32;
    uint32_t data;
} ioapic_t;

#define MAX_CPUS 32
extern cpu_t cpus[MAX_CPUS];
extern size_t nr_cpus;

extern uint32_t volatile* lapic;

extern ioapic_t volatile* ioapic;
extern uint8_t ioapic_id;

#endif // MP_H
