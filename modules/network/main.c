#include <fudge.h>
#include <kernel.h>
#include <net/ethernet.h>
#include <system/system.h>
#include <base/base.h>
#include <event/event.h>
#include "network.h"

static struct system_node root;
static struct list protocols;

void network_notify(struct network_interface *interface, unsigned int count, void *buffer)
{

    struct ethernet_header *header = buffer;
    unsigned short type = (header->type[0] << 8) | header->type[1];
    struct list_item *current;

    for (current = protocols.head; current; current = current->next)
    {

        struct network_protocol *protocol = current->data;

        if (protocol->type == type)
            protocol->notify(count, buffer);

    }

    scheduler_mailboxes_send(&interface->mailboxes, count, buffer);
    event_notify(EVENT_TYPE_NETWORK, count, buffer);

}

static unsigned int interfacenode_ctrlread(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct network_interfacenode *node = (struct network_interfacenode *)self->parent;

    return memory_read(buffer, count, &node->interface->settings, sizeof (struct ctrl_networksettings), offset);

}

static unsigned int interfacenode_ctrlwrite(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct network_interfacenode *node = (struct network_interfacenode *)self->parent;

    return memory_write(&node->interface->settings, sizeof (struct ctrl_networksettings), buffer, count, offset);

}

static unsigned int interfacenode_dataopen(struct system_node *self)
{

    struct network_interfacenode *node = (struct network_interfacenode *)self->parent;

    scheduler_mailboxes_addactive(&node->interface->mailboxes);

    return system_open(self);

}

static unsigned int interfacenode_dataclose(struct system_node *self)
{

    struct network_interfacenode *node = (struct network_interfacenode *)self->parent;

    scheduler_mailboxes_removeactive(&node->interface->mailboxes);

    return system_close(self);

}

static unsigned int interfacenode_dataread(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct network_interfacenode *node = (struct network_interfacenode *)self->parent;

    return scheduler_mailboxes_readactive(&node->interface->mailboxes, count, buffer);

}

static unsigned int interfacenode_datawrite(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct network_interfacenode *node = (struct network_interfacenode *)self->parent;

    return node->interface->wdata(count, buffer);

}

static unsigned int protocolnode_dataopen(struct system_node *self)
{

    struct network_protocolnode *node = (struct network_protocolnode *)self->parent;

    scheduler_mailboxes_addactive(&node->protocol->mailboxes);

    return system_open(self);

}

static unsigned int protocolnode_dataclose(struct system_node *self)
{

    struct network_protocolnode *node = (struct network_protocolnode *)self->parent;

    scheduler_mailboxes_removeactive(&node->protocol->mailboxes);

    return system_close(self);

}

static unsigned int protocolnode_dataread(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct network_protocolnode *node = (struct network_protocolnode *)self->parent;

    return scheduler_mailboxes_readactive(&node->protocol->mailboxes, count, buffer);

}

static unsigned int protocolnode_datawrite(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    return 0;

}

void network_registerinterface(struct network_interface *interface, struct base_bus *bus, unsigned int id)
{

    base_registerinterface(&interface->base, bus, id);
    system_addchild(&root, &interface->node.base);
    system_addchild(&interface->node.base, &interface->node.ctrl);
    system_addchild(&interface->node.base, &interface->node.data);

}

void network_registerprotocol(struct network_protocol *protocol)
{

    resource_register(&protocol->resource);
    list_add(&protocols, &protocol->item);
    system_addchild(&root, &protocol->node.base);
    system_addchild(&protocol->node.base, &protocol->node.data);

}

void network_unregisterinterface(struct network_interface *interface)
{

    base_unregisterinterface(&interface->base);
    system_removechild(&interface->node.base, &interface->node.ctrl);
    system_removechild(&interface->node.base, &interface->node.data);
    system_removechild(&root, &interface->node.base);

}

void network_unregisterprotocol(struct network_protocol *protocol)
{

    resource_unregister(&protocol->resource);
    list_remove(&protocols, &protocol->item);
    system_removechild(&protocol->node.base, &protocol->node.data);
    system_removechild(&root, &protocol->node.base);

}

void network_initinterface(struct network_interface *interface, struct base_driver *driver, unsigned int (*rdata)(unsigned int count, void *buffer), unsigned int (*wdata)(unsigned int count, void *buffer))
{

    base_initinterface(&interface->base, driver);
    system_initnode(&interface->node.base, SYSTEM_NODETYPE_GROUP | SYSTEM_NODETYPE_MULTI, driver->name);
    system_initnode(&interface->node.ctrl, SYSTEM_NODETYPE_NORMAL, "ctrl");
    system_initnode(&interface->node.data, SYSTEM_NODETYPE_NORMAL, "data");
    list_init(&interface->mailboxes);
    ctrl_init_networksettings(&interface->settings);

    interface->rdata = rdata;
    interface->wdata = wdata;
    interface->node.interface = interface;
    interface->node.ctrl.read = interfacenode_ctrlread;
    interface->node.ctrl.write = interfacenode_ctrlwrite;
    interface->node.data.open = interfacenode_dataopen;
    interface->node.data.close = interfacenode_dataclose;
    interface->node.data.read = interfacenode_dataread;
    interface->node.data.write = interfacenode_datawrite;

}

void network_initprotocol(struct network_protocol *protocol, char *name, unsigned short type, void (*notify)(unsigned int count, void *buffer))
{

    resource_init(&protocol->resource, RESOURCE_TYPE_PROTONET, protocol);
    system_initnode(&protocol->node.base, SYSTEM_NODETYPE_GROUP | SYSTEM_NODETYPE_MULTI, name);
    system_initnode(&protocol->node.data, SYSTEM_NODETYPE_NORMAL, "data");
    list_init(&protocol->mailboxes);
    list_inititem(&protocol->item, protocol);

    protocol->type = type;
    protocol->notify = notify;
    protocol->node.protocol = protocol;
    protocol->node.data.open = protocolnode_dataopen;
    protocol->node.data.close = protocolnode_dataclose;
    protocol->node.data.read = protocolnode_dataread;
    protocol->node.data.write = protocolnode_datawrite;

}

void init()
{

    list_init(&protocols);
    system_initnode(&root, SYSTEM_NODETYPE_GROUP, "network");
    system_registernode(&root);

}

void destroy()
{

    system_unregisternode(&root);

}

