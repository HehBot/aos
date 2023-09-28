#include "interrupt.h"
#include "pic.h"

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint16_t low_offset;
    uint16_t seg;
    uint8_t always0;
    uint8_t flags;
    uint16_t high_offset;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256
static idt_entry_t idt[IDT_ENTRIES];
static idt_register_t idt_reg;
static isr_t isr_handlers[IDT_ENTRIES];

static void set_idt_entry(uint32_t n, uint32_t isr)
{
    idt[n].low_offset = (isr & 0xffff);
    idt[n].seg = 0x8; // kernel code segment
    idt[n].always0 = 0;
    idt[n].flags = 0x8e;
    idt[n].high_offset = (isr >> 16);
}

static void set_idt()
{
    idt_reg.base = (uint32_t)&idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_entry_t) - 1;
    lidt((uintptr_t)&idt_reg);
}

#define D_excep(z) void isr_##z##_excep(cpu_state_t)
D_excep(0);
D_excep(1);
D_excep(2);
D_excep(3);
D_excep(4);
D_excep(5);
D_excep(6);
D_excep(7);
D_excep(8);
D_excep(9);
D_excep(10);
D_excep(11);
D_excep(12);
D_excep(13);
D_excep(14);
D_excep(15);
D_excep(16);
D_excep(17);
D_excep(18);
D_excep(19);
D_excep(20);
D_excep(21);
D_excep(22);
D_excep(23);
D_excep(24);
D_excep(25);
D_excep(26);
D_excep(27);
D_excep(28);
D_excep(29);
D_excep(30);
D_excep(31);
#define D_irq(z) void isr_##z##_irq(cpu_state_t)
D_irq(32);
D_irq(33);
D_irq(34);
D_irq(35);
D_irq(36);
D_irq(37);
D_irq(38);
D_irq(39);
D_irq(40);
D_irq(41);
D_irq(42);
D_irq(43);
D_irq(44);
D_irq(45);
D_irq(46);
D_irq(47);
void isr_install()
{
    for (size_t i = 0; i < IDT_ENTRIES; ++i)
        isr_handlers[i] = 0;

#define I_excep(z) set_idt_entry(z, (uint32_t)isr_##z##_excep)
    I_excep(0);
    I_excep(1);
    I_excep(2);
    I_excep(3);
    I_excep(4);
    I_excep(5);
    I_excep(6);
    I_excep(7);
    I_excep(8);
    I_excep(9);
    I_excep(10);
    I_excep(11);
    I_excep(12);
    I_excep(13);
    I_excep(14);
    I_excep(15);
    I_excep(16);
    I_excep(17);
    I_excep(18);
    I_excep(19);
    I_excep(20);
    I_excep(21);
    I_excep(22);
    I_excep(23);
    I_excep(24);
    I_excep(25);
    I_excep(26);
    I_excep(27);
    I_excep(28);
    I_excep(29);
    I_excep(30);
    I_excep(31);

    PIC_remap(32, 40);
    PIC_set_mask(0x0000);
#define I_irq(z) set_idt_entry(z, (uint32_t)isr_##z##_irq)
    I_irq(32);
    I_irq(33);
    I_irq(34);
    I_irq(35);
    I_irq(36);
    I_irq(37);
    I_irq(38);
    I_irq(39);
    I_irq(40);
    I_irq(41);
    I_irq(42);
    I_irq(43);
    I_irq(44);
    I_irq(45);
    I_irq(46);
    I_irq(47);

    set_idt();
}

void isr_excep_common(cpu_state_t* cpu_state)
{
    char const* exception_messages[] = {
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
    printf("\
Received exception %u (error code %u)\n\
    %s\n\
cs = 0x%x  ds = 0x%x  ss = 0x%x\n\
eip = 0x%x  eflags = 0x%x  esp = 0x%x  ebp = 0x%x\n\
edi = 0x%x  esi = 0x%x  edx = 0x%x  ebx = 0x%x  ecx = 0x%x  eax = 0x%x\n\
",
           cpu_state->int_no, cpu_state->err_code,
           exception_messages[cpu_state->int_no],
           cpu_state->cs, cpu_state->ds, cpu_state->ss,
           cpu_state->eip, cpu_state->eflags, cpu_state->useresp, cpu_state->ebp,
           cpu_state->edi, cpu_state->esi, cpu_state->edx, cpu_state->ebx, cpu_state->ecx, cpu_state->eax);
    if (cpu_state->int_no == 14)
        printf("Page fault at address 0x%x\n", rcr2());

    asm("cli\r\nhlt");
}

void isr_irq_common(cpu_state_t* cpu_state)
{
    PIC_send_EOI(cpu_state->int_no - 32);

    if (isr_handlers[cpu_state->int_no] != 0) {
        isr_t handler = isr_handlers[cpu_state->int_no];
        handler(cpu_state);
    }
}

void register_isr_handler(uint8_t n, isr_t handler)
{
    isr_handlers[n] = handler;
}
