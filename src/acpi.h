#ifndef ACPI_H
#define ACPI_H

#include <memory/page.h>
#include <stdint.h>

typedef struct {
    phys_addr_t lapic_addr;
    phys_addr_t ioapic_addr;
    uint8_t ioapic_id;
} acpi_info_t;

struct multiboot_tag_old_acpi;
struct multiboot_tag_new_acpi;
acpi_info_t parse_acpi(struct multiboot_tag_old_acpi const* old_acpi_tag, struct multiboot_tag_new_acpi const* new_acpi_tag, virt_addr_t* mapping_addr_ptr);

#endif // ACPI_H
