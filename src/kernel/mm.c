#include "kpalloc.h"
#include "pmm.h"
#include "string.h"

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>

page_directory_t kernel_pgdir __attribute__((__aligned__(PAGE_SIZE)));

page_directory_t* dir;

void switch_page_directory(page_directory_t* d)
{
    dir = d;
    lcr3(d->physical_addr);
}

static inline pte_t* get_pte(page_directory_t* d, uintptr_t va, int make)
{
    size_t pde_no = PDX(va);
    size_t pte_no = PTX(va);
    if (d->tables[pde_no])
        return &d->tables[pde_no]->pages[pte_no];
    else if (make) {
        page_table_t* pgtb = kpalloc(1);
        uintptr_t phy = *get_pte(d, (uintptr_t)pgtb, 0);
        d->tables[pde_no] = pgtb;
        d->tables_physical[pde_no] = phy | PTE_W | PTE_P;
        memset(pgtb, 0, PAGE_SIZE);
        return &pgtb->pages[pte_no];
    } else
        return NULL;
}

// map page at given virt addr to given phy addr with given flags
int map_page(uintptr_t pa, uintptr_t va, uint8_t flags)
{
    pte_t* pte = get_pte(dir, va, 1);
    if (*pte & PTE_P)
        return 0;
    *pte = (pa & 0xfffff000) | flags | PTE_P;
    invlpg(va);
    return 1;
}

// remap page at given virt addr to given phy addr with given flags
int remap_page(uintptr_t pa, uintptr_t va, uint8_t flags)
{
    pte_t* pte = get_pte(dir, va, 0);
    if (!(*pte & PTE_P))
        return 0;
    pmm_free_frame((*pte) & 0xfffff000);
    *pte = (pa & 0xfffff000) | flags | PTE_P;
    invlpg(va);
    return 1;
}

// unmap page at given virt addr and free corresponding frame
int unmap_page(uintptr_t va)
{
    pte_t* pte = get_pte(dir, va, 0);
    if (!pte || !(*pte & PTE_P))
        return 0;
    uintptr_t phys = (*pte) & 0xfffff000;
    pmm_free_frame(phys);
    *pte = 0;
    invlpg(va);
    return 1;
}

extern pde_t entry_pgdir[];
extern mem_map_t kernel_mem_map[];
extern size_t kernel_mem_map_entries;

void init_mm(void)
{
    memset(&kernel_pgdir, 0, sizeof(kernel_pgdir));
    kernel_pgdir.physical_addr = ((uintptr_t)kernel_pgdir.tables_physical - KERN_BASE);

    // 1. set up meta page table in current pgdir and kernel pgdir
    uintptr_t pgtb_phy = pmm_get_frame();
    entry_pgdir[0x3ff] = LPG_ROUND_DOWN(pgtb_phy) | PTE_LP | PTE_W | PTE_P;
    page_table_t* pgtb = (void*)(0xffc00000 + (pgtb_phy - LPG_ROUND_DOWN(pgtb_phy)));
    memset(pgtb->pages, 0, sizeof(pgtb->pages));
    pgtb->pages[0] = pgtb_phy | PTE_W | PTE_P;
    entry_pgdir[0x3ff] = pgtb_phy | PTE_W | PTE_P;
    pgtb = (void*)0xffc00000;

    // 2. map kernel mappings
    for (size_t i = 0; kernel_mem_map[i].present; ++i) {
        uint8_t perms = kernel_mem_map[i].perms | PTE_P;
        if (kernel_mem_map[i].mapped) {
            for (uintptr_t virt = kernel_mem_map[i].virt, phy = kernel_mem_map[i].phy_start; phy < kernel_mem_map[i].phy_end; virt += PAGE_SIZE, phy += PAGE_SIZE)
                pgtb->pages[PTX(virt)] = (phy & 0xfffff000) | perms;
        } else {
            uintptr_t ul = kernel_mem_map[i].virt + kernel_mem_map[i].nr_pages * PAGE_SIZE;
            for (uintptr_t virt = kernel_mem_map[i].virt; virt < ul; virt += PAGE_SIZE)
                pgtb->pages[PTX(virt)] = (pmm_get_frame() & 0xfffff000) | perms;
        }
    }

    size_t pdx = PDX(kernel_mem_map[0].virt);

    uintptr_t entry_pgdir_frame = (pgtb->pages[PTX((uintptr_t)entry_pgdir)] & 0xfffff000);
    pgtb->pages[PTX((uintptr_t)entry_pgdir)] = pgtb_phy | PTE_W | PTE_P;

    // 3. add meta page table in kernel mapping
    // mapping to same virtual address as entry_pgdir
    kernel_pgdir.tables[pdx] = (void*)entry_pgdir;
    kernel_pgdir.tables_physical[pdx] = pgtb_phy | PTE_W | PTE_P;
    pgtb->pages[0] = 0;
    pgtb = (void*)entry_pgdir;

    // 4. switch to kpgdir and free entry_pgdir frame
    switch_page_directory(&kernel_pgdir);
    pmm_free_frame(entry_pgdir_frame);
}
