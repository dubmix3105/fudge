#include <lib/string.h>
#include <kernel/log.h>
#include <kernel/vfs.h>
#include <kernel/modules.h>
#include <modules/io/io.h>
#include <modules/ata/ata.h>

static struct ata_bus ataBusPrimary;
static struct ata_bus ataBusSecondary;
static struct ata_device ataDevices[4];
static unsigned int ataDevicesCount;

static struct ata_device *ata_get_device(struct vfs_node *node)
{

    unsigned int i;

    for (i = 0; i < ataDevicesCount; i++)
    {

        if (&ataDevices[i].base.node == node)
            return &ataDevices[i];

    }

    return 0;

}

static unsigned int ata_device_node_read(struct vfs_node *node, unsigned int count, void *buffer)
{

    struct ata_device *device = ata_get_device(node);

    if (!device)
        return 0;

    string_copy(buffer, "Type: ");

    switch (device->type)
    {

        case ATA_DEVICE_TYPE_ATA:

            string_concat(buffer, "ATA\n");

            break;

        case ATA_DEVICE_TYPE_ATAPI:

            string_concat(buffer, "ATAPI\n");

            break;

        case ATA_DEVICE_TYPE_SATA:

            string_concat(buffer, "SATA\n");

            break;

        case ATA_DEVICE_TYPE_SATAPI:

            string_concat(buffer, "SATAPI\n");

            break;

        default:

            string_concat(buffer, "UNKNOWN\n");

            break;

    }

    string_concat(buffer, "Position: ");
    string_concat(buffer, "\n");

    return string_length(buffer);

}

static void ata_sleep(struct ata_device *device)
{

    io_inb(device->control + ATA_CONTROL_STATUS);
    io_inb(device->control + ATA_CONTROL_STATUS);
    io_inb(device->control + ATA_CONTROL_STATUS);
    io_inb(device->control + ATA_CONTROL_STATUS);

}

static void ata_select(struct ata_device *device)
{

    io_outb(device->data + ATA_DATA_SELECT, device->secondary ? 0xB0 : 0xA0);
    ata_sleep(device);

}

static unsigned char ata_get_command(struct ata_device *device)
{

    return io_inb(device->data + ATA_DATA_COMMAND);

}

static void ata_set_command(struct ata_device *device, unsigned char command)
{

    io_outb(device->data + ATA_DATA_COMMAND, command);
    ata_sleep(device);

}

static unsigned char ata_read_identity(struct ata_device *device)
{

    while (1)
    {

        unsigned char status = ata_get_command(device);

        if (status & ATA_STATUS_FLAG_ERROR)
            return 0;
            
        if (!(status & ATA_STATUS_FLAG_BUSY) && (status & ATA_STATUS_FLAG_DRQ))
            break;

    }

    unsigned short buffer[256];

    memory_set(buffer, 0, 512);

    unsigned int i;

    for (i = 0; i < 256; i++)
        buffer[i] = io_inw(device->data);

    log_write("[ata] Ctrl num: %x\n", buffer[0]);

    return 1;

}

static unsigned char ata_identify(struct ata_device *device)
{

    ata_select(device);
    ata_set_command(device, ATA_COMMAND_ID);

    unsigned char status = ata_get_command(device);

    if (!status)
        return 0;

    unsigned short lba = io_inb(device->data + ATA_DATA_LBA1) | (io_inb(device->data + ATA_DATA_LBA2) << 8);

    if (lba == 0x0000)
    {

        device->type = ATA_DEVICE_TYPE_ATA;
        ata_read_identity(device);

        return 1;

    }

    if (lba == 0xEB14)
    {

        device->type = ATA_DEVICE_TYPE_ATAPI;

        return 1;

    }

    if (lba == 0xC33C)
    {

        device->type = ATA_DEVICE_TYPE_SATA;

        return 1;

    }

    if (lba == 0x9669)
    {

        device->type = ATA_DEVICE_TYPE_SATAPI;

        return 1;

    }

    return 0;

}

void ata_init_busses()
{

    ataBusPrimary.base.module.type = MODULES_TYPE_BUS;
    ataBusPrimary.base.type = MODULES_BUS_TYPE_ATA;
    string_copy(ataBusPrimary.base.name, "ata:0");
    modules_register_bus(&ataBusPrimary.base);

    ataBusSecondary.base.module.type = MODULES_TYPE_BUS;
    ataBusSecondary.base.type = MODULES_BUS_TYPE_ATA;
    string_copy(ataBusSecondary.base.name, "ata:1");
    modules_register_bus(&ataBusSecondary.base);

}

void ata_init_devices()
{

    ataDevicesCount = 4;

    unsigned int i;

    for (i = 0; i < ataDevicesCount; i++)
    {

        struct ata_device *device = &ataDevices[i];

        device->base.module.type = MODULES_TYPE_DEVICE;
        device->base.type = MODULES_DEVICE_TYPE_ATA;
        device->base.node.operations.read = ata_device_node_read;
        
        switch (i)
        {

            case 0:

                string_copy(device->base.name, "ata:0:0");
                device->control = ATA_PRIMARY_MASTER_CONTROL;
                device->data = ATA_PRIMARY_MASTER_DATA;

                break;

            case 1:

                string_copy(device->base.name, "ata:0:1");
                device->control = ATA_PRIMARY_SLAVE_CONTROL;
                device->data = ATA_PRIMARY_SLAVE_DATA;

                break;

            case 2:

                string_copy(device->base.name, "ata:1:0");
                device->control = ATA_SECONDARY_MASTER_CONTROL;
                device->data = ATA_SECONDARY_MASTER_DATA;

                break;

            case 3:

                string_copy(device->base.name, "ata:1:1");
                device->control = ATA_SECONDARY_SLAVE_CONTROL;
                device->data = ATA_SECONDARY_SLAVE_DATA;

                break;

        }

        if (ata_identify(device))
            modules_register_device(&device->base);

    }

}

void ata_init()
{

    ata_init_busses();
    ata_init_devices();

}

