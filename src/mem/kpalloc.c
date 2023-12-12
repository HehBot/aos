#include "kwmalloc.h"
#include "mm.h"
#include "pmm.h"

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct run {
    struct run* next;
    struct run* prev;
    uintptr_t addr;
    size_t alloc : 1;
    size_t sz : 31;
} run_t;

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
    PANIC();
}
static inline void free_run(run_t* p)
{
    p->alloc = 0;
}

static run_t* free_list;
static run_t* free_list_end;
static uintptr_t end;
static run_t* occ_list;

void init_kpalloc(void)
{
    pool = kwmalloc(POOL_SZ * (sizeof pool[0]));
    memset(pool, 0, POOL_SZ * (sizeof pool[0]));

    free_list = free_list_end = alloc_run();
    free_list->next = free_list->prev = NULL;

    extern void* kernel_heap_start;
    free_list->addr = (uintptr_t)&kernel_heap_start;
    free_list->sz = INIT_HEAP_SZ;

    end = free_list->addr + INIT_HEAP_SZ * PAGE_SIZE;
    occ_list = NULL;
}

void* kpalloc(size_t n)
{
    run_t* p = free_list;

    for (; p != NULL && p->sz < n; p = p->next)
        ;

    if (p == NULL) {

        if (n > 0x100000 || end > 0xffffffff - n * PAGE_SIZE)
            return NULL;

        size_t nr;
        if (free_list_end != NULL && free_list_end->addr + free_list_end->sz * PAGE_SIZE == end)
            nr = (n - free_list_end->sz);
        else
            nr = n;

        // optimise
        // nr = (nr * 3) / 2;

        for (uintptr_t virt = end; virt < end + nr * PAGE_SIZE; virt += PAGE_SIZE)
            map_page(pmm_get_frame(), virt, PTE_W);

        if (free_list_end != NULL && free_list_end->addr + free_list_end->sz * PAGE_SIZE == end)
            free_list_end->sz += nr;
        else if (free_list_end != NULL) {
            free_list_end->next = alloc_run();
            free_list_end->next->prev = free_list_end;
            free_list_end = free_list_end->next;
            free_list_end->sz = nr;
            free_list_end->addr = end;
        } else {
            free_list = free_list_end = alloc_run();
            free_list->next = free_list->prev = NULL;
            free_list->sz = nr;
            free_list->addr = end;
        }

        end += nr * PAGE_SIZE;
        return kpalloc(n);
    } else if (p->sz == n) {
        if (p->next != NULL)
            p->next->prev = p->prev;
        else
            free_list_end = p->prev;
        if (p->prev != NULL)
            p->prev->next = p->next;
        else
            free_list = p->next;
    } else {
        p->sz -= n;
        run_t* new = alloc_run();
        new->addr = (p->addr + p->sz * PAGE_SIZE);
        new->sz = n;
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
    return (void*)p->addr;
}

int kpfree(void* a)
{
    uintptr_t addr = PG_ROUND_DOWN((uintptr_t)a);
    run_t* p = occ_list;
    for (; p != NULL && p->addr != addr; p = p->next)
        ;
    if (p == NULL)
        return 1;
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
        while (p->prev != NULL && (p->prev->addr + p->prev->sz * PAGE_SIZE == p->addr))
            p = p->prev;
        while (p->next != NULL && (p->addr + p->sz * PAGE_SIZE == p->next->addr)) {
            p->sz += p->next->sz;
            run_t* old = p->next;
            p->next = p->next->next;
            free_run(old);
        }
        if (p->next != NULL)
            p->next->prev = p;
        else
            free_list_end = p;
    }
    return 0;
}

/*
#include <stdio.h>
static void print_kheap(void)
{
    run_t* p = occ_list;
    printf("Page allocator status:\n");
    if (p == NULL)
        printf("  No occu\n");
    else {
        printf("  Occu:\n");
        for (; p != NULL; p = p->next)
            printf("0x%x-0x%x %dpg; ", p->addr, p->addr+p->sz*PAGE_SIZE, p->sz);
        printf("\n");
    }
    p = free_list;
    if (p == NULL)
        printf("  No free\n");
    else {
        printf("  Free:\n");
        for (; p != NULL; p = p->next)
            printf("0x%x-0x%x %dpg; ", p->addr, p->addr+p->sz*PAGE_SIZE, p->sz);
        printf("\n");
    }
}
 */
void test_kpalloc(void)
{
    //     print_kheap();
    void* p = kpalloc(721);
    //     print_kheap();
    void* p2 = kpalloc(4);
    //     print_kheap();
    kpfree(p);
    //     print_kheap();
    p = kpalloc(800);
    //     print_kheap();
    kpfree(p2);
    //     print_kheap();
    kpfree(p);
    //     print_kheap();
    p = kpalloc(1000);
    //     print_kheap();
    kpfree(p);
    //     print_kheap();
}
