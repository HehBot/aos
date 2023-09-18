#include "pmm.h"
#include "string.h"

#include <stddef.h>
#include <stdint.h>
#include <x86.h>

page_directory_t* current_dir;

void switch_page_directory(page_directory_t* dir)
{
    current_dir = dir;
    asm("mov cr3, %0"
        :
        : "r"(dir->physical_addr));
}

// Address 0xffc00000 contains a page table for the address range 0xffc00000 - 0xffffffff
// for every page table in use
static page_table_t* const meta_page_table = (void*)0xffc00000;
static void* get_page_for_pgtb(uintptr_t* phy_ptr)
{
    for (int i = 1; i < 1024; ++i) {
        if (!(meta_page_table->pages[i] & PTE_P)) {
            uintptr_t phy = pmm_get_frame();
            meta_page_table->pages[i] = phy | PTE_W | PTE_P;
            *phy_ptr = phy;
            return (void*)((uintptr_t)meta_page_table + PAGE_SIZE * i);
        }
    }
    // not reachable as entire address space can be mapped by 1024 page tables
    return NULL;
}
static pte_t* get_pte(page_directory_t* dir, uintptr_t va, int make)
{
    va >>= 12;
    size_t pde_no = va >> 10;
    size_t pte_no = va & 0x3ff;
    if (dir->tables[pde_no])
        return &dir->tables[pde_no]->pages[pte_no];
    else if (make) {
        uintptr_t phy;
        page_table_t* pgtb = get_page_for_pgtb(&phy);
        dir->tables[pde_no] = pgtb;
        dir->tables_physical[pde_no] = phy | PTE_W | PTE_P;
        memset(pgtb, 0, PAGE_SIZE);
        return &pgtb->pages[pte_no];
    } else
        return NULL;
}

// map page at given virt addr to given phy addr with given flags
int map_page(page_directory_t* dir, uintptr_t pa, uintptr_t va, uint8_t flags)
{
    pte_t* pte = get_pte(dir, va, 1);
    if (*pte & PTE_P)
        return 0;
    *pte = (pa & 0xfffff000) | flags | PTE_P;
    return 1;
}

// unmap page at given virt addr and free corresponding frame
int unmap_page(page_directory_t* dir, uintptr_t va)
{
    pte_t* pte = get_pte(dir, va, 0);
    if (!pte || !(*pte & PTE_P))
        return 0;
    uintptr_t phys = (*pte) & 0xfffff000;
    pmm_free_frame(phys);
    *pte = 0;
    return 1;
}
