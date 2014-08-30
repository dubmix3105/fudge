#include <module.h>
#include <kernel/resource.h>
#include <base/base.h>
#include <arch/x86/pci/pci.h>
#include "uhci.h"

enum uhci_register
{

    UHCI_REGISTER_COMMAND               = 0x00,
    UHCI_REGISTER_STATUS                = 0x02,
    UHCI_REGISTER_INTERRUPT             = 0x04,
    UHCI_REGISTER_FRAMEINDEX            = 0x06,
    UHCI_REGISTER_FRAMEBASE             = 0x08,
    UHCI_REGISTER_FRAMEMOD              = 0x0C,
    UHCI_REGISTER_PORT1                 = 0x10,
    UHCI_REGISTER_PORT2                 = 0x12

};

static struct base_driver driver;

static unsigned int check(struct base_bus *bus, unsigned int id)
{

    if (bus->type != PCI_BUS_TYPE)
        return 0;

    return pci_bus_inb(bus, id, PCI_CONFIG_CLASS) == PCI_CLASS_SERIAL && pci_bus_inb(bus, id, PCI_CONFIG_SUBCLASS) == PCI_CLASS_SERIAL_USB && pci_bus_inb(bus, id, PCI_CONFIG_INTERFACE) == 0x00;

}

static void attach(struct base_bus *bus, unsigned int id)
{

    /*
    unsigned int bar0 = pci_bus_ind(bus, id, PCI_CONFIG_BAR4);
    */

}

static void detach(struct base_bus *bus, unsigned int id)
{

}

void init()
{

    base_init_driver(&driver, "uhci", check, attach, detach);
    base_register_driver(&driver);

}

void destroy()
{

    base_unregister_driver(&driver);

}
