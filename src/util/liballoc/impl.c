#include <memory/kalloc.h>
#include <stddef.h>

int liballoc_lock()
{
    return 0;
}

int liballoc_unlock()
{
    return 0;
}

void* liballoc_alloc(size_t pages)
{
    return kpalloc(pages);
}

int liballoc_free(void* ptr, size_t)
{
    return kpfree(ptr);
}
