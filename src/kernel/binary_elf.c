#include <fudge.h>
#include <elf/elf.h>
#include "resource.h"
#include "task.h"
#include "service.h"
#include "binary.h"

static struct binary_format format;

static unsigned int readheader(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, struct elf_header *header)
{

    return protocol->read(backend, id, 0, ELF_HEADER_SIZE, header);

}

static unsigned int readprogramheader(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, struct elf_header *header, unsigned int index, struct elf_programheader *programheader)
{

    return protocol->read(backend, id, header->phoffset + index * header->phsize, header->phsize, programheader);

}

static unsigned int readsectionheader(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, struct elf_header *header, unsigned int index, struct elf_sectionheader *sectionheader)
{

    return protocol->read(backend, id, header->shoffset + index * header->shsize, header->shsize, sectionheader);

}

static unsigned int relocate(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, struct elf_header *header, struct elf_sectionheader *relocationheader, unsigned int address)
{

    struct elf_sectionheader dataheader;
    struct elf_sectionheader symbolheader;
    unsigned int i;

    if (!readsectionheader(protocol, backend, id, header, relocationheader->link, &symbolheader))
        return 0;

    if (!readsectionheader(protocol, backend, id, header, relocationheader->info, &dataheader))
        return 0;

    for (i = 0; i < relocationheader->size / relocationheader->esize; i++)
    {

        struct elf_relocation relocation;
        struct elf_symbol symbol;
        unsigned char type;
        unsigned char index;
        unsigned long *entry;
        unsigned int addend;

        if (!protocol->read(backend, id, relocationheader->offset + i * relocationheader->esize, relocationheader->esize, &relocation))
            return 0;

        type = relocation.info & 0x0F;
        index = relocation.info >> 8;

        if (!protocol->read(backend, id, symbolheader.offset + index * symbolheader.esize, symbolheader.esize, &symbol))
            return 0;

        entry = (unsigned long *)(address + dataheader.offset + relocation.offset);
        addend = 0;

        if (symbol.shindex)
        {

            struct elf_sectionheader referenceheader;

            if (!readsectionheader(protocol, backend, id, header, symbol.shindex, &referenceheader))
                return 0;

            addend = address + referenceheader.offset + symbol.value;

        }

        switch (type)
        {

        case ELF_RELOC_TYPE_32:
            *entry += addend;

            break;

        case ELF_RELOC_TYPE_PC32:
            *entry += addend - (unsigned long)entry;

            break;

        }

    }

    return 1;

}

static unsigned long findsymbol(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, struct elf_header *header, struct elf_sectionheader *symbolheader, unsigned int count, char *symbolname)
{

    struct elf_sectionheader stringheader;
    char strings[4096];
    unsigned int i;

    if (!readsectionheader(protocol, backend, id, header, symbolheader->link, &stringheader))
        return 0;

    if (stringheader.size > 4096)
        return 0;

    if (!protocol->read(backend, id, stringheader.offset, stringheader.size, strings))
        return 0;

    for (i = 0; i < symbolheader->size / symbolheader->esize; i++)
    {

        struct elf_symbol symbol;
        char *s;

        if (!protocol->read(backend, id, symbolheader->offset + i * symbolheader->esize, symbolheader->esize, &symbol))
            return 0;

        if (!symbol.shindex)
            continue;

        s = strings + symbol.name;

        if (s[count] == '\0' && memory_match(symbolname, s, count))
        {

            struct elf_sectionheader referenceheader;

            if (!readsectionheader(protocol, backend, id, header, symbol.shindex, &referenceheader))
                return 0;

            return symbol.value + referenceheader.address + referenceheader.offset;

        }

    }

    return 0;

}

static unsigned int format_match(struct service_protocol *protocol, struct service_backend *backend, unsigned int id)
{

    struct elf_header header;

    if (!readheader(protocol, backend, id, &header))
        return 0;

    return elf_validate(&header);

}

static unsigned long format_findsymbol(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned int count, char *symbolname)
{

    struct elf_header header;
    unsigned int address;
    unsigned int i;

    if (!readheader(protocol, backend, id, &header))
        return 0;

    for (i = 0; i < header.shcount; i++)
    {

        struct elf_sectionheader referenceheader;

        if (!readsectionheader(protocol, backend, id, &header, i, &referenceheader))
            return 0;

        if (referenceheader.type != ELF_SECTION_TYPE_SYMTAB)
            continue;

        address = findsymbol(protocol, backend, id, &header, &referenceheader, count, symbolname);

        if (address)
            return address;

    }

    return 0;

}

static unsigned long format_findentry(struct service_protocol *protocol, struct service_backend *backend, unsigned int id)
{

    struct elf_header header;

    if (!readheader(protocol, backend, id, &header))
        return 0;

    return header.entry;

}

static unsigned long format_findbase(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned long address)
{

    struct elf_header header;
    unsigned int i;

    if (!readheader(protocol, backend, id, &header))
        return 0;

    for (i = 0; i < header.phcount; i++)
    {

        struct elf_programheader programheader;

        if (!readprogramheader(protocol, backend, id, &header, i, &programheader))
            return 0;

        if (programheader.vaddress <= address && programheader.vaddress + programheader.msize > address)
            return programheader.vaddress;

    }

    return 0;

}

static unsigned int format_copyprogram(struct service_protocol *protocol, struct service_backend *backend, unsigned int id)
{

    struct elf_header header;
    unsigned int i;

    if (!readheader(protocol, backend, id, &header))
        return 0;

    for (i = 0; i < header.phcount; i++)
    {

        struct elf_programheader programheader;

        if (!readprogramheader(protocol, backend, id, &header, i, &programheader))
            return 0;

        if (programheader.fsize)
        {

            if (!protocol->read(backend, id, programheader.offset, programheader.fsize, (void *)programheader.vaddress))
                return 0;

        }

        memory_clear((void *)(programheader.vaddress + programheader.fsize), programheader.msize - programheader.fsize);

    }

    return 1;

}

static unsigned int format_relocate(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned int address)
{

    struct elf_header header;
    unsigned int i;

    if (!readheader(protocol, backend, id, &header))
        return 0;

    for (i = 0; i < header.shcount; i++)
    {

        struct elf_sectionheader referenceheader;

        if (!readsectionheader(protocol, backend, id, &header, i, &referenceheader))
            return 0;

        referenceheader.address += address;

        if (!protocol->write(backend, id, header.shoffset + i * header.shsize, header.shsize, &referenceheader))
            return 0;

        if (referenceheader.type != ELF_SECTION_TYPE_REL)
            continue;

        if (!relocate(protocol, backend, id, &header, &referenceheader, address))
            return 0;

    }

    return 1;

}

void binary_setupelf(void)
{

    binary_initformat(&format, format_match, format_findsymbol, format_findentry, format_findbase, format_copyprogram, format_relocate);
    resource_register(&format.resource);

}

