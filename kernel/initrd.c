#include <lib/types.h>
#include <lib/memory.h>
#include <lib/string.h>
#include <lib/vfs.h>
#include <kernel/initrd.h>

initrd_header_t *initrdHeader;
initrd_file_header_t *initrdFileHeaders;
vfs_node_t initrdRoot;
vfs_node_t initrdNodes[32];

static unsigned int initrd_read(vfs_node_t *node, unsigned int offset, unsigned int size, unsigned char *buffer)
{

    initrd_file_header_t header = initrdFileHeaders[node->inode];

    if (offset > header.length)
        return 0;

    if (offset + size > header.length)
        size = header.length - offset;

    memory_copy(buffer, (unsigned char*)(header.offset + offset), size);

    return size;

}

static vfs_node_t *initrd_walk(vfs_node_t *node, unsigned int index)
{

    if (index < initrdHeader->nfiles)
        return &initrdNodes[index];
    else
        return 0;

}

static vfs_node_t *initrd_find(vfs_node_t *node, char *name)
{

    unsigned int i;

    for (i = 0; i < initrdHeader->nfiles; i++)
    {

        if (!string_compare(name, initrdNodes[i].name))
            return &initrdNodes[i];

    }

    return 0;

}

vfs_node_t *initrd_init(unsigned int location)
{

    initrdHeader = (initrd_header_t *)location;
    initrdFileHeaders = (initrd_file_header_t *)(location + sizeof (initrd_header_t));

    string_copy(initrdRoot.name, "initrd");
    initrdRoot.inode = 0;
    initrdRoot.flags = VFS_DIRECTORY;
    initrdRoot.length = 0;
    initrdRoot.open = 0;
    initrdRoot.close = 0;
    initrdRoot.read = 0;
    initrdRoot.write = 0;
    initrdRoot.walk = initrd_walk;
    initrdRoot.find = initrd_find;

    unsigned int i;

    for (i = 0; i < initrdHeader->nfiles; i++)
    {

        initrdFileHeaders[i].offset += location;
        string_copy(initrdNodes[i].name, initrdFileHeaders[i].name);
        initrdNodes[i].inode = i;
        initrdNodes[i].flags = VFS_FILE;
        initrdNodes[i].length = initrdFileHeaders[i].length;
        initrdNodes[i].open = 0;
        initrdNodes[i].close = 0;
        initrdNodes[i].read = initrd_read;
        initrdNodes[i].write = 0;
        initrdNodes[i].walk = 0;
        initrdNodes[i].find = 0;

    }

    return &initrdRoot;

}

