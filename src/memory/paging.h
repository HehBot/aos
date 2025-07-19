#ifndef PAGING_H
#define PAGING_H

#include "page.h"

#include <bootinfo.h>
#include <stddef.h>
#include <stdint.h>

void init_paging(section_info_t* section_info, size_t nr_sections);
pgdir_t paging_new_pgdir();

enum {
    PAGING_OK = 0,
    PAGING_ERROR_FRAME_ALLOCATION_FAILURE,
    PAGING_ERROR_PARENT_ENTRY_MARKED_HP,
    PAGING_ERROR_PAGE_ALREADY_MAPPED,
    PAGING_ERROR_PAGE_NOT_MAPPED,
    PAGING_ERROR = 0xff,
};

/*
 * XXX all `page`,`frame` arguments must be appropriately
 *      aligned according to specified `type`
 * XXX we check and enforce that relevant VA is not kernel-owned
 */
noignore int paging_map_with_table_flags(pgdir_t pgdir, virt_addr_t page, phys_addr_t frame,
                                         page_type_t type, pte_flags_t flags, pte_flags_t parent_table_flags);
noignore int paging_map(pgdir_t pgdir, virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags);
noignore phys_addr_t paging_unmap(pgdir_t pgdir, virt_addr_t page, page_type_t type);
noignore phys_addr_t paging_translate_page(pgdir_t pgdir, virt_addr_t page, page_type_t type);
pte_t paging_show_pte(pgdir_t pgdir, virt_addr_t page, page_type_t type);
noignore int paging_update_flags(pgdir_t pgdir, virt_addr_t page, page_type_t type, pte_flags_t flags);
void switch_pgdir(pgdir_t pgdir);

/*
 * XXX all `page`,`frame` arguments must be appropriately
 *      aligned according to specified `type`
 * XXX we check and enforce that relevant VA is kernel-owned
 */
noignore int paging_kernel_map_with_table_flags(virt_addr_t page, phys_addr_t frame,
                                                page_type_t type, pte_flags_t flags, pte_flags_t parent_table_flags);
noignore int paging_kernel_map(virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags);
noignore phys_addr_t paging_kernel_unmap(virt_addr_t page, page_type_t type);
noignore phys_addr_t paging_kernel_translate_page(virt_addr_t page, page_type_t type);
pte_t paging_kernel_show_pte(virt_addr_t page, page_type_t type);
noignore int paging_kernel_update_flags(virt_addr_t page, page_type_t type, pte_flags_t flags);
void switch_kernel_pgdir();

static inline phys_addr_t phys_addr_of_kernel_static(virt_addr_t v)
{
    extern void* KERN_BASE;
    virt_addr_t kb = &KERN_BASE;
    return (v - kb);
}
static inline virt_addr_t kernel_static_from_phys_addr(phys_addr_t p)
{
    extern void* KERN_BASE;
    virt_addr_t kb = &KERN_BASE;
    return (kb + p);
}

#endif // PAGING_H
