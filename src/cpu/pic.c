#include "port.h"

void PIC_send_EOI(unsigned char irq)
{
#define PIC_EOI 0x20
    if (irq >= 8)
        port_write_byte(PORT_PIC2_CTRL, PIC_EOI);
    port_write_byte(PORT_PIC1_CTRL, PIC_EOI);
}

/*  After initialising PIC using ICW1 byte, it waits for 3 extra initalisation bytes on data port
 *  Vector offset ICW2
 *  Master/slave wiring ICW3
 *  Additional info ICW4
 */
void PIC_remap(unsigned char offset1, unsigned char offset2)
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
    unsigned char a1 = port_read_byte(PORT_PIC1_DATA);
    unsigned char a2 = port_read_byte(PORT_PIC2_DATA);

    // ICW1 Initialise in cascade mode
    port_write_byte(PORT_PIC1_CTRL, ICW1_INIT | ICW1_ICW4);
    port_write_byte(PORT_PIC2_CTRL, ICW1_INIT | ICW1_ICW4);

    // ICW2
    port_write_byte(PORT_PIC1_DATA, offset1);
    port_write_byte(PORT_PIC1_DATA, offset2);

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
void PIC_set_mask(unsigned short int m)
{
    port_write_byte(PORT_PIC1_DATA, m & 0xff);
    port_write_byte(PORT_PIC2_DATA, (m >> 8) & 0xff);
}
