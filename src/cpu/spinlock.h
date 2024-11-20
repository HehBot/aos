#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "mp.h"

typedef struct spinlock {
    volatile int locked;
    cpu_t* held_by;
} spinlock_t;

int holding(spinlock_t* lock);
void acquire(spinlock_t* lock);
void release(spinlock_t* lock);

#endif // SPINLOCK_H
