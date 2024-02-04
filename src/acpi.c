#include <mem/mm.h>
#include <mp.h>
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
static void parse_madt(madt_t const* madt)
{
    // FIXME max size as macro
    lapic = map_phy(madt->lapic_pa, 0x400, PTE_W);
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
            ioapic_id = p[2];
            ioapic = map_phy(*(uintptr_t*)(&p[4]), sizeof(*ioapic), PTE_W);
            goto end;
        end:
        default:
            p += p[1];
            continue;
        }
    }
}

void init_acpi(void const* __rsdp)
{
    rsdp_t const* rsdp = __rsdp;
    if (rsdp == NULL
        || memcmp(rsdp->sign, "RSD PTR ", 8) != 0
        || rsdp->version != 0
        || memsum(rsdp, sizeof(*rsdp)) != 0) {
        printf("Bad RSDP\n");
        return;
    }

    uintptr_t table_pa = rsdp->rsdt_pa;
    // TODO wtf is the size here?? need unmap_pa
    rsdt_t* rsdt = map_phy(table_pa, -1, 0);

    uint8_t sum;
    if (memcmp(rsdt->header.sign, "RSDT", 4) != 0 || (sum = memsum(rsdt, rsdt->header.len) != 0)) {
        printf("Bad RSDT\n");
        return;
    }

    size_t nr_rsdt_entries = ((rsdt->header.len - sizeof(rsdt->header)) / sizeof(rsdt->other_sdt[0]));
    printf("[ACPI: ");
    if (nr_rsdt_entries > 0)
        printf("Detected tables");
    else
        printf("No tables found");
    for (size_t i = 0; i < nr_rsdt_entries; ++i) {
        table_pa = rsdt->other_sdt[i];
        // TODO wtf is the size here?? need unmap_pa
        acpi_sdt_header_t* h = map_phy(table_pa, -1, 0);

        printf(" ");
        for (int j = 0; j < 4; ++j)
            printf("%c", h->sign[j]);
        if (memsum(h, h->len) == 0 && !memcmp(h->sign, "APIC", 4))
            parse_madt((void*)h);
    }
    printf("]\n");
}
