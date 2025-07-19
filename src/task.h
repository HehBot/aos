#ifndef TASK_H
#define TASK_H

#include <cpu/x86.h>
#include <stddef.h>

typedef struct task {
    size_t tid;

    virt_addr_t kernel_stack;

    context_t* context;

    enum {
        TASK_S_INVALID = 0,
        TASK_S_EMBRYO,
        TASK_S_READY,
        TASK_S_RUNNING,
    } state;

    pgdir_t pgdir;
} task_t;

task_t* new_task(int is_kernel_task);

#endif // TASK_H
