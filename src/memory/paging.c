#include "frame_allocator.h"
#include "page.h"
#include "paging.h"

#include <cpu/x86.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct page_table {
    pte_t entries[NR_PTE_ENTRIES];
} __attribute__((__aligned__(PAGE_SIZE))) page_table_t;

static page_table_t map_temp_p1;
static virt_addr_t buf_page = (virt_addr_t)0xfffffffffffff000;

static page_table_t* p4;

virt_addr_t map_temp(phys_addr_t pa)
{
    map_temp_p1.entries[VA_P1_INDEX(buf_page)] = PTE(pa, PTE_W | PTE_P);

    flush_tlb(buf_page);

    return buf_page;
}

void init_paging()
{
    extern void* KERN_BASE;
    if (VA_P4_INDEX(&KERN_BASE) != VA_P4_INDEX(buf_page)
        || VA_P3_INDEX(&KERN_BASE) != VA_P3_INDEX(buf_page))
        PANIC("Bad buf_page address");

    phys_addr_t phys_addr_p4 = read_cr3();
    pmm_reserve_frame(phys_addr_p4);
    p4 = kernel_static_from_phys_addr(phys_addr_p4);

    phys_addr_t phys_addr_init_p3 = PTE_FRAME(p4->entries[VA_P4_INDEX(&KERN_BASE)]);
    pmm_reserve_frame(phys_addr_init_p3);
    page_table_t* init_p3 = kernel_static_from_phys_addr(phys_addr_init_p3);

    phys_addr_t phys_addr_init_p2 = PTE_FRAME(init_p3->entries[VA_P3_INDEX(&KERN_BASE)]);
    pmm_reserve_frame(phys_addr_init_p2);
    page_table_t* init_p2 = kernel_static_from_phys_addr(phys_addr_init_p2);

    pte_t* pte = &init_p2->entries[VA_P2_INDEX(buf_page)];
    if ((*pte) & PTE_P)
        PANIC("Paging buffer page is already mapped");
    *pte = PTE(phys_addr_of_kernel_static(&map_temp_p1), PTE_W | PTE_P);

    p4->entries[0] = init_p3->entries[0] = 0;

    flush_tlb_all();
}

page_table_t* page_walk(virt_addr_t addr, page_table_level_t level)
{
    page_table_level_t curr_level = PAGE_TABLE_P4;
    page_table_t* curr_page_table = p4;
    while (1) {
        if (curr_level == level)
            break;

        pte_t* pte = &curr_page_table->entries[VA_PT_INDEX(addr, curr_level)];
        if (((*pte) & PTE_P) == 0) // absent
            return NULL;
        /*
        else if ((*pte) & PTE_HP)   // parent entry is huge page
            return NULL;
        */
        phys_addr_t pa = PTE_FRAME(*pte);

        curr_page_table = map_temp(pa);

        curr_level--;
    }
    return curr_page_table;
}

int map_to_with_table_flags(virt_addr_t page, phys_addr_t frame, pte_flags_t flags, pte_flags_t parent_table_flags)
{
    virt_addr_t addr = page;

    page_table_level_t parent_level = PAGE_TABLE_P1;

    /*
    parent_table_flags &= ~PTE_HP;
    */

    page_table_level_t curr_level = PAGE_TABLE_P4;
    page_table_t* curr_page_table = p4;
    while (1) {
        if (curr_level == parent_level)
            break;

        int clear = 0;

        pte_t* pte = &curr_page_table->entries[VA_PT_INDEX(addr, curr_level)];
        if (((*pte) & PTE_P) == 0) {
            phys_addr_t new_frame = pmm_get_frame(); // frame allocation failure
            *pte = PTE(new_frame, (parent_table_flags | PTE_P | PTE_W));
            clear = 1;
        } /* else if ((*pte) & PTE_HP)   // parent entry is huge page
             return 0;
         */

        phys_addr_t pa = PTE_FRAME(*pte);
        curr_page_table = map_temp(pa);
        if (clear)
            memset(curr_page_table, 0, sizeof(*curr_page_table));

        curr_level--;
    }
    pte_t* pte = &curr_page_table->entries[VA_PT_INDEX(addr, parent_level)];
    if ((*pte) & PTE_P) // page is already mapped
        return 0;
    *pte = PTE(frame, flags | PTE_P);

    flush_tlb(page);

    return 1;
}

int map_to(virt_addr_t page, phys_addr_t frame, pte_flags_t flags)
{
    return map_to_with_table_flags(page, frame, flags, PTE_W | PTE_P);
}
