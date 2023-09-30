#ifndef PROC_H
#define PROC_H

void init_tss(void);
void set_tss(void* kstack);
void enter_usermode(cpu_state_t* c);

#endif // PROC_H
