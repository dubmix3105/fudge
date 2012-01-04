#include <lib/elf.h>
#include <lib/string.h>
#include <kernel/elf.h>
#include <kernel/symbol.h>

static struct elf_header *get_header(void *address)
{

    struct elf_header *header = (struct elf_header *)address;

    if (header->identify[0] != ELF_IDENTITY_MAGIC0)
        return 0;

    if (header->identify[1] != ELF_IDENTITY_MAGIC1)
        return 0;

    if (header->identify[2] != ELF_IDENTITY_MAGIC2)
        return 0;

    if (header->identify[3] != ELF_IDENTITY_MAGIC3)
        return 0;

    return header;

}

static struct elf_program_header *get_program_header(void *address)
{

    struct elf_header *header = get_header(address);

    return (struct elf_program_header *)(address + header->phoffset);

}

static struct elf_section_header *get_section_header(void *address, unsigned int index)
{

    struct elf_header *header = get_header(address);

    return (struct elf_section_header *)(address + header->shoffset + header->shsize * index);

}

void *elf_get_entry(void *address)
{

    struct elf_header *header = get_header(address);

    if (!header)
        return 0;

    return header->entry;

}

void *elf_get_virtual(void *address)
{

    struct elf_header *header = get_header(address);

    if (!header)
        return 0;

    struct elf_program_header *pheader = get_program_header(address);

    return pheader->vaddress;

}

void *elf_get_symbol(void *address, char *symname)
{

    struct elf_header *header = get_header(address);

    if (!header)
        return 0;

    struct elf_section_header *relHeader = get_section_header(address, 2);
    struct elf_section_header *infoHeader = get_section_header(address, relHeader->info);
    struct elf_section_header *symHeader = get_section_header(address, relHeader->link);
    struct elf_section_header *strHeader = get_section_header(address, symHeader->link);
    struct elf_symbol *symTable = (struct elf_symbol *)(address + symHeader->offset);
    char *strtbl = (char *)(address + strHeader->offset);

    unsigned int i;
    unsigned int count = symHeader->size / symHeader->esize;

    for (i = 0; i < count; i++)
    {

        struct elf_symbol *symbol = &symTable[i];

        if (!symbol->name)
            continue;

        char *name = strtbl + symbol->name;

        if (!string_compare(symname, name))
            return (void *)(address + infoHeader->offset + symbol->value);

    }

    return 0;

}

void elf_relocate(void *address)
{

    struct elf_header *header = get_header(address);

    if (!header)
        return;

    struct elf_section_header *relHeader = get_section_header(address, 2);
    struct elf_section_header *infoHeader = get_section_header(address, relHeader->info);
    struct elf_section_header *symHeader = get_section_header(address, relHeader->link);
    struct elf_section_header *strHeader = get_section_header(address, symHeader->link);
    struct elf_relocate *relTable = (struct elf_relocate *)(address + relHeader->offset);
    struct elf_symbol *symTable = (struct elf_symbol *)(address + symHeader->offset);
    char *strtbl = (char *)(address + strHeader->offset);

    unsigned int i;
    unsigned int count = relHeader->size / relHeader->esize; 

    for (i = 0; i < count; i++)
    {

        struct elf_relocate *relocate = &relTable[i];
        struct elf_symbol *symbol = &symTable[relocate->info >> 8];
        int offset = get_section_header(address, symbol->index)->offset;
        int *entry = (int *)((int)address + infoHeader->offset + relocate->offset);
        int reloc = (symbol->index) ? (int)address + offset : (int)symbol_find(strtbl + symbol->name);
        int paddress = reloc + symbol->value;

        switch (relocate->info & 0x0F)
        {

            case 1:

                *entry = paddress + *entry;

                break;

            case 2:

                *entry = paddress + *entry - (int)entry;

                break;

        }

    }

}

