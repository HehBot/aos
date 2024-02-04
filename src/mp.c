#include <mp.h>

cpu_t cpus[MAX_CPUS];
size_t nr_cpus = 0;
uint32_t volatile* lapic;

uint8_t ioapic_id;
ioapic_t volatile* ioapic;
