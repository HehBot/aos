#include <cpu/interrupt.h>
#include <cpu/mp.h>
#include <cpu/spinlock.h>
#include <cpu/x86.h>
#include <memory/frame_allocator.h>
#include <memory/kalloc.h>
#include <memory/paging.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <task.h>

#define NTASK 20

struct {
    task_t list[NTASK];
    spinlock_t lock;
    int next_tid;
} tasks = { .next_tid = 1 };

context_t* schedule_next(context_t* const context)
{
    context_t* new_context = NULL;

    acquire(&tasks.lock);

    cpu_t* c = get_cpu();
    int next_slot = -1;
    if (c->curr_task != NULL) {
        int curr_task_slot = (c->curr_task - &tasks.list[0]);

        for (int i = (1 + curr_task_slot) % NTASK; i != curr_task_slot; i = (i + 1) % NTASK) {
            if (tasks.list[i].state == TASK_S_READY) {
                next_slot = i;
                break;
            }
        }
    } else {
        for (int i = 0; i < NTASK; ++i) {
            if (tasks.list[i].state == TASK_S_READY) {
                next_slot = i;
                break;
            }
        }
    }

    if (next_slot != -1) {
        task_t* next_task = &tasks.list[next_slot];
        if (c->curr_task != NULL) {
            c->curr_task->state = TASK_S_READY;
            c->curr_task->context = context;
        }
        new_context = next_task->context;
        switch_pgdir(next_task->pgdir);
        next_task->state = TASK_S_RUNNING;
        c->curr_task = next_task;
        c->syscall_info.kstack_rsp = (uintptr_t)next_task->kernel_stack;
        c->tss.ist[INTERRUPT_IST] = (uintptr_t)next_task->kernel_stack;
    }

    release(&tasks.lock);

    return new_context;
}

task_t* new_task(int is_kernel_task)
{
    pgdir_t pgdir = paging_new_pgdir();

    void* kernel_stack = kpalloc(3);
    phys_addr_t frame = paging_kernel_unmap(kernel_stack, PAGE_4KiB);
    ASSERT((frame & PAGING_ERROR) == 0);
    ASSERT(frame_allocator_free_frame(frame) == FRAME_ALLOCATOR_OK);
    kernel_stack = kernel_stack + 3 * PAGE_SIZE;

    void* task_stack = (void*)((uintptr_t)0x800000000000 - 16);
    {
        size_t nr_stack_pages = 10;
        virt_addr_t page = (virt_addr_t)PAGE_ROUND_DOWN((uintptr_t)task_stack);
        pte_flags_t flags = PTE_W | PTE_P;
        if (!is_kernel_task)
            flags |= PTE_U;
        for (size_t i = 0; i < nr_stack_pages; ++i) {
            phys_addr_t frame = frame_allocator_get_frame();
            ASSERT((frame & FRAME_ALLOCATOR_ERROR) == 0);
            ASSERT(paging_map(pgdir, page, frame, PAGE_4KiB, flags) == PAGING_OK);
            page -= PAGE_SIZE;
        }
    }

    context_t* context = (kernel_stack - sizeof(*context));
    *context = (context_t) {
        .rip = (uintptr_t)0, // needs to be set by caller
        .cs = (is_kernel_task ? KERNEL_CODE_SEG : USER_CODE_SEG),
        .rflags = RFLAGS_INT,
        .rsp = (uintptr_t)task_stack,
        .ss = (is_kernel_task ? KERNEL_DATA_SEG : USER_DATA_SEG),
    };

    task_t* task = NULL;
    acquire(&tasks.lock);
    for (int i = 0; i < NTASK; ++i) {
        if (tasks.list[i].state == TASK_S_INVALID) {
            task = &tasks.list[i];
            *task = (task_t) {
                .tid = tasks.next_tid,
                .kernel_stack = kernel_stack,
                .context = context,
                .state = TASK_S_EMBRYO,
                .pgdir = pgdir,
            };
            break;
        }
    }
    ASSERT(task != NULL);
    tasks.next_tid++;
    release(&tasks.lock);

    return task;
}
