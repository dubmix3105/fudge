#include <kernel/modules.h>
#include <kernel/vfs.h>
#include <modules/nodefs/nodefs.h>
#include <modules/vga/vga.h>

static struct vga_driver driver;
static struct nodefs_node console;

void init()
{

    vga_driver_init(&driver);
    modules_register_driver(&driver.base);

    struct nodefs_driver *nodefs = (struct nodefs_driver *)modules_get_driver(NODEFS_DRIVER_TYPE);

    if (!nodefs)
        return;

    console.name = "vga/console";
    console.read = 0;
    console.write = 0;

    nodefs->register_node(nodefs, &console);

}

void destroy()
{

    modules_unregister_driver(&driver.base);

}

