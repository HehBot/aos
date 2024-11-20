#include "mp.h"
#include "spinlock.h"
#include "x86.h"

static void pushcli(void)
{
    uint64_t rflags = read_rflags();
    disable_interrupts();

    cpu_t* c = get_cpu();

    if (c->ncli == 0)
        c->interrut_enabled = (rflags & RFLAGS_INT);
    c->ncli += 1;
}
static void popcli(void)
{
    if (read_rflags() & RFLAGS_INT)
        PANIC("popcli - interruptible");
    cpu_t* c = get_cpu();
    if (c->ncli == 0)
        PANIC("popcli");
    c->ncli--;
    if (c->ncli == 0 && c->interrut_enabled)
        enable_interrupts();
}

int holding(spinlock_t* lock)
{
    pushcli();
    int r = lock->locked && lock->held_by == get_cpu();
    popcli();
    return r;
}

void acquire(spinlock_t* lock)
{
    pushcli();
    if (holding(lock))
        PANIC("acquire");

    while (!__sync_bool_compare_and_swap(&lock->locked, 0, 1))
        ;

    lock->held_by = get_cpu();
}
void release(spinlock_t* lock)
{
    if (!holding(lock))
        PANIC("release");

    __sync_synchronize();
    lock->locked = 0;

    popcli();
}
