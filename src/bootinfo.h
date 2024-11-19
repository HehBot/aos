#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <cpu/x86.h>
#include <multiboot2.h>

struct multiboot_info {
    struct multiboot_tag_mmap const* tag_mmap;
    struct multiboot_tag_elf_sections const* tag_elf_sections;
    struct multiboot_tag_framebuffer const* tag_framebuffer;
    struct multiboot_tag_old_acpi const* tag_old_acpi;
    struct multiboot_tag_new_acpi const* tag_new_acpi;
    size_t size_reserved;
};

struct multiboot_info parse_mboot_info(void* mboot_info);

typedef struct {
    char const* name;
    pte_flags_t flags;
    phys_addr_t start, end;
    int present;
} section_info_t;

struct multiboot_tag_elf_sections;
void parse_elf_section_info(struct multiboot_tag_elf_sections const* e, section_info_t (*section_info)[4]);

#endif // BOOTINFO_H
