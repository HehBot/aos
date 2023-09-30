#include "interrupt.h"
#include "pic.h"

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define GATE_TYPE_INT 0xe
#define GATE_TYPE_TRAP 0xf

typedef struct {
    uint16_t low_offset;
    uint16_t seg;
    uint8_t always0_1;
    uint8_t gate_type : 4;
    uint8_t always0_2 : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    uint16_t high_offset;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256
static idt_entry_t idt[IDT_ENTRIES] = { 0 };
static idt_register_t idt_reg;

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

static inline void set_idt(void)
{
    idt_reg.base = (uint32_t)&idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_entry_t) - 1;
    lidt((uintptr_t)&idt_reg);
}

void isr_excep0(void);
void isr_excep1(void);
void isr_excep2(void);
void isr_excep3(void);
void isr_excep4(void);
void isr_excep5(void);
void isr_excep6(void);
void isr_excep7(void);
void isr_excep8(void);
void isr_excep9(void);
void isr_excep10(void);
void isr_excep11(void);
void isr_excep12(void);
void isr_excep13(void);
void isr_excep14(void);
void isr_excep15(void);
void isr_excep16(void);
void isr_excep17(void);
void isr_excep18(void);
void isr_excep19(void);
void isr_excep20(void);
void isr_excep21(void);
void isr_excep22(void);
void isr_excep23(void);
void isr_excep24(void);
void isr_excep25(void);
void isr_excep26(void);
void isr_excep27(void);
void isr_excep28(void);
void isr_excep29(void);
void isr_excep30(void);
void isr_excep31(void);
void isr_irq0(void);
void isr_irq1(void);
void isr_irq2(void);
void isr_irq3(void);
void isr_irq4(void);
void isr_irq5(void);
void isr_irq6(void);
void isr_irq7(void);
void isr_irq8(void);
void isr_irq9(void);
void isr_irq10(void);
void isr_irq11(void);
void isr_irq12(void);
void isr_irq13(void);
void isr_irq14(void);
void isr_irq15(void);
void isr_syscall(void);

void isr_install()
{
    // exception
    set_idt_entry(0, &isr_excep0, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(1, &isr_excep1, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(2, &isr_excep2, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(3, &isr_excep3, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(4, &isr_excep4, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(5, &isr_excep5, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(6, &isr_excep6, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(7, &isr_excep7, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(8, &isr_excep8, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(9, &isr_excep9, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(10, &isr_excep10, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(11, &isr_excep11, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(12, &isr_excep12, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(13, &isr_excep13, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(14, &isr_excep14, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(15, &isr_excep15, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(16, &isr_excep16, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(17, &isr_excep17, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(18, &isr_excep18, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(19, &isr_excep19, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(20, &isr_excep20, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(21, &isr_excep21, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(22, &isr_excep22, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(23, &isr_excep23, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(24, &isr_excep24, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(25, &isr_excep25, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(26, &isr_excep26, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(27, &isr_excep27, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(28, &isr_excep28, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(29, &isr_excep29, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(30, &isr_excep30, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(31, &isr_excep31, GATE_TYPE_INT, KERNEL_PL);

    // irq
    pic_remap(T_IRQ0, T_IRQ15);
    pic_set_mask(0x0000);
    set_idt_entry(T_IRQ0, &isr_irq0, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ1, &isr_irq1, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ2, &isr_irq2, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ3, &isr_irq3, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ4, &isr_irq4, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ5, &isr_irq5, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ6, &isr_irq6, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ7, &isr_irq7, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ8, &isr_irq8, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ9, &isr_irq9, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ10, &isr_irq10, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ11, &isr_irq11, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ12, &isr_irq12, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ13, &isr_irq13, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ14, &isr_irq14, GATE_TYPE_INT, KERNEL_PL);
    set_idt_entry(T_IRQ15, &isr_irq15, GATE_TYPE_INT, KERNEL_PL);

    // syscall
    set_idt_entry(T_SYSCALL, &isr_syscall, GATE_TYPE_TRAP, USER_PL);

    set_idt();
}

void excep(cpu_state_t* cpu_state)
{
    static char const* exception_messages[] = {
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

    printf("PANIC: %s detected\n\n", exception_messages[int_no]);
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
            printf("At address 0x%x: ", rcr2());
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

    printf("  eax: ..................... 0x%x\n", cpu_state->eax);
    printf("  ebx: ..................... 0x%x\n", cpu_state->ebx);
    printf("  ecx: ..................... 0x%x\n", cpu_state->ecx);
    printf("  edx: ..................... 0x%x\n", cpu_state->edx);
    printf("  edi: ..................... 0x%x\n", cpu_state->edi);
    printf("  esi: ..................... 0x%x\n", cpu_state->esi);
    printf("  ebp: ..................... 0x%x\n", cpu_state->ebp);
    printf("  useresp: ................. 0x%x\n", cpu_state->useresp);
    printf("  eip: ..................... 0x%x\n", cpu_state->eip);
    printf("\n");
    printf("  cs: 0x%x, ds: 0x%x, ss: 0x%x\n", cpu_state->cs, cpu_state->ds, cpu_state->ss);
    printf("  es: 0x%x, fs: 0x%x, gs: 0x%x\n", cpu_state->es, cpu_state->fs, cpu_state->gs);
    printf("\n");
    printf("  Interrupt Number: ........ %u\n", int_no);
    printf("  Error Code: .............. %u\n", err_code);
    printf("  eflags: .................. 0x%x\n", cpu_state->eflags);

    HALT();
}

static isr_t irq_handlers[T_IRQ15 - T_IRQ0 + 1] = { 0 };
void irq(cpu_state_t* cpu_state)
{
    size_t irq_no = cpu_state->int_no - T_IRQ0;
    pic_send_eoi(irq_no);

    if (irq_handlers[irq_no] != NULL) {
        isr_t handler = irq_handlers[irq_no];
        handler(cpu_state);
    }
}

void syscall(cpu_state_t* cpu_state)
{
    printf("Syscall number %d\n", cpu_state->eax);
    if (cpu_state->eax == 0) {
        char* z = (char*)cpu_state->ebx;
        for (size_t i = 0; i < cpu_state->ecx; ++i)
            printf("%c", z[i]);
    }
}

void register_irq_handler(size_t n, isr_t handler)
{
    if (n < T_IRQ0 || n > T_IRQ15)
        PANIC();
    irq_handlers[n - T_IRQ0] = handler;
}
