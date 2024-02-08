#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>

typedef struct {
    uintptr_t lapic_addr;
    uintptr_t ioapic_addr;
    uint8_t ioapic_id;
} acpi_info_t;

acpi_info_t init_acpi(void const* rsdp);

#endif // ACPI_H
