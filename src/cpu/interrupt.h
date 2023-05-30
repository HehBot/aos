#ifndef INTERRUPT_H
#define INTERRUPT_H

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

typedef struct {
    unsigned int ds;

    unsigned int edi, esi, ebp, useless_esp, ebx, edx, ecx, eax;

    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
} __attribute__((packed)) cpu_state_t;

typedef void (*isr_t)(cpu_state_t*);
void register_isr_handler(unsigned char n, isr_t handler);

#endif // INTERRUPT_H
