#include "port.h"
#include "x86.h"

#include <memory/paging.h>

// Taken from xv6:lapic.c

// LAPIC registers
#define LAPIC_ID (0x0020 >> 2) // ID
#define LAPIC_VER (0x0030 >> 2) // Version
#define LAPIC_TPR (0x0080 >> 2) // Task Priority
#define LAPIC_EOI (0x00B0 >> 2) // EOI
#define LAPIC_LDR (0x00E0 >> 2) // Logical Destination
#define LAPIC_DFR (0x00E0 >> 2) // Destionation Format
#define LAPIC_SVR (0x00F0 >> 2) // Spurious Interrupt Vector
#define SW_ENABLE 0x00000100 // Unit Enable
#define LAPIC_ESR (0x0280 >> 2) // Error Status
#define LAPIC_ICRL (0x0300 >> 2) // Interrupt Command
#define LAPIC_ICRH (0x0310 >> 2) // Interrupt Command [63:32]
#define INIT 0x00000500 // INIT/RESET
#define STARTUP 0x00000600 // Startup IPI
#define DELIVS 0x00001000 // Delivery status
#define ASSERT 0x00004000 // Assert interrupt (vs deassert)
#define DEASSERT 0x00000000
#define LEVEL 0x00008000 // Level triggered
#define BCAST 0x00080000 // Send to all LAPICs, including self.
#define BUSY 0x00001000
#define FIXED 0x00000000
#define LAPIC_TIMER (0x0320 >> 2) // Local Vector Table 0 (TIMER)
#define TIMER_PERIODIC 0x00020000 // Periodic
#define LAPIC_PCINT (0x0340 >> 2) // Performance Counter LVT
#define LAPIC_LINT0 (0x0350 >> 2) // Local Vector Table 1 (LINT0)
#define LAPIC_LINT1 (0x0360 >> 2) // Local Vector Table 2 (LINT1)
#define LAPIC_ERROR (0x0370 >> 2) // Local Vector Table 3 (ERROR)
#define DISABLE 0x00010000 // Interrupt masked
#define LAPIC_TICR (0x0380 >> 2) // Timer Initial Count
#define LAPIC_TCCR (0x0390 >> 2) // Timer Current Count
#define LAPIC_TDCR (0x03E0 >> 2) // Timer Divide Configuration

// 8259 APIC
#define PORT_8259APIC_MASTER 0x20
#define PORT_8259APIC_SLAVE 0xa0
#define PORT_8259APIC_MASTER_CTRL (PORT_8259APIC_MASTER)
#define PORT_8259APIC_MASTER_DATA (PORT_8259APIC_MASTER + 1)
#define PORT_8259APIC_SLAVE_CTRL (PORT_8259APIC_SLAVE)
#define PORT_8259APIC_SLAVE_DATA (PORT_8259APIC_SLAVE + 1)

static uint32_t volatile* lapic;

static uint32_t lapic_read(size_t reg)
{
    return lapic[reg];
}

static void lapic_write(size_t reg, uint32_t val)
{
    lapic[reg] = val;
    lapic[reg]; // read register to wait for write to finish
}

// see https://wiki.osdev.org/APIC_Timer#Example_code_in_ASM
void init_lapic(void* lapic_addr)
{
    lapic = lapic_addr;

    lapic_write(LAPIC_SVR, SW_ENABLE | (T_IRQ0 + IRQ_SPUR));

    lapic_write(LAPIC_TIMER, TIMER_PERIODIC | (T_IRQ0 + IRQ_TIMER));
    lapic_write(LAPIC_TDCR, 0x1);
    lapic_write(LAPIC_TICR, 10000);

    lapic_write(LAPIC_LINT0, DISABLE);
    lapic_write(LAPIC_LINT1, DISABLE);
    if (((lapic_read(LAPIC_VER) >> 16) & 0xff) >= 4)
        lapic_write(LAPIC_PCINT, DISABLE);

    lapic_write(LAPIC_ERROR, (T_IRQ0 + IRQ_ERR));

    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0);

    lapic_write(LAPIC_EOI, 0);

    lapic_write(LAPIC_ICRH, 0);
    lapic_write(LAPIC_ICRL, BCAST | INIT | LEVEL);
    while (lapic[LAPIC_ICRL] & DELIVS)
        ;

    lapic_write(LAPIC_TPR, 0);

    // mask 8295A PICs
    port_write_byte(PORT_8259APIC_MASTER_DATA, 0xff);
    port_write_byte(PORT_8259APIC_SLAVE_DATA, 0xff);
}

uint8_t lapic_id(void)
{
    return (lapic[LAPIC_ID] >> 24);
}

void lapic_eoi(void)
{
    lapic_write(LAPIC_EOI, 0);
}
