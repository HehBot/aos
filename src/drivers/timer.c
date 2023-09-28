#include <cpu/interrupt.h>
#include <cpu/port.h>
#include <stdint.h>
#include <stdio.h>

#define PORT_PIT_CTRL 0x43
#define PORT_PIT_CHANNEL0_DATA 0x40
#define PORT_PIT_CHANNEL1_DATA 0x41
#define PORT_PIT_CHANNEL2_DATA 0x42

#define PIT_BASE_FREQ 1193182
#define PIT_MIN_FREQ ((PIT_BASE_FREQ >> 16) + 1)

void init_timer(uint32_t freq, isr_t timer_callback)
{
    register_isr_handler(IRQ0, timer_callback);
    if (freq < PIT_MIN_FREQ)
        freq = PIT_MIN_FREQ;
    uint32_t div = PIT_BASE_FREQ / freq;
    port_write_byte(PORT_PIT_CTRL, 0x36);
    port_write_byte(PORT_PIT_CHANNEL0_DATA, div & 0xff);
    port_write_byte(PORT_PIT_CHANNEL0_DATA, (div >> 8) & 0xff);
}
