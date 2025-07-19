#include "frame_allocator.h"
#include "kalloc.h"
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

static virt_addr_t map_temp(phys_addr_t pa)
{
    map_temp_p1.entries[VA_P1_INDEX(buf_page)] = PTE(pa, PTE_W | PTE_P);
    flush_tlb(buf_page);
    return buf_page;
}

static pgdir_t kpgdir;

static int kern_pgdir_setup = 0;
static section_info_t kern_sections[4];
static size_t nr_kern_sections;

static int is_va_kernel_managed(virt_addr_t addr)
{
    extern void* KERN_BASE;
    int index = VA_P4_INDEX(addr);
    return (index == 256 || index == VA_P4_INDEX(&KERN_BASE));
}

pgdir_t paging_new_pgdir()
{
    if (!kern_pgdir_setup) {
        extern void* KERN_BASE;

        pgdir_t new_pgdir;

        new_pgdir.p4 = kpalloc(1);
        memset(new_pgdir.p4, 0, sizeof(*new_pgdir.p4));

        /*
         * add heap mapping
         * these are owned by the kernel, hence no PTE_U
         */
        new_pgdir.p4->entries[256] = PTE(PTE_FRAME(kpgdir.p4->entries[256]), PTE_W | PTE_P);

        /*
         * add kernel mapping
         * these are owned by the kernel, hence no PTE_U
         */
        phys_addr_t phys_addr_p3 = frame_allocator_get_frame();
        ASSERT((phys_addr_p3 & FRAME_ALLOCATOR_ERROR) == 0);
        new_pgdir.p4->entries[VA_P4_INDEX(&KERN_BASE)] = PTE(phys_addr_p3, PTE_W | PTE_P);

        /*
         * PTE_U will be included in every other level
         */
        page_table_t* p3 = map_temp(phys_addr_p3);
        memset(p3, 0, sizeof(*p3));
        phys_addr_t phys_addr_p2 = frame_allocator_get_frame();
        ASSERT((phys_addr_p2 & FRAME_ALLOCATOR_ERROR) == 0);
        p3->entries[VA_P3_INDEX(&KERN_BASE)] = PTE(phys_addr_p2, PTE_U | PTE_W | PTE_P);

        page_table_t* p2 = map_temp(phys_addr_p2);
        memset(p2, 0, sizeof(*p2));
        phys_addr_t phys_addr_p1 = frame_allocator_get_frame();
        ASSERT((phys_addr_p1 & FRAME_ALLOCATOR_ERROR) == 0);
        p2->entries[VA_P2_INDEX(&KERN_BASE)] = PTE(phys_addr_p1, PTE_U | PTE_W | PTE_P);
        /*
         * add p1 of temp buf
         */
        p2->entries[VA_P2_INDEX(buf_page)] = PTE(phys_addr_of_kernel_static(&map_temp_p1), PTE_U | PTE_W | PTE_P);

        page_table_t* p1 = map_temp(phys_addr_p1);
        memset(p1, 0, sizeof(*p1));

        for (size_t i = 0; i < nr_kern_sections; ++i) {
            section_info_t* s = &kern_sections[i];
            if (s->present) {
                virt_addr_t start_page = (virt_addr_t)PAGE_ROUND_DOWN((size_t)s->start);
                virt_addr_t end_page = (virt_addr_t)PAGE_ROUND_DOWN((size_t)s->end - 1);
                for (virt_addr_t page = start_page; page <= end_page; page += PAGE_SIZE) {
                    phys_addr_t frame = phys_addr_of_kernel_static(page);
                    p1->entries[VA_P1_INDEX(page)] = PTE(frame, s->flags | PTE_P);
                }
            }
        }

        phys_addr_t phys_addr_old_p4 = read_cr3();

        /*
         * switch to proper kernel pgdir
         */
        phys_addr_t phys_addr_p4 = paging_kernel_translate_page(new_pgdir.p4, PAGE_4KiB);
        write_cr3(phys_addr_p4);
        kpgdir = new_pgdir;

        /*
         * unmap boot kernel pgdir (to act as stack guard)
         */
        phys_addr_t old_phys[3] = {
            phys_addr_old_p4,
            phys_addr_old_p4 + PAGE_SIZE,
            phys_addr_old_p4 + 2 * PAGE_SIZE,
        };
        for (size_t i = 0; i < 3; ++i) {
            virt_addr_t old_p = kernel_static_from_phys_addr(old_phys[i]);
            phys_addr_t frame = paging_kernel_unmap(old_p, PAGE_4KiB);
            ASSERT((frame & PAGING_ERROR) == 0);
            ASSERT(frame_allocator_free_frame(frame) == FRAME_ALLOCATOR_OK);
        }

        kern_pgdir_setup = 1;
        return kpgdir;
    } else {
        pgdir_t new_pgdir;
        new_pgdir.p4 = kpalloc(1);
        ASSERT(new_pgdir.p4 != NULL);

        memcpy(new_pgdir.p4, kpgdir.p4, sizeof(*new_pgdir.p4));
        return new_pgdir;
    }
}

static void reserve_kernel_frames(section_info_t* section_info, size_t nr_sections)
{
    for (size_t i = 0; i < nr_sections; ++i) {
        if (section_info[i].present) {
            phys_addr_t section_start = phys_addr_of_kernel_static((virt_addr_t)section_info[i].start);
            phys_addr_t section_end = phys_addr_of_kernel_static((virt_addr_t)section_info[i].end);
            phys_addr_t first_frame = PAGE_ROUND_DOWN(section_start);
            phys_addr_t last_frame = PAGE_ROUND_DOWN(section_end - 1);
            for (phys_addr_t p = first_frame; p <= last_frame; p += PAGE_SIZE)
                ASSERT(frame_allocator_reserve_frame(p) == FRAME_ALLOCATOR_OK);
        }
    }
}

void init_paging(section_info_t* section_info, size_t nr_sections)
{
    extern void* KERN_BASE;
    ASSERT(VA_P4_INDEX(&KERN_BASE) == VA_P4_INDEX(buf_page)
           && VA_P3_INDEX(&KERN_BASE) == VA_P3_INDEX(buf_page));

    reserve_kernel_frames(section_info, nr_sections);

    phys_addr_t phys_addr_p4 = read_cr3();
    page_table_t* p4 = kernel_static_from_phys_addr(phys_addr_p4);

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

    kpgdir.p4 = p4;

    flush_tlb_all();

    memcpy(&kern_sections[0], section_info, nr_sections * sizeof(kern_sections[0]));
    nr_kern_sections = nr_sections;
}

static int page_walk(pgdir_t pgdir, virt_addr_t addr, page_table_level_t level, page_table_t** ret)
{
    page_table_level_t curr_level = PAGE_TABLE_P4;
    page_table_t* curr_page_table = pgdir.p4;

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
static noignore int get_pte(pgdir_t pgdir, virt_addr_t page, page_type_t type, pte_t** ret)
{
    page_table_level_t parent_level = page_type_parent_table_level(type);
    page_table_t* parent_table;
    int err = page_walk(pgdir, page, parent_level, &parent_table);
    if (err != PAGING_OK)
        return err;

    pte_t* pte = &parent_table->entries[VA_PT_INDEX(page, parent_level)];
    if ((((*pte) & PTE_P) == 0) || (type != PAGE_4KiB && (((*pte) & PTE_HP) == 0)))
        return PAGING_ERROR_PAGE_NOT_MAPPED;

    *ret = pte;
    return PAGING_OK;
}

static int paging_map_with_table_flags_internal(pgdir_t pgdir, virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags, pte_flags_t parent_table_flags)
{
    page_table_level_t parent_level = page_type_parent_table_level(type);
    parent_table_flags &= ~PTE_HP;
    page_table_level_t curr_level = PAGE_TABLE_P4;
    page_table_t* curr_page_table = pgdir.p4;

    while (1) {
        if (curr_level == parent_level)
            break;

        int clear_new_table = 0;

        pte_t* pte = &curr_page_table->entries[VA_PT_INDEX(page, curr_level)];
        if (((*pte) & PTE_P) == 0) {
            phys_addr_t new_frame = frame_allocator_get_frame();
            if (new_frame == FRAME_ALLOCATOR_ERROR_NO_FRAME_AVAILABLE)
                return PAGING_ERROR_FRAME_ALLOCATION_FAILURE;

            *pte = PTE(new_frame, (parent_table_flags | PTE_U | PTE_W | PTE_P));
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
int paging_map_with_table_flags(pgdir_t pgdir, virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags, pte_flags_t parent_table_flags)
{
    ASSERT(!is_va_kernel_managed(page));
    return paging_map_with_table_flags_internal(pgdir, page, frame, type, flags, parent_table_flags);
}
int paging_kernel_map_with_table_flags(virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags, pte_flags_t parent_table_flags)
{
    ASSERT(is_va_kernel_managed(page));
    return paging_map_with_table_flags_internal(kpgdir, page, frame, type, flags, parent_table_flags);
}

int paging_map(pgdir_t pgdir, virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags)
{
    return paging_map_with_table_flags(pgdir, page, frame, type, flags, PTE_U | PTE_W | PTE_P);
}
int paging_kernel_map(virt_addr_t page, phys_addr_t frame, page_type_t type, pte_flags_t flags)
{
    return paging_kernel_map_with_table_flags(page, frame, type, flags, PTE_U | PTE_W | PTE_P);
}

static phys_addr_t paging_unmap_internal(pgdir_t pgdir, virt_addr_t page, page_type_t type)
{
    pte_t* pte;
    int err = get_pte(pgdir, page, type, &pte);
    if (err != PAGING_OK)
        return err;

    phys_addr_t frame = PTE_FRAME(*pte);
    *pte = 0;

    flush_tlb(page);
    return frame;
}
phys_addr_t paging_unmap(pgdir_t pgdir, virt_addr_t page, page_type_t type)
{
    ASSERT(!is_va_kernel_managed(page));
    return paging_unmap_internal(pgdir, page, type);
}
phys_addr_t paging_kernel_unmap(virt_addr_t page, page_type_t type)
{
    ASSERT(is_va_kernel_managed(page));
    return paging_unmap_internal(kpgdir, page, type);
}

static phys_addr_t paging_translate_page_internal(pgdir_t pgdir, virt_addr_t page, page_type_t type)
{
    pte_t* pte;
    int err = get_pte(pgdir, page, type, &pte);
    if (err != PAGING_OK)
        return err;

    return PTE_FRAME(*pte);
}
phys_addr_t paging_translate_page(pgdir_t pgdir, virt_addr_t page, page_type_t type)
{
    ASSERT(!is_va_kernel_managed(page));
    return paging_translate_page_internal(pgdir, page, type);
}
phys_addr_t paging_kernel_translate_page(virt_addr_t page, page_type_t type)
{
    ASSERT(is_va_kernel_managed(page));
    return paging_translate_page_internal(kpgdir, page, type);
}

static pte_t paging_show_pte_internal(pgdir_t pgdir, virt_addr_t page, page_type_t type)
{
    pte_t* pte;
    int err = get_pte(pgdir, page, type, &pte);
    if (err != PAGING_OK)
        return err;

    return *pte;
}
pte_t paging_show_pte(pgdir_t pgdir, virt_addr_t page, page_type_t type)
{
    ASSERT(!is_va_kernel_managed(page));
    return paging_show_pte_internal(pgdir, page, type);
}
pte_t paging_kernel_show_pte(virt_addr_t page, page_type_t type)
{
    ASSERT(is_va_kernel_managed(page));
    return paging_show_pte_internal(kpgdir, page, type);
}

noignore int paging_update_flags(pgdir_t pgdir, virt_addr_t page, page_type_t type, pte_flags_t flags)
{
    pte_t* pte;
    int err = get_pte(pgdir, page, type, &pte);
    if (err != PAGING_OK)
        return err;

    phys_addr_t frame = PTE_FRAME(*pte);
    *pte = PTE(frame, flags | PTE_P | (type == PAGE_4KiB ? 0 : PTE_HP));
    flush_tlb(page);

    return PAGING_OK;
}

void switch_pgdir(pgdir_t pgdir)
{
    phys_addr_t phys_addr_p4 = paging_kernel_translate_page(pgdir.p4, PAGE_4KiB);
    write_cr3(phys_addr_p4);
}
void switch_kernel_pgdir()
{
    switch_pgdir(kpgdir);
}
