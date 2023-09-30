#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>

__attribute__((__aligned__(PAGE_SIZE))) uint32_t entry_pgdir[1024]
    = {
          [0] = (0 | PDE_LP | PTE_W | PTE_P),
          [PDX(KERN_BASE)] = (0 | PDE_LP | PTE_W | PTE_P),
      };

static gdt_entry_t gdt[]
    = {
          { 0 },
          [KERNEL_CODE_GDT_INDEX] = { 0xffff, 0x0, 0x0, 0, .read_write = 0, 0, .code = 1, .code_data_segment = 1, .dpl = 0, .present = 1, 0xf, 0, 0, 1, .gran = 1, 0x0 },
          [KERNEL_DATA_GDT_INDEX] = { 0xffff, 0x0, 0x0, 0, .read_write = 1, 0, .code = 0, .code_data_segment = 1, .dpl = 0, .present = 1, 0xf, 0, 0, 1, .gran = 1, 0x0 },
          [USER_CODE_GDT_INDEX] = { 0xffff, 0x0, 0x0, 0, .read_write = 0, 0, .code = 1, .code_data_segment = 1, .dpl = 3, .present = 1, 0xf, 0, 0, 1, .gran = 1, 0x0 },
          [USER_DATA_GDT_INDEX] = { 0xffff, 0x0, 0x0, 0, .read_write = 1, 0, .code = 0, .code_data_segment = 1, .dpl = 3, .present = 1, 0xf, 0, 0, 1, .gran = 1, 0x0 },
          [TSS_GDT_INDEX] = { 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
      };
gdt_desc_t gdt_desc = { sizeof(gdt) - 1, gdt };

extern void *kernel_code_start, *kernel_phy_code_start, *kernel_phy_code_end;
extern void *kernel_data_start, *kernel_phy_data_start, *kernel_phy_data_end;
extern void* kernel_heap_start;

mem_map_t kernel_mem_map[] = {
    { 1, 1, 0, (uintptr_t)&kernel_code_start, { { (uintptr_t)&kernel_phy_code_start, (uintptr_t)&kernel_phy_code_end } } },
    { 1, 1, PTE_W, (uintptr_t)&kernel_data_start, { { (uintptr_t)&kernel_phy_data_start, (uintptr_t)&kernel_phy_data_end } } },
    { 1, 0, PTE_W, (uintptr_t)&kernel_heap_start, { { INIT_HEAP_SZ, 0 } } },
    { 0 }
};
