struct keyboard_interfacenode
{

    struct system_node base;
    struct system_node data;
    struct keyboard_interface *interface;

};

struct keyboard_interface
{

    struct base_interface base;
    struct keyboard_interfacenode node;
    unsigned int (*rdata)(unsigned int offset, unsigned int count, void *buffer);

};

void keyboard_registerinterface(struct keyboard_interface *interface, struct base_bus *bus, unsigned int id);
void keyboard_unregisterinterface(struct keyboard_interface *interface);
void keyboard_initinterface(struct keyboard_interface *interface, struct base_driver *driver, unsigned int (*rdata)(unsigned int offset, unsigned int count, void *buffer));
