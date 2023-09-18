#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <x86.h>

void switch_page_directory(page_directory_t* new);
pte_t* get_pte(page_directory_t* dir, uintptr_t virt_addr, int make);

#endif // MM_H
