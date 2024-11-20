#include "frame_allocator.h"
#include "kalloc.h"
#include "paging.h"

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct run {
    struct run* next;
    struct run* prev;
    virt_addr_t addr;
    size_t nr_pages;
    int alloc;
} __attribute__((packed)) run_t;

static run_t* pool;
#define POOL_SZ 1000
static inline run_t* alloc_run()
{
    static size_t last = 0;
    for (size_t i = last; i < POOL_SZ; ++i)
        if (!pool[i].alloc) {
            pool[i].alloc = 1;
            last = i;
            return &pool[i];
        }
    for (size_t i = 0; i < last; ++i)
        if (!pool[i].alloc) {
            pool[i].alloc = 1;
            last = i;
            return &pool[i];
        }
    PANIC("Unable to allocate run");
}
static inline void free_run(run_t* p)
{
    p->alloc = 0;
}

#define INIT_NR_PAGES 16
#define NR_PAGES_INCREMENT 16

static run_t* free_list;
static run_t* free_list_end;
static virt_addr_t end;
static run_t* occ_list;

void init_kpalloc(virt_addr_t heap_start)
{
    pool = heap_start;
    for (size_t i = 0; i < POOL_SZ * sizeof(pool[0]); i += PAGE_SIZE, heap_start += PAGE_SIZE) {
        phys_addr_t frame = frame_allocator_get_frame();
        if (frame == FRAME_ALLOCATOR_ERROR_NO_FRAME_AVAILABLE)
            PANIC("Unable to allocate frame for kpalloc pool");
        int err = paging_map(heap_start, frame, PAGE_4KiB, PTE_P | PTE_W);
        if (err != PAGING_OK)
            PANIC("Unable to map pages for kpalloc pool");
    }
    memset(pool, 0, POOL_SZ * (sizeof pool[0]));

    free_list = free_list_end = alloc_run();
    free_list->next = free_list->prev = NULL;

    free_list->addr = heap_start;
    free_list->nr_pages = INIT_NR_PAGES;

    end = free_list->addr + INIT_NR_PAGES * PAGE_SIZE;
    printf("[Heap: %p-%p]\n", heap_start, end);
    occ_list = NULL;

    for (size_t i = 0; i < INIT_NR_PAGES; ++i) {
        phys_addr_t frame = frame_allocator_get_frame();
        if (frame == FRAME_ALLOCATOR_ERROR_NO_FRAME_AVAILABLE)
            PANIC("Unable to allocate frames for heap");
        int err = paging_map(heap_start + i * PAGE_SIZE, frame, PAGE_4KiB, PTE_W | PTE_P);
        if (err != PAGING_OK)
            PANIC("Unable to map heap");
    }
}

void* kpalloc(size_t n)
{
    run_t* p = free_list;

    for (; p != NULL && p->nr_pages < n; p = p->next)
        ;

    if (p == NULL) {
        size_t nr;
        if (free_list_end != NULL && free_list_end->addr + free_list_end->nr_pages * PAGE_SIZE == end)
            nr = (n - free_list_end->nr_pages);
        else
            nr = n;

        if (nr < 16)
            nr = 16;
        else {
            // optimise
            // nr = (nr * 3) / 2;
        }

        for (virt_addr_t page = end; page < end + nr * PAGE_SIZE; page += PAGE_SIZE) {
            phys_addr_t frame = frame_allocator_get_frame();
            if (frame == FRAME_ALLOCATOR_ERROR_NO_FRAME_AVAILABLE)
                PANIC("Unable to allocate more frames for heap");
            int err = paging_map(page, frame, PAGE_4KiB, PTE_W | PTE_P);
            if (err != PAGING_OK)
                PANIC("Unable to map heap");
        }

        if (free_list_end != NULL && free_list_end->addr + free_list_end->nr_pages * PAGE_SIZE == end)
            free_list_end->nr_pages += nr;
        else if (free_list_end != NULL) {
            free_list_end->next = alloc_run();
            free_list_end->next->prev = free_list_end;
            free_list_end = free_list_end->next;
            free_list_end->nr_pages = nr;
            free_list_end->addr = end;
        } else {
            free_list = free_list_end = alloc_run();
            free_list->next = free_list->prev = NULL;
            free_list->nr_pages = nr;
            free_list->addr = end;
        }

        end += nr * PAGE_SIZE;
        return kpalloc(n);
    } else if (p->nr_pages == n) {
        if (p->next != NULL)
            p->next->prev = p->prev;
        else
            free_list_end = p->prev;
        if (p->prev != NULL)
            p->prev->next = p->next;
        else
            free_list = p->next;
    } else {
        p->nr_pages -= n;
        run_t* new = alloc_run();
        new->addr = (p->addr + p->nr_pages * PAGE_SIZE);
        new->nr_pages = n;
        p = new;
    }

    if (occ_list == NULL) {
        occ_list = p;
        p->next = p->prev = NULL;
    } else {
        occ_list->prev = p;
        p->next = occ_list;
        p->prev = NULL;
        occ_list = p;
    }
    return p->addr;
}

int kpfree(virt_addr_t addr)
{
    run_t* p = occ_list;
    for (; p != NULL && p->addr != addr; p = p->next)
        ;
    if (p == NULL)
        return 0;
    if (p->next != NULL)
        p->next->prev = p->prev;
    if (p->prev != NULL)
        p->prev->next = p->next;
    else
        occ_list = p->next;

    if (free_list == NULL) {
        free_list = free_list_end = p;
        free_list->next = free_list->prev = NULL;
    } else {
        run_t* l = NULL;
        run_t* r = free_list;
        while (r != NULL && r->addr < p->addr) {
            l = r;
            r = r->next;
        }
        if (l == NULL) {
            if (r == NULL) {
                free_list = free_list_end = p;
                p->prev = p->next = NULL;
            } else {
                r->prev = p;
                p->next = r;
                p->prev = NULL;
                free_list = p;
            }
        } else {
            p->prev = l;
            p->next = l->next;
            if (l->next != NULL)
                l->next->prev = p;
            else
                free_list_end = p;
            l->next = p;
        }

        // coalesce free areas
        while (p->prev != NULL && (p->prev->addr + p->prev->nr_pages * PAGE_SIZE == p->addr))
            p = p->prev;
        while (p->next != NULL && (p->addr + p->nr_pages * PAGE_SIZE == p->next->addr)) {
            p->nr_pages += p->next->nr_pages;
            run_t* old = p->next;
            p->next = p->next->next;
            free_run(old);
        }
        if (p->next != NULL)
            p->next->prev = p;
        else
            free_list_end = p;
    }
    return 1;
}
