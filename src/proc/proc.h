#ifndef PROC_H
#define PROC_H

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>

typedef struct proc {
    size_t pgdir;
    uintptr_t code;
    uintptr_t stack;
    void* kstack;
    cpu_state_t cpu_state;
} proc_t;

void init_proc(void);
proc_t* alloc_proc(void);
void switch_proc(proc_t* p);

#endif // PROC_H
