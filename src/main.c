#include <acpi.h>
#include <bootinfo.h>
#include <cpu/interrupt.h>
#include <cpu/ioapic.h>
#include <cpu/mp.h>
#include <cpu/spinlock.h>
#include <cpu/x86.h>
#include <drivers/ega.h>
#include <fs/fs.h>
#include <fs/initrd.h>
#include <hash_table.h>
#include <memory/frame_allocator.h>
#include <memory/kalloc.h>
#include <memory/paging.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTASK 20

typedef struct task {
    size_t tid;

    virt_addr_t user_stack;
    virt_addr_t kernel_stack;

    virt_addr_t rip;

    enum {
        TASK_S_INVALID = 0,
        TASK_S_READY,
        TASK_S_RUNNING,
    } state;

    pgdir_t pgdir;
} task_t;

struct {
    task_t list[NTASK];
    spinlock_t lock;
    int next_tid;
} tasks = { .next_tid = 1 };

task_t* setup_task(fs_node_t* prog)
{
    pgdir_t pgdir = paging_new_pgdir();

    size_t nr_pages = PAGE_ROUND_UP(prog->length) / PAGE_SIZE;

    char* z = (virt_addr_t)0x0;

    for (size_t i = 0; i < nr_pages; ++i) {
        phys_addr_t frame = frame_allocator_get_frame();
        ASSERT((frame & FRAME_ALLOCATOR_ERROR) == 0);
        ASSERT(paging_map(pgdir, z + i * PAGE_SIZE, frame, PAGE_4KiB, PTE_U | PTE_W | PTE_P) == PAGING_OK);
    }

    switch_pgdir(pgdir);
    read_fs(prog, 0, prog->length, z);
    switch_kernel_pgdir();

    void* kernel_stack = kpalloc(3);
    phys_addr_t frame = paging_kernel_unmap(kernel_stack, PAGE_4KiB);
    ASSERT((frame & PAGING_ERROR) == 0);
    ASSERT(frame_allocator_free_frame(frame) == FRAME_ALLOCATOR_OK);
    kernel_stack = kernel_stack + 3 * PAGE_SIZE;

    int slot = -1;
    for (int i = 0; i < NTASK; ++i) {
        if (tasks.list[i].state == TASK_S_INVALID) {
            slot = i;
            break;
        }
    }
    ASSERT(slot != -1);
    task_t* task = &tasks.list[slot];

    *task = (task_t) {
        .tid = tasks.next_tid,
        .kernel_stack = kernel_stack,
        .rip = z,
        .state = TASK_S_READY,
        .pgdir = pgdir,
    };
    tasks.next_tid++;

    return task;
}

void swtch(task_t* t)
{
    switch_pgdir(t->pgdir);
    asm volatile(
        "pushfq;"
        "pop %%r11;"
        "mov %q0, %%rsp;"
        "mov %q1, %%rcx;"
        "sysretq"
        :
        : "r"(t->user_stack), "r"(t->rip));
}

void sched()
{
    acquire(&tasks.lock);

    cpu_t* c = get_cpu();
    ASSERT(c->curr_task != NULL);

    task_t* next_task = NULL;

    for (int i = 0; i < NTASK; ++i) {
        if (tasks.list[i].state == TASK_S_READY) {
            next_task = &tasks.list[i];
            break;
        }
    }
    if (next_task != NULL) {
        c->curr_task->state = TASK_S_READY;
        c->curr_task = next_task;
        c->curr_task->state = TASK_S_RUNNING;
    }
    release(&tasks.lock);

    swtch(c->curr_task);
}

void main(phys_addr_t phys_addr_mboot_info)
{
    struct multiboot_info mboot_info = parse_mboot_info(kernel_static_from_phys_addr(phys_addr_mboot_info));

    init_ega(mboot_info.tag_framebuffer, NULL);
    ega_clear();
    printf("Hello from C!\n");

    section_info_t section_info[4];
    parse_elf_section_info(mboot_info.tag_elf_sections, &section_info);

    init_frame_allocator(mboot_info.tag_mmap);

    struct multiboot_tag_module const* m = mboot_info.tag_initrd_module;
    phys_addr_t mod_start_frame = PAGE_ROUND_DOWN(m->mod_start);
    phys_addr_t mod_end_frame = PAGE_ROUND_DOWN(m->mod_end - 1);
    for (phys_addr_t frame = mod_start_frame; frame <= mod_end_frame; frame += PAGE_SIZE)
        ASSERT(frame_allocator_reserve_frame(frame) == FRAME_ALLOCATOR_OK);

    init_paging(section_info, sizeof(section_info) / sizeof(section_info[0]));

    /*
     * all required physical frames have been reserved
     * NOTE: device phy addresses are not
     *          physical frames and so
     *          will not be returned by
     *          frame_allocator_get_frame
     */

    virt_addr_t heap_start = (virt_addr_t)0xffff800000000000;

    init_ega(mboot_info.tag_framebuffer, &heap_start);

    acpi_info_t acpi_info = parse_acpi(mboot_info.tag_old_acpi, mboot_info.tag_new_acpi, &heap_start);
    init_lapic(&heap_start, acpi_info.lapic_addr);

    virt_addr_t initrd_start = heap_start;
    for (phys_addr_t frame = mod_start_frame; frame <= mod_end_frame; heap_start += PAGE_SIZE, frame += PAGE_SIZE)
        ASSERT(paging_kernel_map(heap_start, frame, PAGE_4KiB, PTE_P | PTE_W) == PAGING_OK);

    init_idt();
    init_cpu(); // needs get_cpu which needs lapic initialised

    /*
     * now we can start using spinlocks
     */
    ega_enable_lock();
    frame_allocator_enable_lock();

    init_ioapic(&heap_start, acpi_info.ioapic_addr, acpi_info.ioapic_id);
    ioapic_enable(IRQ_KBD, get_cpu()->lapic_id);

    init_kpalloc(heap_start);

    pgdir_t kpgdir = paging_new_pgdir();

    fs_node_t* root = read_initrd(initrd_start);

    fs_node_t* user_prog = find_dir_fs(root, "user_prog");

    task_t* init_task = setup_task(user_prog);
    init_task->state = TASK_S_RUNNING;
    get_cpu()->curr_task = init_task;

    __sync_synchronize();
    enable_interrupts();

    sched();

    while (1)
        hlt();
}

typedef struct {
    size_t nr;
    size_t args[6];
} __attribute__((packed)) syscall_t;

void syscall_handler(syscall_t* s)
{
    switch (s->nr) {
    case 0: {
        char* str = kmalloc(s->args[1] + 1);
        memcpy(str, (virt_addr_t)s->args[0], s->args[1]);
        str[s->args[1]] = '\0';
        printf("%s", str);
        kfree(s);
    } break;
    default:
        printf("[unknown syscall#%lu]\n", s->nr);
    }
}
