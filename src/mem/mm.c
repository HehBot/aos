#include "kpalloc.h"
#include "pmm.h"

#include <cpu/x86.h>
#include <liballoc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef uint32_t pde_t;
typedef uint32_t pte_t;

typedef struct page_table {
    pte_t pages[1024];
} page_table_t;

typedef struct page_directory {
    pde_t tables_physical[1024];
    page_table_t* tables[1024];
} __attribute__((__aligned__(PAGE_SIZE))) page_directory_t;

static page_directory_t** pgdir_list;
static uintptr_t* pgdir_phy_addr_list;
#define INIT_PGDIRS_SIZE 5
static size_t pgdir_list_size;

static page_directory_t* current_dir;

void switch_page_directory(size_t d)
{
    if (d >= pgdir_list_size)
        PANIC();
    current_dir = pgdir_list[d];
    lcr3(pgdir_phy_addr_list[d]);
}

void init_mm(void)
{
    static page_directory_t kernel_pgdir __attribute__((__aligned__(PAGE_SIZE)));
    extern pde_t entry_pgdir[];
    extern mem_map_t kernel_mem_map[];

    memset(&kernel_pgdir, 0, sizeof(kernel_pgdir));
    uintptr_t kernel_pgdir_physical_addr = ((uintptr_t)&kernel_pgdir.tables_physical[0] - KERN_BASE);

    // 1. set up meta page table in current pgdir and kernel pgdir
    uintptr_t pgtb_phy = pmm_get_frame();
    entry_pgdir[0x3ff] = LPG_ROUND_DOWN(pgtb_phy) | PDE_LP | PTE_W | PTE_P;
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

    uintptr_t entry_pgdir_frame = (pgtb->pages[PTX((uintptr_t)entry_pgdir)] & 0xfffff000);
    pgtb->pages[PTX((uintptr_t)entry_pgdir)] = pgtb_phy | PTE_W | PTE_P;

    // 3. add meta page table in kernel mapping
    // mapping to same virtual address as entry_pgdir
    size_t pdx = PDX(kernel_mem_map[0].virt);
    kernel_pgdir.tables[pdx] = (void*)entry_pgdir;
    kernel_pgdir.tables_physical[pdx] = pgtb_phy | PTE_W | PTE_P | PTE_U;
    pgtb->pages[0] = 0;
    pgtb = (void*)entry_pgdir;

    // 4. switch to kpgdir and free entry_pgdir frame
    lcr3(kernel_pgdir_physical_addr);
    current_dir = &kernel_pgdir;
    pmm_free_frame(entry_pgdir_frame);

    // 5. initialise kernel heap
    init_kpalloc();

    // 6. set up directory list
    pgdir_list_size = INIT_PGDIRS_SIZE;
    pgdir_list = kmalloc(INIT_PGDIRS_SIZE * (sizeof(pgdir_list[0])));
    pgdir_phy_addr_list = kmalloc(INIT_PGDIRS_SIZE * (sizeof(pgdir_phy_addr_list[0])));
    memset(pgdir_list, 0, INIT_PGDIRS_SIZE * (sizeof(pgdir_list[0])));
    pgdir_list[0] = current_dir = &kernel_pgdir;
    pgdir_phy_addr_list[0] = kernel_pgdir_physical_addr;
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
        // all page tables are marked user
        // privilege-level restrictions are only imposed page-wise
        d->tables_physical[pde_no] = phy | PTE_W | PTE_P | PTE_U;
        memset(pgtb, 0, PAGE_SIZE);
        return &pgtb->pages[pte_no];
    } else
        return NULL;
}

// map page at given virt addr to given phy addr with given flags
int map_page(uintptr_t pa, uintptr_t va, uint8_t flags)
{
    pte_t* pte = get_pte(current_dir, va, 1);
    if (!pte || *pte & PTE_P)
        return 0;
    *pte = (pa & 0xfffff000) | flags | PTE_P;
    invlpg(va);

    // kernel mappings have to be updated in all page dirs
    if (va >= KERN_BASE) {
        for (size_t i = 0; i < pgdir_list_size; ++i) {
            if (pgdir_list[i] == current_dir || pgdir_list[i] == NULL)
                continue;
            pte = get_pte(pgdir_list[i], va, 1);
            *pte = (pa & 0xfffff000) | flags | PTE_P;
        }
    }

    return 1;
}

// remap page at given virt addr to given phy addr with given flags
// if pa == -1 then old pa is used, else new pa is used and old frame is freed
int remap_page(uintptr_t pa, uintptr_t va, uint8_t flags)
{
    pte_t* pte = get_pte(current_dir, va, 0);
    if (!pte || !(*pte & PTE_P))
        return 0;
    if (pa + 1 == 0)
        pa = (*pte);
    else
        pmm_free_frame((*pte) & 0xfffff000);
    *pte = (pa & 0xfffff000) | flags | PTE_P;
    invlpg(va);

    // kernel mappings have to be updated in all page dirs
    if (va >= KERN_BASE) {
        for (size_t i = 0; i < pgdir_list_size; ++i) {
            if (pgdir_list[i] == current_dir || pgdir_list[i] == NULL)
                continue;
            pte = get_pte(pgdir_list[i], va, 1);
            *pte = (pa & 0xfffff000) | flags | PTE_P;
        }
    }

    return 1;
}

// unmap page at given virt addr and free corresponding frame
int unmap_page(uintptr_t va)
{
    pte_t* pte = get_pte(current_dir, va, 0);
    if (!pte || !(*pte & PTE_P))
        return 0;
    uintptr_t phys = (*pte) & 0xfffff000;
    pmm_free_frame(phys);
    *pte = 0;
    invlpg(va);

    // kernel mappings have to be updated in all page dirs
    if (va >= KERN_BASE) {
        for (size_t i = 0; i < pgdir_list_size; ++i) {
            if (pgdir_list[i] == current_dir || pgdir_list[i] == NULL)
                continue;
            pte = get_pte(pgdir_list[i], va, 1);
            *pte = 0;
        }
    }

    return 1;
}

size_t alloc_page_directory(void)
{
    page_directory_t* p = kpalloc(PG_ROUND_UP(sizeof(*p)) / PAGE_SIZE);
    memset(p, 0, sizeof(*p));
    for (size_t i = PDX(KERN_BASE); i < 1024; ++i) {
        p->tables[i] = current_dir->tables[i];
        p->tables_physical[i] = current_dir->tables_physical[i];
    }

    size_t d = 0;
    for (; d < pgdir_list_size; ++d)
        if (pgdir_list[d] == NULL)
            break;

    if (d == pgdir_list_size) {
        pgdir_list = krealloc(pgdir_list, 2 * pgdir_list_size * (sizeof(pgdir_list[0])));
        memset(&pgdir_list[pgdir_list_size], 0, pgdir_list_size * (sizeof(pgdir_list[0])));
        pgdir_phy_addr_list = krealloc(pgdir_phy_addr_list, 2 * pgdir_list_size * (sizeof(pgdir_phy_addr_list[0])));
        pgdir_list_size *= 2;
    }

    pgdir_list[d] = p;
    pgdir_phy_addr_list[d] = ((*get_pte(p, (uintptr_t)p, 0)) & 0xfffff000) | (((uintptr_t)p) & 0xfff);

    return d;
}

void free_page_directory(size_t d)
{
    if (d >= pgdir_list_size)
        PANIC();

    page_directory_t* p = pgdir_list[d];

    for (size_t i = 0; i < PDX(KERN_BASE); ++i) {
        if (p->tables_physical[i] & PTE_P) {
            if (p->tables_physical[i] & PDE_LP)
                pmm_free_large_frame(p->tables_physical[i] & 0xffc00000);
            else {
                for (size_t j = 0; j < 1024; ++j) {
                    if (p->tables[i]->pages[j] & PTE_P)
                        pmm_free_frame(p->tables[i]->pages[j] & 0xfffff000);
                }
                kpfree(p->tables[i]);
            }
            p->tables[i] = NULL;
            p->tables_physical[i] = 0;
        }
    }
}
