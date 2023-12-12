#include "proc.h"

#include <cpu/x86.h>
#include <liballoc.h>
#include <mem/mm.h>
#include <mem/pmm.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern gdt_desc_t gdt_desc;

typedef struct tss_entry_struct {
    uint32_t : 32;
    uint32_t esp; // kernel stack pointer to load when changing to kernel mode.
    uint16_t ss; // kernel stack segment to load when changing to kernel mode.
    uint16_t useless[47];
} __attribute__((packed)) tss_entry_t;

static tss_entry_t tss_entry;

static inline void init_tss(void)
{
    gdt_entry_t* g = &gdt_desc.gdt[TSS_GDT_INDEX];

    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = (sizeof tss_entry);

    g->limit_low = limit & 0xffff;
    g->limit_high = (limit >> 16) & 0xf;
    g->base_low = base & 0xffff;
    g->base_mid = (base >> 16) & 0xff;
    g->base_high = (base >> 24) & 0xff;

    memset(&tss_entry, 0, sizeof tss_entry);
}
static inline void set_tss(void* kstack)
{
    tss_entry.ss = KERNEL_DATA_SEG;
    tss_entry.esp = (uint32_t)kstack;

    ltr(TSS_SEG);
}

void init_proc(void)
{
    init_tss();
}

proc_t* alloc_proc(void)
{
    proc_t* p = kmalloc(sizeof(*p));
    p->kstack = kmalloc(0x500);

    size_t d = alloc_page_directory();
    switch_page_directory(d);
    p->pgdir = d;

    p->code = 0x0;
    p->stack = (KERN_BASE - 16);
    map_page(pmm_get_frame(), p->stack, PTE_W | PTE_U);

    cpu_state_t cpu_state = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, USER_DATA_SEG, USER_DATA_SEG, USER_DATA_SEG, USER_DATA_SEG, 0x0, 0x0, p->code, USER_CODE_SEG, 0x0, p->stack, USER_DATA_SEG };
    p->cpu_state = cpu_state;
    switch_page_directory(0);
    return p;
}
void switch_proc(proc_t* p)
{
    switch_page_directory(p->pgdir);
    set_tss(p->kstack);
    void enter_usermode(cpu_state_t*);
    enter_usermode(&p->cpu_state);
}
