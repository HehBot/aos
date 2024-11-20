#include "mp.h"

#include <cpu/interrupt.h>
#include <cpu/x86.h>
#include <string.h>

cpu_t cpus[MAX_CPUS] = { 0 };
size_t nr_cpus = 0;

cpu_t* get_cpu(void)
{
    if (read_rflags() & RFLAGS_INT)
        PANIC("get_cpu - interruptible");

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

static void init_gdt(cpu_t* c)
{
    memset(&c->gdt[0], 0, sizeof(c->gdt));
    memset(&c->tss, 0, sizeof(c->tss));

    c->gdt[KERNEL_CODE_GDT_INDEX] = SEG(KERNEL_PL, 1, 0);
    c->gdt[USER_CODE_GDT_INDEX] = SEG(USER_PL, 1, 0);
    c->gdt[TSS_GDT_INDEX] = SEG_TSS1(KERNEL_PL, (uintptr_t)&c->tss, sizeof(c->tss));
    c->gdt[TSS_GDT_INDEX + 1] = SEG_TSS2((uintptr_t)&c->tss);

    c->tss.ist[DOUBLE_FAULT_IST] = ((uintptr_t)&c->ist_stack) + sizeof(c->ist_stack);
    c->tss.iomap_base = sizeof(tss_t);

    // to verify that excep_stack is indeed used, uncomment
    //      the next line and see that `int $T_DOUBLE_FAULT` causes #GPF
    // c->tss.ist[DOUBLE_FAULT_IST] = 0x8ffffffffffff;
}

void init_cpu(void)
{
    cpu_t* c = get_cpu();

    init_gdt(c);
    lgdt(&c->gdt[0], sizeof(c->gdt), KERNEL_CODE_SEG);
    ltr(TSS_SEG);

    load_idt();
    printf("[Started cpu %d]\n", c->acpi_proc_id);
}
