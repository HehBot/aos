#include "mp.h"

#include <cpu/x86.h>
#include <string.h>

cpu_t cpus[MAX_CPUS];
size_t nr_cpus = 0;

cpu_t* get_cpu(void)
{
    // if (get_eflags() & EFLAGS_INT)
    //     PANIC(__FILE__);

    uint8_t lapic_id(void);
    uint8_t id = lapic_id();

    for (size_t i = 0; i < nr_cpus; ++i)
        if (cpus[i].lapic_id == id)
            return &cpus[i];
    PANIC("lapic_id() does not match with any CPU");
}
uint8_t cpu_id(void)
{
    return (get_cpu() - &cpus[0]);
}

void init_seg(void)
{
    cpu_t* c = get_cpu();
    memset(&c->gdt[0], 0, sizeof(c->gdt));
    c->gdt[KERNEL_CODE_GDT_INDEX] = SEG(KERNEL_PL, 1, 0);
    c->gdt[KERNEL_DATA_GDT_INDEX] = SEG(KERNEL_PL, 0, 1);
    c->gdt[USER_CODE_GDT_INDEX] = SEG(USER_PL, 1, 0);
    c->gdt[USER_DATA_GDT_INDEX] = SEG(USER_PL, 0, 1);
    lgdt(&c->gdt[0], sizeof(c->gdt), KERNEL_CODE_SEG);
}
