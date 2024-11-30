#include <cpu/spinlock.h>
#include <memory/kalloc.h>
#include <stddef.h>

static spinlock_t lock = {};

int liballoc_lock()
{
    acquire(&lock);
    return 0;
}

int liballoc_unlock()
{
    release(&lock);
    return 0;
}

void* liballoc_alloc(size_t pages)
{
    return kpalloc(pages);
}

int liballoc_free(void* ptr, size_t)
{
    return !kpfree(ptr);
}
