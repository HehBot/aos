#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>

__attribute__((__aligned__(PAGE_SIZE))) pde_t entry_pgdir[1024]
    = {
          [0] = (0 | PTE_LP | PTE_W | PTE_P),
          [KERN_BASE >> 22] = (0 | PTE_LP | PTE_W | PTE_P),
      };

static gdt_entry_t gdt[]
    = {
          { 0 },
          { 0xffff, 0x0, 0x0, 0x9a, 0xcf, 0x0 },
          { 0xffff, 0x0, 0x0, 0x92, 0xcf, 0x0 },
      };
gdt_desc_t entry_gdt_desc = { sizeof(gdt) - 1, gdt };

extern void *kernel_code_start, *kernel_phy_code_start, *kernel_phy_code_end;
extern void *kernel_data_start, *kernel_phy_data_start, *kernel_phy_data_end;
extern void* kernel_heap_start;

mem_map_t kernel_mem_map[] = {
    { 1, 1, 0, (uintptr_t)&kernel_code_start, { { (uintptr_t)&kernel_phy_code_start, (uintptr_t)&kernel_phy_code_end } } },
    { 1, 1, PTE_W, (uintptr_t)&kernel_data_start, { { (uintptr_t)&kernel_phy_data_start, (uintptr_t)&kernel_phy_data_end } } },
    { 1, 0, PTE_W, (uintptr_t)&kernel_heap_start, { { 1, 0 } } },
    { 0 }
};
