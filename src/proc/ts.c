#include <cpu/x86.h>
#include <string.h>

extern gdt_desc_t gdt_desc;

typedef struct tss_entry_struct {
    uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
    uint32_t esp0; // The stack pointer to load when changing to kernel mode.
    uint32_t ss0; // The stack segment to load when changing to kernel mode.
    uint32_t useless[23];
} __attribute__((packed)) tss_entry_t;

static tss_entry_t tss_entry;

void init_tss(void)
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

void set_tss(void* kstack)
{
    tss_entry.ss0 = KERNEL_DATA_SEG;
    tss_entry.esp0 = (uint32_t)kstack;

    ltr(TSS_SEG);
}
