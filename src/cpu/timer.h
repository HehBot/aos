#ifndef TIMER_H
#define TIMER_H

#include "interrupt.h"

void init_timer(unsigned int freq, isr_t timer_callback);

#endif // TIMER_H
