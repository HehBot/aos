#include "interrupt.h"

#include <drivers/screen.h>

typedef struct {
    unsigned short low_offset;
    unsigned short seg;
    unsigned char always0;
    unsigned char flags;
    unsigned short high_offset;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256
idt_entry_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

static void set_idt_entry(unsigned int n, unsigned int isr)
{
    idt[n].low_offset = (isr & 0xffff);
    idt[n].seg = 0x8; // kernel code segment
    idt[n].always0 = 0;
    idt[n].flags = 0x8e;
    idt[n].high_offset = (isr >> 16);
}

static void set_idt()
{
    idt_reg.base = (unsigned int)&idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_entry_t) - 1;
    asm("lidt [%0]"
        :
        : "r"(&idt_reg));
}

#define D(z) void isr_##z(cpu_state_t)
D(0);
D(1);
D(2);
D(3);
D(4);
D(5);
D(6);
D(7);
D(8);
D(9);
D(10);
D(11);
D(12);
D(13);
D(14);
D(15);
D(16);
D(17);
D(18);
D(19);
D(20);
D(21);
D(22);
D(23);
D(24);
D(25);
D(26);
D(27);
D(28);
D(29);
D(30);
D(31);
void isr_install()
{
#define I(z) set_idt_entry(z, (unsigned int)isr_##z)
    I(0);
    I(1);
    I(2);
    I(3);
    I(4);
    I(5);
    I(6);
    I(7);
    I(8);
    I(9);
    I(10);
    I(11);
    I(12);
    I(13);
    I(14);
    I(15);
    I(16);
    I(17);
    I(18);
    I(19);
    I(20);
    I(21);
    I(22);
    I(23);
    I(24);
    I(25);
    I(26);
    I(27);
    I(28);
    I(29);
    I(30);
    I(31);
    set_idt();
}

void isr_common(cpu_state_t* cpu_state)
{
    char const* z = "0123456789ABCDEF";
    puts("Received interrupt 0x");
    int shift = 28;
    while (shift >= 0) {
        putc(z[(cpu_state->int_no >> shift) & 0xf]);
        shift -= 4;
    }
    putc('\n');
    char const* exception_messages[] = {
        "Division By Zero",
        "Debug",
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
    puts(exception_messages[cpu_state->int_no]);
    putc('\n');
}
