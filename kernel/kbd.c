#include <lib/types.h>
#include <lib/io.h>
#include <lib/cbuffer.h>
#include <kernel/regs.h>
#include <kernel/irq.h>
#include <kernel/kbd.h>

char kbdMapLowerUS[128] =
{
       0,   27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=', '\b', '\t',
     'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',    0,  'a',  's',
     'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',  '`',    0, '\\',  'z',  'x',  'c',  'v',
     'b',  'n',  'm',  ',',  '.',  '/',    0,  '*',    0,  ' ',    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  '-',    0,    0,    0,  '+',    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

char kbdMapUpperUS[128] =
{
       0,   27,  '!',  ' ',  '#',  ' ',  '%',  '&',  '/',  '(',  ')',  '=',  '-',  '=', '\b', '\t',
     'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '[',  ']', '\n',    0,  'A',  'S',
     'D',  'F',  'G',  'H',  'J',  'K',  'L',  ';', '\'',  '`',    0, '\\',  'Z',  'X',  'C',  'V',
     'B',  'N',  'M',  ',',  '.',  '/',    0,  '*',    0,  ' ',    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  '-',    0,    0,    0,  '+',    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

kbd_device_t keyboard;

void kbd_handler(registers_t *r)
{

    unsigned char scancode = inb(KBD_PORT_READ);

    if (scancode & 0x80)
    {

        if (scancode == 0xAA)
            keyboard.toggleShift = 0;

    }

    else
    {

        if (scancode == 0x2A)
            keyboard.toggleShift = 1;

        else
        {

            if (keyboard.toggleShift)
                cbuffer_write(&keyboard.cbuffer, kbdMapUpperUS[scancode]);
            else
                cbuffer_write(&keyboard.cbuffer, kbdMapLowerUS[scancode]);

        }

    }

}

void kbd_init()
{

    keyboard.cbuffer = cbuffer_create(keyboard.buffer, KBD_BUFFER_SIZE);
    keyboard.toggleAlt = 0;
    keyboard.toggleCtrl = 0;
    keyboard.toggleShift = 0;

}

