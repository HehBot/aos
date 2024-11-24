#include "bootinfo.h"

#include <multiboot2.h>
#include <string.h>

/*
 * ELF sections tag parsing
 */
typedef struct elf_section_header {
    uint32_t name; // Offset into the string table
    uint32_t type; // Section type
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
    uint64_t flags; // Section attributes
    uint64_t addr; // Memory address
    uint64_t offset; // Offset in file
    uint64_t size; // Section size
    uint32_t link; // Associated section
    uint32_t info; // Extra information
    uint64_t addralign; // Alignment
    uint64_t entsize; // Entry size
} const* elf_section_header_t;

void parse_elf_section_info(struct multiboot_tag_elf_sections const* e, section_info_t (*section_info)[4])
{
    section_info_t si[4] = {
        { ".text", 0, 0, 0, 0 },
        { ".rodata", PTE_NX, 0, 0, 0 },
        { ".data", PTE_NX | PTE_W, 0, 0, 0 },
        { ".bss", PTE_NX | PTE_W, 0, 0, 0 },
    };

    elf_section_header_t shstrtab = (void*)&e->sections[e->shndx * e->entsize];
    char const* strings = (void*)shstrtab->addr;
    for (size_t i = 0; i < e->num; ++i) {
        elf_section_header_t section = (void const*)&e->sections[i * e->entsize];
        if ((section->flags & SHF_ALLOC) == 0)
            continue;
        char const* name = &strings[section->name];
        for (size_t i = 0; i < 4; ++i) {
            if (!memcmp(si[i].name, name, strlen(si[i].name))) {
                si[i].start = (virt_addr_t)section->addr;
                si[i].end = (virt_addr_t)section->addr + section->size;
                si[i].present = 1;
                break;
            }
        }
    }
    if (!si[0].present)
        PANIC("No text section");
    /*
     * coalesce .data and .bss, we know they will be consecutive
     */
    if (si[2].present && si[3].present) {
        si[2].name = ".data+.bss";
        if (si[2].start < si[3].start)
            si[2].end = si[3].end;
        else
            si[2].start = si[3].start;
        si[3] = (section_info_t) {};
    }

    memcpy(section_info, &si, sizeof(si));
}

/*
 * Multiboot info parsing
 */
struct multiboot_info parse_mboot_info(void* mboot_info)
{
    struct multiboot_info info = { 0 };
    uint8_t* p = (uint8_t*)mboot_info;
    info.size_reserved = *((size_t*)p);
    p += 8;
    int done = 0;
    while (!done) {
        struct multiboot_tag* tag = (struct multiboot_tag*)p;
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_MMAP:
            info.tag_mmap = (void*)tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
            info.tag_elf_sections = (void*)tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            info.tag_framebuffer = (void*)tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_ACPI_NEW:
            info.tag_new_acpi = (void*)tag;
            goto end;
        case MULTIBOOT_TAG_TYPE_ACPI_OLD:
            info.tag_old_acpi = (void*)tag;
            goto end;
        end:
        default:
#define ROUND_UP_8(x) ((((x) + 7) >> 3) << 3)
            p += ROUND_UP_8(tag->size);
#undef ROUND_UP_8
            continue;
        case MULTIBOOT_TAG_TYPE_END:
            done = 1;
            break;
        }
    }
    return info;
}
