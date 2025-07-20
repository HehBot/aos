#ifndef MP_H
#define MP_H

#include <cpu/x86.h>
#include <memory/page.h>
#include <stddef.h>
#include <stdint.h>

typedef struct task task_t;

typedef struct {
    task_t* curr_task;

    int ncli;
    int interrut_enabled;

    gdt_entry_t gdt[NR_GDT_ENTRIES];
    tss_t tss;

    struct {
        uint64_t _scratch_rsp;
        uint64_t kernel_stack_rsp;
    } __attribute__((packed)) syscall_info;

    uint8_t acpi_proc_id;
    uint8_t lapic_id;
    uint8_t interrupt_kernel_stack[1200] __attribute__((aligned(16)));
} cpu_t;

#define MAX_CPUS 32
extern cpu_t cpus[MAX_CPUS];
extern size_t nr_cpus;

cpu_t* get_cpu(void);
uint8_t cpu_id(void);

void init_lapic(virt_addr_t mapping_addr_ptr, phys_addr_t lapic_addr);
void lapic_eoi(void);

void init_cpu(void);

#endif // MP_H
