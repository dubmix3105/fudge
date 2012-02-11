#include <lib/memory.h>
#include <kernel/modules.h>
#include <kernel/vfs.h>
#include <modules/nodefs/nodefs.h>
#include <modules/rtl8139/rtl8139.h>

static struct rtl8139_driver driver;
static struct vfs_node mac;
static struct vfs_node data;

static unsigned int mac_read(struct vfs_node *self, unsigned int count, void *buffer)
{

    memory_copy(buffer, driver.mac, 6);

    return 6;

}

static unsigned int data_read(struct vfs_node *self, unsigned int count, void *buffer)
{

    return driver.read(&driver, buffer);

}

static unsigned int data_write(struct vfs_node *self, unsigned int count, void *buffer)
{

    return driver.write(&driver, count, buffer);

}

void init()
{

    rtl8139_driver_init(&driver);
    modules_register_driver(&driver.base);

    struct nodefs_driver *nodefs = (struct nodefs_driver *)modules_get_driver(NODEFS_DRIVER_TYPE);

    if (!nodefs)
        return;

    vfs_node_init(&mac, "module/rtl8139/mac", 0, 0, mac_read, 0);
    vfs_node_init(&data, "module/rtl8139/data", 0, 0, data_read, data_write);

    nodefs->register_node(nodefs, &mac);
    nodefs->register_node(nodefs, &data);

}

void destroy()
{

    modules_unregister_driver(&driver.base);

}

