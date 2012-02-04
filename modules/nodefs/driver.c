#include <lib/memory.h>
#include <lib/string.h>
#include <kernel/log.h>
#include <kernel/modules.h>
#include <kernel/vfs.h>
#include <modules/nodefs/nodefs.h>

static struct vfs_node *filesystem_get_node(struct vfs_filesystem *self, unsigned int index)
{

    struct nodefs_filesystem *filesystem = (struct nodefs_filesystem *)self;

    if (index >= filesystem->count)
        return 0;

    return filesystem->nodes[index];

}

static struct vfs_node *filesystem_find_node(struct vfs_filesystem *self, char *name)
{

    struct nodefs_filesystem *filesystem = (struct nodefs_filesystem *)self;

    unsigned int i;

    for (i = 0; i < filesystem->count; i++)
    {

        if (string_find(filesystem->nodes[i]->name, name))
            return filesystem->nodes[i];

    }

    return 0;

}

static unsigned int filesystem_walk(struct vfs_filesystem *self, unsigned int index)
{

    struct nodefs_filesystem *filesystem = (struct nodefs_filesystem *)self;

    if (index >= filesystem->count)
        return 0;

    return index + 1;

}

static void register_node(struct nodefs_driver *self, struct vfs_node *node)
{

    unsigned int i;

    for (i = 0; i < 128; i++)
    {

        if (!self->filesystem.nodes[i])
        {

            self->filesystem.nodes[i] = node;
            self->filesystem.count++;

            break;

        }

    }

}

static void unregister_node(struct nodefs_driver *self, struct vfs_node *node)
{

    unsigned int i;

    for (i = 0; i < 128; i++)
    {

        if (self->filesystem.nodes[i] == node)
        {

            self->filesystem.nodes[i] = 0;
            self->filesystem.count--;

            break;

        }

    }

}

void nodefs_filesystem_init(struct nodefs_filesystem *filesystem)
{

    vfs_filesystem_init(&filesystem->base, filesystem_get_node, filesystem_find_node, filesystem_walk); 
    filesystem->base.firstIndex = 0;
    filesystem->count = 0;

    memory_clear(filesystem->nodes, sizeof (struct vfs_node *) * 128);

    vfs_register_filesystem(&filesystem->base);

}

void nodefs_driver_init(struct nodefs_driver *driver)
{

    modules_driver_init(&driver->base, NODEFS_DRIVER_TYPE);
    nodefs_filesystem_init(&driver->filesystem);

    driver->register_node = register_node;
    driver->unregister_node = unregister_node;

}

