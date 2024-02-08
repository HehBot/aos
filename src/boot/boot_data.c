#include <cpu/x86.h>
#include <stdint.h>

__attribute__((__aligned__(PAGE_SIZE))) uint32_t entry_pgdir[1024]
    = {
          [0] = (0 | PDE_LP | PTE_W | PTE_P),
          [PDX(KERN_BASE)] = (0 | PDE_LP | PTE_W | PTE_P),
      };

static gdt_entry_t entry_gdt[]
    = {
          { 0 },
          [KERNEL_CODE_GDT_INDEX] = SEG(KERNEL_PL, 1, 0, 0x0, 0xffffffff),
          [KERNEL_DATA_GDT_INDEX] = SEG(KERNEL_PL, 0, 1, 0x0, 0xffffffff),
      };
gdt_desc_t entry_gdt_desc = { sizeof(entry_gdt) - 1, entry_gdt };

extern void *kernel_code_start, *kernel_phy_code_start, *kernel_phy_code_end;
extern void *kernel_data_start, *kernel_phy_data_start, *kernel_phy_data_end;
extern void* kernel_heap_start;

mem_map_t kernel_mem_map[] = {
    { 1, 1, 0, (uintptr_t)&kernel_code_start, { { (uintptr_t)&kernel_phy_code_start, (uintptr_t)&kernel_phy_code_end } } },
    { 1, 1, PTE_W, (uintptr_t)&kernel_data_start, { { (uintptr_t)&kernel_phy_data_start, (uintptr_t)&kernel_phy_data_end } } },
    { 1, 0, PTE_W, (uintptr_t)&kernel_heap_start, { { INIT_HEAP_SZ, 0 } } },
    { 0 }
};
