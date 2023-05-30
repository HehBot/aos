#include <cpu/interrupt.h>
#include <cpu/timer.h>
#include <stdio.h>

static unsigned int time;
static unsigned int freq;

static void timer_callback(cpu_state_t*)
{
    if (time % freq == 0)
        printf("Tick: %d\n", time / freq);
    ++time;
}

void main()
{
    time = 0;
    freq = 1000;
    init_timer(freq, timer_callback);

    //     printf("Testing %s %c %d 0o%o %hu 0x%hhx\n", "hello", 'h', -1234567, 012345, 12345678, 0xffffffff);
}
