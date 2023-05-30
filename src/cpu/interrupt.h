#ifndef INTERRUPT_H
#define INTERRUPT_H

typedef struct {
    unsigned int ds;

    unsigned int edi, esi, ebp, useless_esp, ebx, edx, ecx, eax;

    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
} __attribute__((packed)) cpu_state_t;

#endif // INTERRUPT_H
