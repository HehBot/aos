#include "port.h"

#include <stdint.h>

#define PORT_PIC1 0x20
#define PORT_PIC1_CTRL (PORT_PIC1)
#define PORT_PIC1_DATA (PORT_PIC1 + 1)

#define PORT_PIC2 0xa0
#define PORT_PIC2_CTRL (PORT_PIC2)
#define PORT_PIC2_DATA (PORT_PIC2 + 1)

#define PIC_EOI 0x20

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        port_write_byte(PORT_PIC2_CTRL, PIC_EOI);
    port_write_byte(PORT_PIC1_CTRL, PIC_EOI);
}

/*  After initialising PIC using ICW1 byte, it waits for 3 extra initalisation bytes on data port
 *  Vector offset ICW2
 *  Master/slave wiring ICW3
 *  Additional info ICW4
 */
void pic_remap(uint8_t offset1, uint8_t offset2)
{
#define ICW1_ICW4 0x01 // ICW4 is present
#define ICW1_ICW_SINGLE 0x02 // Single (cascade) mode
#define ICW1_INTERVAL4 0x04 // Call address interval 4 (8)
#define ICW1_LEVEL 0x08 // Level (edge) triggered mode
#define ICW1_INIT 0x10 // Initialisation - REQUIRED!

#define ICW4_8086 0x01 // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO 0x02 // Auto (normal) EOI [end of interrupt]
#define ICW4_BUF_SLAVE 0x08 // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0c // Buffered mode/master
#define ICW4_SFNM 0x10 // Special fully nested (not)

    // save masks
    uint8_t a1 = port_read_byte(PORT_PIC1_DATA);
    uint8_t a2 = port_read_byte(PORT_PIC2_DATA);

    // ICW1 Initialise in cascade mode
    port_write_byte(PORT_PIC1_CTRL, ICW1_INIT | ICW1_ICW4);
    port_write_byte(PORT_PIC2_CTRL, ICW1_INIT | ICW1_ICW4);

    // ICW2
    port_write_byte(PORT_PIC1_DATA, offset1);
    port_write_byte(PORT_PIC2_DATA, offset2);

    // ICW3
    port_write_byte(PORT_PIC1_DATA, 4); // telling PIC1 that IRQ2 has a slave PIC (hence 0b00000100)
    port_write_byte(PORT_PIC2_DATA, 2); // telling PIC2 that it's a slave on IRQ 2 (i.e. its cascade identity)

    // ICW4
    port_write_byte(PORT_PIC1_DATA, ICW4_8086);
    port_write_byte(PORT_PIC2_DATA, ICW4_8086);

    // restore masks
    port_write_byte(PORT_PIC1_DATA, a1);
    port_write_byte(PORT_PIC1_DATA, a2);
}

// when mask bit is set corresponding IRQ is ignored
void pic_set_mask(uint16_t m)
{
    port_write_byte(PORT_PIC1_DATA, m & 0xff);
    port_write_byte(PORT_PIC2_DATA, (m >> 8) & 0xff);
}
