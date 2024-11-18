#include "acpi.h"

#include <cpu/ioapic.h>
#include <cpu/mp.h>
#include <mem/mm.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct rsdp {
    uint8_t sign[8];
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t version;
    uint32_t rsdt_pa;
} __attribute__((packed)) rsdp_t;

typedef struct acpi_sdt_header {
    uint8_t sign[4];
    uint32_t len;
    uint8_t version;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_version;
    uint32_t creator_id;
    uint32_t creator_version;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct rsdt {
    acpi_sdt_header_t header;
    uintptr_t other_sdt[0];
} __attribute__((packed)) rsdt_t;
typedef struct madt {
    acpi_sdt_header_t header;
    uintptr_t lapic_pa;
    uint32_t flags;
    uint8_t entries[0];
} __attribute__((packed)) madt_t;

// see https://wiki.osdev.org/MADT
static acpi_info_t parse_madt(madt_t const* madt)
{
    acpi_info_t info;

    info.lapic_addr = madt->lapic_pa;
    uint8_t const* p = madt->entries;
    while (p < (madt->entries + (madt->header.len - sizeof(*madt)))) {
        switch (p[0]) {
        case 0:
            // processor and lapic
            if (p[4] & 1) {
                cpus[nr_cpus].acpi_proc_id = p[2];
                cpus[nr_cpus].lapic_id = p[3];
                nr_cpus++;
            }
            goto end;
        case 1:
            // ioapic
            info.ioapic_addr = *(uintptr_t*)(&p[4]);
            info.ioapic_id = p[2];
            goto end;
        end:
        default:
            p += p[1];
            continue;
        }
    }

    return info;
}

acpi_info_t init_acpi(void const* __rsdp)
{
    rsdp_t const* rsdp = __rsdp;
    if (rsdp == NULL
        || memcmp(rsdp->sign, "RSD PTR ", 8) != 0
        || rsdp->version != 0
        || memsum(rsdp, sizeof(*rsdp)) != 0) {
        PANIC("Bad RSDP");
    }

    // uintptr_t table_pa = rsdp->rsdt_pa;
    // TODO wtf is the size here?? need unmap_pa
    rsdt_t* rsdt = NULL; // map_phy(table_pa, -1, 0);

    uint8_t sum;
    if (memcmp(rsdt->header.sign, "RSDT", 4) != 0 || (sum = memsum(rsdt, rsdt->header.len) != 0))
        PANIC("Bad RSDT");

    size_t nr_rsdt_entries = ((rsdt->header.len - sizeof(rsdt->header)) / sizeof(rsdt->other_sdt[0]));
    printf("[ACPI: ");
    if (nr_rsdt_entries > 0)
        printf("Detected tables");
    else
        PANIC("No ACPI tables found");

    acpi_info_t ret;

    for (size_t i = 0; i < nr_rsdt_entries; ++i) {
        uintptr_t table_pa = rsdt->other_sdt[i];
        // TODO wtf is the size here?? need unmap_pa
        acpi_sdt_header_t* h = NULL; // map_phy(table_pa, -1, 0);

        printf(" ");
        for (int j = 0; j < 4; ++j)
            printf("%c", h->sign[j]);
        if (memsum(h, h->len) == 0 && !memcmp(h->sign, "APIC", 4))
            ret = parse_madt((void*)h);
    }
    printf("]\n");

    return ret;
}
