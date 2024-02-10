#include <cpu/mp.h>
#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define GATE_TYPE_INT 0xe
#define GATE_TYPE_TRAP 0xf

#define NR_ISRS 256
static idt_entry_t idt[NR_ISRS] = { 0 };
extern void (*(isrs[]))(void);

static inline void set_idt_entry(size_t n, void (*isr)(void), uint8_t gate_type, uint8_t dpl)
{
    uintptr_t addr = (uintptr_t)isr;
    idt[n].low_offset = (addr & 0xffff);
    idt[n].seg = KERNEL_CODE_SEG;
    idt[n].always0_1 = 0;
    idt[n].gate_type = gate_type;
    idt[n].always0_2 = 0;
    idt[n].dpl = dpl;
    idt[n].present = 1;
    idt[n].high_offset = (addr >> 16);
}

void init_isrs(void)
{
    for (size_t i = 0; i < NR_ISRS; ++i)
        set_idt_entry(i, isrs[i], GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_SYSCALL, isrs[T_SYSCALL], GATE_TYPE_TRAP, USER_PL);
}

void init_idt(void)
{
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

    printf("\nPANIC: %s detected\n", exception_messages[int_no]);
    if (err_code) {
        switch (int_no) {
        case 13:
            // https://wiki.osdev.org/Exceptions#Selector_Error_Code
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
            break;
        case 14:
            // https://wiki.osdev.org/Exceptions#Page_Fault
            printf("At address 0x%lx: ", rcr2());
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
    }
    printf("\nRegister states at exception:\n");

    printf("  eax: ..................... 0x%lx\n", cpu_state->eax);
    printf("  ebx: ..................... 0x%lx\n", cpu_state->ebx);
    printf("  ecx: ..................... 0x%lx\n", cpu_state->ecx);
    printf("  edx: ..................... 0x%lx\n", cpu_state->edx);
    printf("  edi: ..................... 0x%lx\n", cpu_state->edi);
    printf("  esi: ..................... 0x%lx\n", cpu_state->esi);
    printf("  ebp: ..................... 0x%lx\n", cpu_state->ebp);
    printf("  useresp: ................. 0x%lx\n", cpu_state->useresp);
    printf("  eip: ..................... 0x%lx\n", cpu_state->eip);
    printf("\n");
    printf("  cs: 0x%x, ds: 0x%x, ss: 0x%x\n", cpu_state->cs, cpu_state->ds, cpu_state->ss);
    printf("  es: 0x%x, fs: 0x%x, gs: 0x%x\n", cpu_state->es, cpu_state->fs, cpu_state->gs);
    printf("\n");
    printf("  Interrupt Number: ........ %u\n", int_no);
    printf("  Error Code: .............. %u\n", err_code);
    printf("  eflags: .................. 0x%lx\n", cpu_state->eflags);

    HALT();
}

void syscall(cpu_state_t* cpu_state)
{
    printf("Syscall number %lu\n", cpu_state->eax);
    if (cpu_state->eax == 0) {
        char* z = (char*)cpu_state->ebx;
        for (size_t i = 0; i < cpu_state->ecx; ++i)
            printf("%c", z[i]);
    }
}

void __attribute__((cdecl)) isr_common(cpu_state_t* cpu_state)
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
