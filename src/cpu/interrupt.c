#include <cpu/mp.h>
#include <cpu/x86.h>
#include <drivers/ega.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define GATE_TYPE_INT 0xe
#define GATE_TYPE_TRAP 0xf

#define NR_ISRS 256
static idt_entry_t idt[NR_ISRS] = { 0 };
extern void (*(isrs[256]))(void);

static inline void set_idt_entry(size_t n, void (*isr)(void), uint8_t gate_type, uint8_t dpl, uint8_t hw_ist_index)
{
    uintptr_t addr = (uintptr_t)isr;
    idt[n] = (idt_entry_t) {
        .low_offset = (addr & 0xffff),
        .ist = hw_ist_index,
        .seg = KERNEL_CODE_SEG,
        .gate_type = gate_type,
        .dpl = dpl,
        .present = 1,
        .mid_offset = (addr >> 16) & 0xffff,
        .high_offset = (addr >> 32)
    };
}

void init_idt()
{
    for (size_t i = 0; i < NR_ISRS; ++i)
        set_idt_entry(i, isrs[i], GATE_TYPE_INT, KERNEL_PL, INTERRUPT_IST + 1);
}

void load_idt()
{
    lidt(&idt[0], sizeof(idt));
}

void excep(context_t* context)
{
    static char const* exception_messages[32] = {
        "Division By Zero",
        "Debug Panic",
        "Non Maskable Interrupt",
        [T_BREAKPOINT] = "Breakpoint",
        "Into Detected Overflow",
        "Out of Bounds",
        "Invalid Opcode",
        "No Coprocessor",

        [T_DOUBLE_FAULT] = "Double Fault",
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

    int int_no = context->int_no, err_code = context->err_code;

    if (int_no == T_BREAKPOINT)
        return;

    char err_code_msg[100] = {};
    int end = 0;
    int const len = sizeof(err_code_msg);

    switch (int_no) {
    case 13:
        // https://wiki.osdev.org/Exceptions#Selector_Error_Code
        if (err_code != 0) {
            if (err_code & 0x1)
                end += snprintf(&err_code_msg[end], len - end, "External");
            else {
                end += snprintf(&err_code_msg[end], len - end, "Internal: ");
                switch ((err_code >> 1) & 0x3) {
                case 0:
                    end += snprintf(&err_code_msg[end], len - end, "GDT");
                    break;
                case 1:
                case 3:
                    end += snprintf(&err_code_msg[end], len - end, "IDT");
                    break;
                case 2:
                    end += snprintf(&err_code_msg[end], len - end, "LDT");
                    break;
                }
                end += snprintf(&err_code_msg[end], len - end, " index %u\n", err_code >> 3);
            }
        }
        break;
    case 14:
        // https://wiki.osdev.org/Exceptions#Page_Fault
        end += snprintf(&err_code_msg[end], len - end, "At address %p:\n  ", read_cr2());
        if (err_code & PTE_P)
            end += snprintf(&err_code_msg[end], len - end, "protection violation, ");
        else
            end += snprintf(&err_code_msg[end], len - end, "page not present, ");
        if (err_code & PTE_W)
            end += snprintf(&err_code_msg[end], len - end, "attempted write");
        else if (err_code & 0x10)
            end += snprintf(&err_code_msg[end], len - end, "attempted execution");
        else
            end += snprintf(&err_code_msg[end], len - end, "attempted read");
        if (err_code & PTE_U)
            end += snprintf(&err_code_msg[end], len - end, " from user mode");
        if (err_code & 0x8)
            end += snprintf(&err_code_msg[end], len - end, "\n  (reserved bit set in a PTE)");
        break;
    }

    PANIC("\
Exception: %s\n\
%s\n\
Registers at exception:\n\
  rax: 0x%lx rbx: 0x%lx rcx: 0x%lx rdx: 0x%lx\n\
  rdi: 0x%lx rsi: 0x%lx rbp: 0x%lx rsp: 0x%lx\n\
  r8 : 0x%lx r9 : 0x%lx r10: 0x%lx r11: 0x%lx\n\
  r12: 0x%lx r13: 0x%lx r14: 0x%lx r15: 0x%lx\n\
\n\
  rip: 0x%lx cs: 0x%hx ss: 0x%hx\n\
\n\
  Interrupt Number: ........ %u\n\
  Error Code: .............. 0x%x\n\
  rflags: .................. 0x%lx\n\
",
          exception_messages[int_no],
          err_code_msg,
          context->rax, context->rbx, context->rcx, context->rdx,
          context->rdi, context->rsi, context->rbp, context->rsp,
          context->r8, context->r9, context->r10, context->r11,
          context->r12, context->r13, context->r14, context->r15,
          context->rip, context->cs, context->ss,
          int_no, err_code, context->rflags);
}

context_t* schedule_next(context_t* context);

void isr_common(context_t* context)
{
    int int_no = context->int_no;
    void keyboard_callback(context_t*);
    if (int_no < 32)
        excep(context);
    else {
        switch (int_no) {
        case T_IRQ0 + IRQ_KBD:
            keyboard_callback(context);
            lapic_eoi();
            break;
        case T_IRQ0 + IRQ_SPUR:
            printf("SPURIOUS!\n");
            break;
        case T_IRQ0 + IRQ_TIMER:
            printf("<TIMER>");
            schedule_next(context);
            lapic_eoi();
            break;
        default:
            printf("Unregistered ISR %d\n", int_no);
        }
    }
}
