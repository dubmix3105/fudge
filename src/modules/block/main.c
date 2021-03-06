#include <fudge.h>
#include <kernel.h>
#include <modules/system/system.h>
#include "block.h"

static struct system_node root;

void block_notify(struct block_interface *interface, void *buffer, unsigned int count)
{

    struct resource *current = 0;

    while ((current = resource_foreachtype(current, RESOURCE_BLOCKPROTOCOL)))
    {

        struct block_protocol *protocol = current->data;

        protocol->notify(buffer, count);

    }

    kernel_notify(&interface->data.states, EVENT_DATA, buffer, count);

}

void block_registerinterface(struct block_interface *interface)
{

    resource_register(&interface->resource);
    system_addchild(&interface->root, &interface->data);
    system_addchild(&root, &interface->root);

}

void block_registerprotocol(struct block_protocol *protocol)
{

    resource_register(&protocol->resource);
    system_addchild(&protocol->root, &protocol->data);
    system_addchild(&root, &protocol->root);

}

void block_unregisterinterface(struct block_interface *interface)
{

    resource_unregister(&interface->resource);
    system_removechild(&interface->root, &interface->data);
    system_removechild(&root, &interface->root);

}

void block_unregisterprotocol(struct block_protocol *protocol)
{

    resource_unregister(&protocol->resource);
    system_removechild(&protocol->root, &protocol->data);
    system_removechild(&root, &protocol->root);

}

void block_initinterface(struct block_interface *interface, unsigned int id)
{

    resource_init(&interface->resource, RESOURCE_BLOCKINTERFACE, interface);
    system_initnode(&interface->root, SYSTEM_NODETYPE_MULTIGROUP, "if");
    system_initnode(&interface->data, SYSTEM_NODETYPE_NORMAL, "data");

    interface->id = id;

}

void block_initprotocol(struct block_protocol *protocol, char *name, void (*notify)(void *buffer, unsigned int count))
{

    resource_init(&protocol->resource, RESOURCE_BLOCKPROTOCOL, protocol);
    system_initnode(&protocol->root, SYSTEM_NODETYPE_GROUP, name);
    system_initnode(&protocol->data, SYSTEM_NODETYPE_NORMAL, "data");

    protocol->notify = notify;

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

