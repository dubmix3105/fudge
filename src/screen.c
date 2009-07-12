#include <system.h>
#include <mem.h>
#include <screen.h>

screen_t screen;

void putc(char c)
{

	uint16_t *where;

	if (c == 0x08)
	{

		if (screen.cursorX != 0)
			screen.cursorX--;

	}

	else if (c == 0x09)
	{

		screen.cursorX = (screen.cursorX + 8) & ~(8 - 1);

	}

	else if (c == '\r')
	{

		screen.cursorX = 0;

	}

	else if (c == '\n')
	{

		screen.cursorX = 0;
		screen.cursorY++;

	}
	
	else if (c >= ' ')
	{

		where = screen.address + (screen.cursorY * 80 + screen.cursorX);
		*where = c | screen.context.attribute << 8;
		screen.cursorX++;

	}

	if (screen.cursorX >= 80)
	{

		screen.cursorX = 0;
		screen.cursorY++;

	}

	screen_scroll();
	screen_move_cursor();

}

void puts(char *s)
{

	int i;

	for (i = 0; i < strlen(s); i++)
		putc(s[i]);

}

void puts_dec(uint32_t n)
{

	if (n == 0)
	{
		putc('0');
		return;
	}

	int32_t acc = n;
	char c[32];
	int i = 0;

	while (acc > 0)
	{

		c[i] = '0' + acc % 10;
		acc /= 10;
		i++;

	}

	c[i] = 0;

	char c2[32];
	c2[i--] = 0;
	int j = 0;

	while (i >= 0)
		c2[i--] = c[j++];

	puts(c2);

}

void puts_hex(uint32_t n)
{

	int32_t tmp;

	puts("0x");

	char noZeroes = 1;

	int i;

	for (i = 28; i > 0; i -= 4)
	{

		tmp = (n >> i) &0xF;

		if (tmp == 0 && noZeroes != 0)
			continue;

		if (tmp >= 0xA)
		{

			noZeroes = 0;
			putc(tmp - 0xA + 'a');

		}

		else
		{

			noZeroes = 0;
			putc(tmp + '0');

		}

	}

	tmp = n & 0xF;

	if (tmp >= 0xA)
		putc(tmp - 0xA + 'a');
	else
		putc(tmp + '0');

}

void screen_set_text_color(uint8_t forecolor, uint8_t backcolor)
{

	screen.context.attribute = (backcolor << 4) | (forecolor & 0x0F);

}

void screen_clear()
{

	uint32_t blank;
	int32_t i;

	blank = 0x20 | (screen.context.attribute << 8);

	for (i = 0; i < 25; i++)
		memsetw(screen.address + i * 80, blank, 80);

	screen.cursorX = 0;
	screen.cursorY = 0;

	screen_move_cursor();

}

void screen_move_cursor()
{

	uint32_t temp;

	temp = screen.cursorY * 80 + screen.cursorX;

	outb(0x3D4, 14);
	outb(0x3D5, temp >> 8);
	outb(0x3D4, 15);
	outb(0x3D5, temp);

}

void screen_scroll()
{

	uint32_t blank, temp;

	blank = 0x20 | (screen.context.attribute << 8);

	if (screen.cursorY >= 25)
	{

		temp = screen.cursorY - 25 + 1;
		memcpy(screen.address, screen.address + temp * 80, (25 - temp) * 80 * 2);
		memsetw(screen.address + (25 - temp) * 80, blank, 80);
		screen.cursorY = 25 - 1;

	}

}

void screen_init()
{

	screen_context_t context;
	context.attribute = 0x0F;

	screen.address = (uint16_t *)SCREEN_ADDRESS;
	screen.cursorX = 0;
	screen.cursorY = 0;
	screen.context = context;

	screen_clear();

}

