#include <kernel/arch/x86/io.h>
#include <kernel/event.h>
#include <kernel/modules.h>
#include <modules/ps2/ps2.h>

static void wait(unsigned char type)
{

    unsigned int timeout = 100000;

    if (type)
    {

        while (timeout--)
        {

            if ((io_inb(0x64) & 2) == 0)
                return;

        }

    }

    else
    {

        while (timeout--)
        {

            if ((io_inb(0x64) & 1) == 1)
                return;

        }

    }

}

static void write(unsigned char value)
{

    wait(1);
    io_outb(0x64, 0xD4);
    wait(1);
    io_outb(0x60, value);

}

static unsigned char read()
{

    wait(0);

    return io_inb(0x60);

}

void mouse_driver_init(struct mouse_driver *driver)
{

    modules_driver_init(&driver->base, MOUSE_DRIVER_TYPE);

    driver->cycle = 0;
    driver->x = 0;
    driver->y = 0;

    unsigned char status;

    wait(1);
    io_outb(0x64, 0xA8);
    wait(1);
    io_outb(0x64, 0x20);
    wait(0);
    status = (io_inb(0x60) | 2);
    wait(1);
    io_outb(0x64, 0x60);
    wait(1);
    io_outb(0x60, status);
    write(0xF6);
    read();
    write(0xF4);
    read();

}

