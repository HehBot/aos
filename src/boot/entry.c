#include <stdint.h>
#include <x86.h>

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
