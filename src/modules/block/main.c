#include <fudge.h>
#include <kernel.h>
#include <modules/system/system.h>
#include "block.h"

static struct system_node root;

void block_notify(struct block_interface *interface, void *buffer, unsigned int count)
{

    system_multicast(&interface->data.links, buffer, count);

}

void block_registerinterface(struct block_interface *interface, unsigned int id)
{

    resource_register(&interface->resource);
    system_addchild(&interface->root, &interface->data);
    system_addchild(&root, &interface->root);

    interface->id = id;

}

void block_unregisterinterface(struct block_interface *interface)
{

    resource_unregister(&interface->resource);
    system_removechild(&interface->root, &interface->data);
    system_removechild(&root, &interface->root);

}

void block_initinterface(struct block_interface *interface, unsigned int (*rdata)(unsigned int offset, void *buffer, unsigned int count), unsigned int (*wdata)(unsigned int offset, void *buffer, unsigned int count))
{

    resource_init(&interface->resource, RESOURCE_BLOCKINTERFACE, interface);
    system_initresourcenode(&interface->root, SYSTEM_NODETYPE_GROUP | SYSTEM_NODETYPE_MULTI, "if", &interface->resource);
    system_initresourcenode(&interface->data, SYSTEM_NODETYPE_MAILBOX, "data", &interface->resource);

    interface->rdata = rdata;
    interface->wdata = wdata;

}

void module_init(void)
{

    system_initnode(&root, SYSTEM_NODETYPE_GROUP, "block");

}

void module_register(void)
{

    system_registernode(&root);

}

void module_unregister(void)
{

    system_unregisternode(&root);

}

