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
static virt_addr_t map_temp(phys_addr_t pa)
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

    /*
     * we don't reserve frames here
     * since they are already reserved
     * as part of kernel frames
     */

    phys_addr_t phys_addr_p4 = read_cr3();
    p4 = kernel_static_from_phys_addr(phys_addr_p4);

    phys_addr_t phys_addr_init_p3 = PTE_FRAME(p4->entries[VA_P4_INDEX(&KERN_BASE)]);
    page_table_t* init_p3 = kernel_static_from_phys_addr(phys_addr_init_p3);

    phys_addr_t phys_addr_init_p2 = PTE_FRAME(init_p3->entries[VA_P3_INDEX(&KERN_BASE)]);
    page_table_t* init_p2 = kernel_static_from_phys_addr(phys_addr_init_p2);

    pte_t* pte = &init_p2->entries[VA_P2_INDEX(buf_page)];
    if ((*pte) & PTE_P)
        PANIC("Paging buffer page is already mapped");
    *pte = PTE(phys_addr_of_kernel_static(&map_temp_p1), PTE_W | PTE_P);

    /*
     * remove kernel identity mapping
     */
    p4->entries[0] = init_p3->entries[0] = 0;

    flush_tlb_all();
}

static int page_walk(virt_addr_t addr, page_table_level_t level, page_table_t** ret)
{
    page_table_level_t curr_level = PAGE_TABLE_P4;
    page_table_t* curr_page_table = p4;
    while (1) {
        if (curr_level == level)
            break;

        pte_t* pte = &curr_page_table->entries[VA_PT_INDEX(addr, curr_level)];

        if (((*pte) & PTE_P) == 0)
            return PAGING_ERROR_PAGE_NOT_MAPPED;
        else if ((*pte) & PTE_HP)
            return PAGING_ERROR_PARENT_ENTRY_MARKED_HP;

        phys_addr_t pa = PTE_FRAME(*pte);

        curr_page_table = map_temp(pa);

        curr_level--;
    }
    *ret = curr_page_table;
    return PAGING_OK;
}
static noignore int get_pte(virt_addr_t page, page_type_t type, pte_t** ret)
{
    page_table_level_t parent_level = page_type_parent_table_level(type);
    page_table_t* parent_table;
    int err = page_walk(page, parent_level, &parent_table);
    if (err != PAGING_OK)
        return err;

    pte_t* pte = &parent_table->entries[VA_PT_INDEX(page, parent_level)];
    if ((((*pte) & PTE_P) == 0) || (type != PAGE_4KiB && (((*pte) & PTE_HP) == 0)))
        return PAGING_ERROR_PAGE_NOT_MAPPED;

    *ret = pte;
    return PAGING_OK;
}

int paging_map_with_table_flags(virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags, pte_flags_t parent_table_flags)
{
    page_table_level_t parent_level = page_type_parent_table_level(type);

    parent_table_flags &= ~PTE_HP;

    page_table_level_t curr_level = PAGE_TABLE_P4;
    page_table_t* curr_page_table = p4;
    while (1) {
        if (curr_level == parent_level)
            break;

        int clear_new_table = 0;

        pte_t* pte = &curr_page_table->entries[VA_PT_INDEX(page, curr_level)];
        if (((*pte) & PTE_P) == 0) {
            phys_addr_t new_frame = frame_allocator_get_frame();
            if (new_frame == FRAME_ALLOCATOR_ERROR_NO_FRAME_AVAILABLE)
                return PAGING_ERROR_FRAME_ALLOCATION_FAILURE;

            *pte = PTE(new_frame, (parent_table_flags | PTE_P | PTE_W));
            clear_new_table = 1;
        } else if ((*pte) & PTE_HP)
            return PAGING_ERROR_PARENT_ENTRY_MARKED_HP;

        phys_addr_t pa = PTE_FRAME(*pte);
        curr_page_table = map_temp(pa);

        if (clear_new_table)
            memset(curr_page_table, 0, sizeof(*curr_page_table));

        curr_level--;
    }
    pte_t* pte = &curr_page_table->entries[VA_PT_INDEX(page, parent_level)];
    if ((*pte) & PTE_P)
        return PAGING_ERROR_PAGE_ALREADY_MAPPED;

    *pte = PTE(frame, flags | PTE_P | (type == PAGE_4KiB ? 0 : PTE_HP));
    flush_tlb(page);

    return PAGING_OK;
}

int paging_map(virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags)
{
    return paging_map_with_table_flags(page, frame, type, flags, PTE_W | PTE_P);
}

phys_addr_t paging_unmap(virt_addr_t page, page_type_t type)
{
    pte_t* pte;
    int err = get_pte(page, type, &pte);
    if (err != PAGING_OK)
        return err;

    phys_addr_t frame = PTE_FRAME(*pte);
    *pte = 0;

    flush_tlb(page);
    return frame;
}

phys_addr_t paging_translate_page(virt_addr_t page, page_type_t type)
{
    pte_t* pte;
    int err = get_pte(page, type, &pte);
    if (err != PAGING_OK)
        return err;

    return PTE_FRAME(*pte);
}

noignore int paging_update_flags(virt_addr_t page, page_type_t type, pte_flags_t flags)
{
    pte_t* pte;
    int err = get_pte(page, type, &pte);
    if (err != PAGING_OK)
        return err;

    phys_addr_t frame = PTE_FRAME(*pte);
    *pte = PTE(frame, flags | PTE_P | (type == PAGE_4KiB ? 0 : PTE_HP));
    flush_tlb(page);

    return PAGING_OK;
}
