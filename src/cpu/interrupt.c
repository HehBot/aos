#include <cpu/mp.h>
#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define GATE_TYPE_INT 0xe
#define GATE_TYPE_TRAP 0xf

#define NR_ISRS 256
static idt_entry_t idt[NR_ISRS] = { 0 };
extern void (*(isrs[256]))(void);

// TODO TSS for separate stack for interrupt handlers
static inline void set_idt_entry(size_t n, void (*isr)(void), uint8_t gate_type, uint8_t dpl, uint16_t kernel_cs)
{
    uintptr_t addr = (uintptr_t)isr;
    idt[n] = (idt_entry_t) {
        .low_offset = (addr & 0xffff),
        .seg = kernel_cs,
        .gate_type = gate_type,
        .dpl = dpl,
        .present = 1,
        .mid_offset = (addr >> 16) & 0xffff,
        .high_offset = (addr >> 32)
    };
}

void init_idt(uint16_t kernel_cs)
{
    for (size_t i = 0; i < NR_ISRS; ++i)
        set_idt_entry(i, isrs[i], GATE_TYPE_INT, KERNEL_PL, kernel_cs);
    set_idt_entry(T_SYSCALL, isrs[T_SYSCALL], GATE_TYPE_TRAP, USER_PL, kernel_cs);
    lidt(&idt[0], sizeof(idt));
}

void excep(cpu_state_t* cpu_state)
{
    static char const* exception_messages[32] = {
        "Division By Zero",
        "Debug Panic",
        "Non Maskable Interrupt",
        "Breakpoint",
        "Into Detected Overflow",
        "Out of Bounds",
        "Invalid Opcode",
        "No Coprocessor",

        "Double Fault",
        "Coprocessor Segment Overrun",
        "Bad TSS",
        "Segment Not Present",
        "Stack Fault",
        "General Protection Fault",
        "Page Fault",
        "Unknown Interrupt",

        "Coprocessor Fault",
        "Alignment Check",
        "Machine Check",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",

        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved"
    };

    int int_no = cpu_state->int_no, err_code = cpu_state->err_code;

    if (int_no == 3)
        return;

    printf("\nPANIC\n");
    printf("%s detected\n", exception_messages[int_no]);

    switch (int_no) {
    case 13:
        // https://wiki.osdev.org/Exceptions#Selector_Error_Code
        if (err_code != 0) {
            if (err_code & 0x1)
                printf("External\n");
            else {
                printf("Internal: ");
                switch ((err_code >> 1) & 0x3) {
                case 0:
                    printf("GDT");
                    break;
                case 1:
                case 3:
                    printf("IDT");
                    break;
                case 2:
                    printf("LDT");
                    break;
                }
                printf(" index %u\n", err_code >> 3);
            }
        }
        break;
    case 14:
        // https://wiki.osdev.org/Exceptions#Page_Fault
        printf("At address %p: ", rcr2());
        if (err_code & PTE_P)
            printf("protection violation");
        else
            printf("page not present");
        printf(", ");
        if (err_code & PTE_W)
            printf("attempted write");
        else
            printf("attempted read");
        printf("\n");
        break;
    }
    printf("\nRegister states at exception:\n");

    printf("  rax: ..................... 0x%lx\n", cpu_state->rax);
    printf("  rbx: ..................... 0x%lx\n", cpu_state->rbx);
    printf("  rcx: ..................... 0x%lx\n", cpu_state->rcx);
    printf("  rdx: ..................... 0x%lx\n", cpu_state->rdx);
    printf("  rdi: ..................... 0x%lx\n", cpu_state->rdi);
    printf("  rsi: ..................... 0x%lx\n", cpu_state->rsi);
    printf("  rbp: ..................... 0x%lx\n", cpu_state->rbp);
    printf("  rsp: ..................... 0x%lx\n", cpu_state->rsp);
    printf("  rip: ..................... 0x%lx\n", cpu_state->rip);
    printf("\n");
    printf("  cs: 0x%lx\n", cpu_state->cs);
    printf("\n");
    printf("  Interrupt Number: ........ %u\n", int_no);
    printf("  Error Code: .............. %u\n", err_code);
    printf("  rflags: .................. 0x%lx\n", cpu_state->rflags);

    HALT();
}

void syscall(cpu_state_t* cpu_state)
{
    printf("Syscall number %lx\n", cpu_state->rax);
    // if (cpu_state->eax == 0) {
    //     char* z = (char*)cpu_state->ebx;
    //     for (size_t i = 0; i < cpu_state->ecx; ++i)
    //         printf("%c", z[i]);
    // }
}

void isr_common(cpu_state_t* cpu_state)
{
    int int_no = cpu_state->int_no;
    void keyboard_callback(cpu_state_t*);
    if (int_no < 32)
        excep(cpu_state);
    else {
        switch (int_no) {
        case T_SYSCALL:
            syscall(cpu_state);
            break;
        case T_IRQ0 + IRQ_KBD:
            keyboard_callback(cpu_state);
            lapic_eoi();
            break;
        case T_IRQ0 + IRQ_SPUR:
            printf("SPURIOUS!\n");
            break;
        case T_IRQ0 + IRQ_TIMER:
            printf("TIMER\n");
            lapic_eoi();
            break;
        default:
            printf("Unregistered ISR %d\n", int_no);
        }
    }
}
