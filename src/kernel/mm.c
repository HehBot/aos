#include "mm.h"

#include "kmalloc.h"
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
page_table_t* meta_page_table = (void*)0xffc00000;
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
pte_t* get_pte(page_directory_t* dir, uintptr_t virt_addr, int make)
{
    virt_addr >>= 12;
    size_t table_no = virt_addr >> 10;
    if (dir->tables[table_no])
        return &dir->tables[table_no]->pages[virt_addr & 0x3ff];
    else if (make) {
        uintptr_t phy;
        void* pgtb = get_page_for_pgtb(&phy);
        dir->tables[table_no] = pgtb;
        dir->tables_physical[table_no] = phy | PTE_W | PTE_P;
        memset(dir->tables[table_no], 0, PAGE_SIZE);
        return &dir->tables[table_no]->pages[virt_addr & 0x3ff];
    } else
        return NULL;
}
