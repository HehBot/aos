#ifndef TIMER_H
#define TIMER_H

#include "interrupt.h"

#include <stdint.h>

void init_timer(uint32_t freq, isr_t timer_callback);

#endif // TIMER_H
