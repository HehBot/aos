#ifndef MP_H
#define MP_H

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int ncli;
    int interrut_enabled;

    gdt_entry_t gdt[NR_GDT_ENTRIES];
    tss_t tss;

    uint8_t acpi_proc_id;
    uint8_t lapic_id;
    uint8_t ist_stack[2000];
} cpu_t;

#define MAX_CPUS 32
extern cpu_t cpus[MAX_CPUS];
extern size_t nr_cpus;

cpu_t* get_cpu(void);
uint8_t cpu_id(void);

void init_lapic(void* lapic_addr);
void lapic_eoi(void);

void init_cpu(void);

#endif // MP_H
