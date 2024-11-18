#include <cpu/page.h>
#include <cpu/x86.h>
#include <liballoc.h>
#include <mem/kpalloc.h>
#include <mem/pmm.h>
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
        PANIC("Bad pgdir index to switch to");
    current_dir = pgdir_list[d];
    lcr3(pgdir_phy_addr_list[d]);
}

void init_mm(void)
{
    // static page_directory_t kernel_pgdir __attribute__((__aligned__(PAGE_SIZE)));
    // extern pde_t entry_pgdir[];
    // extern mem_map_t kernel_mem_map[];

    // memset(&kernel_pgdir, 0, sizeof(kernel_pgdir));
    // uintptr_t kernel_pgdir_physical_addr = ((uintptr_t)&kernel_pgdir.tables_physical[0] - KERN_BASE);

    // // 1. set up meta page table in current pgdir and kernel pgdir
    // uintptr_t pgtb_phy = pmm_get_frame();
    // entry_pgdir[0x3ff] = LPG_ROUND_DOWN(pgtb_phy) | PDE_LP | PTE_W | PTE_P;
    // page_table_t* pgtb = (void*)(0xffc00000 + (pgtb_phy - LPG_ROUND_DOWN(pgtb_phy)));
    // memset(pgtb->pages, 0, sizeof(pgtb->pages));
    // pgtb->pages[0] = pgtb_phy | PTE_W | PTE_P;
    // entry_pgdir[0x3ff] = pgtb_phy | PTE_W | PTE_P;
    // pgtb = (void*)0xffc00000;

    // // 2. map kernel mappings
    // for (size_t i = 0; kernel_mem_map[i].present; ++i) {
    //     uint8_t perms = kernel_mem_map[i].perms | PTE_P;
    //     if (kernel_mem_map[i].mapped) {
    //         for (uintptr_t virt = kernel_mem_map[i].virt, phy = kernel_mem_map[i].phy_start; phy < kernel_mem_map[i].phy_end; virt += PAGE_SIZE, phy += PAGE_SIZE)
    //             pgtb->pages[PTX(virt)] = PA_FRAME(phy) | perms;
    //     } else {
    //         uintptr_t ul = kernel_mem_map[i].virt + kernel_mem_map[i].nr_pages * PAGE_SIZE;
    //         for (uintptr_t virt = kernel_mem_map[i].virt; virt < ul; virt += PAGE_SIZE)
    //             pgtb->pages[PTX(virt)] = PA_FRAME(pmm_get_frame()) | perms;
    //     }
    // }

    // uintptr_t entry_pgdir_frame = PTE_FRAME(pgtb->pages[PTX((uintptr_t)entry_pgdir)]);
    // pgtb->pages[PTX((uintptr_t)entry_pgdir)] = pgtb_phy | PTE_W | PTE_P;

    // // 3. add meta page table in kernel mapping
    // // mapping to same virtual address as entry_pgdir
    // size_t pdx = PDX(kernel_mem_map[0].virt);
    // kernel_pgdir.tables[pdx] = (void*)entry_pgdir;
    // kernel_pgdir.tables_physical[pdx] = pgtb_phy | PTE_W | PTE_P | PTE_U;
    // pgtb->pages[0] = 0;
    // pgtb = (void*)entry_pgdir;

    // // 4. switch to kpgdir and free entry_pgdir frame
    // lcr3(kernel_pgdir_physical_addr);
    // current_dir = &kernel_pgdir;
    // pmm_free_frame(entry_pgdir_frame);

    // // 5. initialise kernel heap
    // init_kpalloc();

    // // 6. set up directory list
    // pgdir_list_size = INIT_PGDIRS_SIZE;
    // pgdir_list = kmalloc(INIT_PGDIRS_SIZE * (sizeof(pgdir_list[0])));
    // pgdir_phy_addr_list = kmalloc(INIT_PGDIRS_SIZE * (sizeof(pgdir_phy_addr_list[0])));
    // memset(pgdir_list, 0, INIT_PGDIRS_SIZE * (sizeof(pgdir_list[0])));
    // pgdir_list[0] = current_dir = &kernel_pgdir;
    // pgdir_phy_addr_list[0] = kernel_pgdir_physical_addr;
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

// map given virt addr range to given phy addr range with given flags
void map(uintptr_t phy, void* virt, size_t len, uint8_t flags)
{
    uintptr_t v = (uintptr_t)virt;
    for (uintptr_t va = v, pa = PA_FRAME(phy); va < v + len; va += PAGE_SIZE, pa += PAGE_SIZE) {
        pte_t* pte = get_pte(current_dir, va, 1);
        if (*pte & PTE_P)
            PANIC("Tried to map already mapped page");
        *pte = pa | flags | PTE_P;
        invlpg(va);
    }

    // kernel mappings have to be updated in all page dirs
    // if (v >= KERN_BASE) {
    //     for (size_t i = 0; i < pgdir_list_size; ++i) {
    //         if (pgdir_list[i] == current_dir || pgdir_list[i] == NULL)
    //             continue;
    //         for (uintptr_t va = v, pa = PA_FRAME(phy); va < v + len; va += PAGE_SIZE, pa += PAGE_SIZE) {
    //             pte_t* pte = get_pte(current_dir, va, 1);
    //             *pte = pa | flags | PTE_P;
    //         }
    //     }
    // }
}

// remap given virt addr range to given phy addr range with given flags
// if phy == -1 then
//      old phy addr range is used
// else
//      new phy addr range is used and old frames are freed
void remap(uintptr_t phy, void* virt, size_t len, uint8_t flags)
{
    uintptr_t v = (uintptr_t)virt;

    int use_old = (phy + 1 == 0);
    if (use_old) {
        for (uintptr_t va = v; va < v + len; va += PAGE_SIZE) {
            pte_t* pte = get_pte(current_dir, va, 0);
            if (!pte || !(*pte & PTE_P))
                PANIC("Tried to remap unmapped page");
            *pte = PTE_FRAME(*pte) | flags | PTE_P;
            invlpg(va);
        }
        // kernel mappings have to be updated in all page dirs
        for (size_t i = 0; i < pgdir_list_size; ++i) {
            if (pgdir_list[i] == current_dir || pgdir_list[i] == NULL)
                continue;
            for (uintptr_t va = v; va < v + len; va += PAGE_SIZE) {
                pte_t* pte = get_pte(current_dir, va, 0);
                *pte = PTE_FRAME(*pte) | flags | PTE_P;
            }
        }
    } else {
        for (uintptr_t va = v, pa = PA_FRAME(phy); va < v + len; va += PAGE_SIZE, pa += PAGE_SIZE) {
            pte_t* pte = get_pte(current_dir, va, 0);
            if (!pte || !(*pte & PTE_P))
                PANIC("Tried to remap unmapped page");
            pmm_free_frame(PTE_FRAME(*pte));
            *pte = pa | flags | PTE_P;
            invlpg(va);
        }
        // kernel mappings have to be updated in all page dirs
        for (size_t i = 0; i < pgdir_list_size; ++i) {
            if (pgdir_list[i] == current_dir || pgdir_list[i] == NULL)
                continue;
            for (uintptr_t va = v, pa = PA_FRAME(phy); va < v + len; va += PAGE_SIZE, pa += PAGE_SIZE) {
                pte_t* pte = get_pte(pgdir_list[i], va, 1);
                *pte = pa | flags | PTE_P;
            }
        }
    }
}

// unmap given virt addr range and free frames
void unmap(void* virt, size_t len)
{
    uintptr_t v = (uintptr_t)virt;

    pte_t* pte;
    for (uintptr_t va = v; va < v + len; va += PAGE_SIZE) {
        pte = get_pte(current_dir, va, 0);
        if (!pte || !(*pte & PTE_P))
            PANIC("Tried to unmap unmapped page");
        pmm_free_frame(PTE_FRAME(*pte));
        *pte = 0;
        invlpg(va);
    }

    // kernel mappings have to be updated in all page dirs
    // if (v >= KERN_BASE) {
    //     for (size_t i = 0; i < pgdir_list_size; ++i) {
    //         if (pgdir_list[i] == current_dir || pgdir_list[i] == NULL)
    //             continue;
    //         for (uintptr_t va = v; va < v + len; va += PAGE_SIZE) {
    //             pte = get_pte(pgdir_list[i], va, 1);
    //             *pte = 0;
    //         }
    //     }
    // }
}

// FIXME reduce excess virtual memory usage due to close-by PAs getting different pages
void* map_phy(uintptr_t pa, size_t len, uint8_t flags)
{
    uintptr_t end_pa = pa + len;
    size_t nr_pages;
    // FIXME -1 is a kludge, fix wherever it occurs with unmap_pa
    if (len + 1 == 0) {
        nr_pages = 1;
        len = (PG_ROUND_UP(pa + 1) - PG_ROUND_DOWN(pa)) >> PAGE_ORDER;
    } else {
        nr_pages = (PG_ROUND_UP(end_pa - 1) - PG_ROUND_DOWN(pa)) >> PAGE_ORDER;
        if (nr_pages == 0)
            nr_pages = 1;
    }

    void* tmp = kpalloc(nr_pages);

    remap(pa, tmp, len, flags);

    return (void*)(((uintptr_t)tmp) + PA_OFF(pa));
}

size_t alloc_page_directory(void)
{
    page_directory_t* p = kpalloc(PG_ROUND_UP(sizeof(*p)) / PAGE_SIZE);
    memset(p, 0, sizeof(*p));
    // for (size_t i = PDX(KERN_BASE); i < 1024; ++i) {
    //     p->tables[i] = current_dir->tables[i];
    //     p->tables_physical[i] = current_dir->tables_physical[i];
    // }

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
    pgdir_phy_addr_list[d] = PTE_FRAME(*get_pte(p, (uintptr_t)p, 0)) | (((uintptr_t)p) & 0xfff);

    return d;
}

void free_page_directory(size_t d)
{
    if (d >= pgdir_list_size)
        PANIC("Bad pgdir index to free");

    // page_directory_t* p = pgdir_list[d];

    // for (size_t i = 0; i < PDX(KERN_BASE); ++i) {
    //     if (p->tables_physical[i] & PTE_P) {
    //         if (p->tables_physical[i] & PDE_LP)
    //             pmm_free_large_frame(p->tables_physical[i] & 0xffc00000);
    //         else {
    //             for (size_t j = 0; j < 1024; ++j) {
    //                 if (p->tables[i]->pages[j] & PTE_P)
    //                     pmm_free_frame(PTE_FRAME(p->tables[i]->pages[j]));
    //             }
    //             kpfree(p->tables[i]);
    //         }
    //         p->tables[i] = NULL;
    //         p->tables_physical[i] = 0;
    //     }
    // }
}
