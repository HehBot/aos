#include "x86.h"

#include <memory/frame_allocator.h>
#include <memory/paging.h>
#include <stdint.h>

typedef struct {
    uint8_t reg;
    uint8_t : 8;
    uint16_t : 16;
    uint32_t : 32;
    uint32_t : 32;
    uint32_t : 32;
    uint32_t data;
} __attribute__((packed)) ioapic_t;

static uint8_t ioapic_id;
static ioapic_t volatile* ioapic;

#define IOAPIC_ID 0x00
#define IOAPIC_VER 0x01
#define IOAPIC_TABLE 0x10

#define DISABLED 0x10000 // default ENABLED
#define LEVEL 0x8000 // default EDGE triggered
#define LOW 0x2000 // default active HIGH
#define DEST_CPU_ID 0x800 // destination is interpreted as APIC ID by default

static uint32_t ioapic_read(uint8_t reg)
{
    ioapic->reg = reg;
    return ioapic->data;
}

static void ioapic_write(uint8_t reg, uint32_t data)
{
    ioapic->reg = reg;
    ioapic->data = data;
}

void init_ioapic(virt_addr_t* mapping_addr_ptr, phys_addr_t ioapic_addr, uint8_t id)
{
    ioapic = *mapping_addr_ptr;
    ioapic_id = id;

    int err = frame_allocator_reserve_frame(ioapic_addr);
    ASSERT(err == FRAME_ALLOCATOR_ERROR_NO_SUCH_FRAME);
    err = paging_map(*mapping_addr_ptr, ioapic_addr, PAGE_4KiB, PTE_NX | PTE_W | PTE_P);
    ASSERT(err == PAGING_OK);
    *mapping_addr_ptr += PAGE_SIZE;

    uint8_t z = (ioapic_read(IOAPIC_ID) >> 24);
    if (z != id)
        PANIC("IOAPIC ID does not match that from ACPI");

    // see https://wiki.osdev.org/IOAPIC
    uint32_t max_intr = ((ioapic_read(IOAPIC_VER) >> 16) & 0xff);
    for (uint32_t irq = 0; irq <= max_intr; ++irq) {
        ioapic_write(IOAPIC_TABLE + 2 * irq, DISABLED | (T_IRQ0 + irq));
        ioapic_write(IOAPIC_TABLE + 2 * irq + 1, 0);
    }
}

void ioapic_enable(uint8_t irq, uint8_t cpu_lapic_id)
{
    ioapic_write(IOAPIC_TABLE + 2 * irq, T_IRQ0 + irq);
    ioapic_write(IOAPIC_TABLE + 2 * irq + 1, cpu_lapic_id << 24);
}
