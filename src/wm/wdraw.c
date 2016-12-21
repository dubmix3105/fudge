#include <abi.h>
#include <fudge.h>
#include <format/pcf.h>
#include "box.h"
#include "element.h"

#define COLOR_DARK                      0x00
#define COLOR_LIGHT                     0x01
#define COLOR_BODY                      0x02
#define COLOR_PASSIVEBACK               0x03
#define COLOR_PASSIVEFRAME              0x04
#define COLOR_ACTIVEBACK                0x05
#define COLOR_ACTIVEFRAME               0x06
#define COLOR_POINTERBACK               0x07
#define COLOR_POINTERFRAME              0x08
#define COLOR_TEXTNORMAL                0x09
#define COLOR_TEXTLIGHT                 0x0A
#define MOUSE_WIDTH                     24
#define MOUSE_HEIGHT                    24
#define TEXT_LINEHEIGHT                 24

static struct ctrl_videosettings oldsettings;
static struct ctrl_videosettings settings;
static unsigned char textcolor[2];
static unsigned char drawdata[0x2000];
static unsigned int (*funcs[3])(unsigned char *data, unsigned int count, struct element *element);
static unsigned int (*tests[5])(struct element *element, void *data, unsigned int line);
static void (*renderers[5])(struct element *element, void *data, unsigned int line);
static unsigned char elementdata[0x8000];
static unsigned int elementcount;
static unsigned char fontdata[0x8000];
static unsigned char *fontbitmapdata;
static unsigned int fontpadding;
static void (*paint)(unsigned int color, unsigned int offset, unsigned int count);
static unsigned char mousedata[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x07, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x07, 0x08, 0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x07, 0x08, 0x00, 0x00, 0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x08, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x08, 0x07, 0x07, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x08, 0x07, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static unsigned char colormap8[] = {
    0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF,
    0x03, 0x02, 0x02,
    0x05, 0x04, 0x04,
    0x07, 0x06, 0x06,
    0x08, 0x10, 0x18,
    0x0C, 0x14, 0x1C,
    0x28, 0x10, 0x18,
    0x38, 0x20, 0x28,
    0x1C, 0x18, 0x18,
    0xFF, 0xFF, 0xFF
};
static unsigned int colormap32[] = {
    0xFF000000,
    0xFFFFFFFF,
    0xFF181014,
    0xFF20181C,
    0xFF30282C,
    0xFF105070,
    0xFF307090,
    0xFFB05070,
    0xFFF898B8,
    0xFF80787C,
    0xFFFFFFFF
};

static void paint8(unsigned int color, unsigned int offset, unsigned int count)
{

    unsigned char *buffer = drawdata + offset;

    while (count--)
        *buffer++ = color;

}

static void paint32(unsigned int color, unsigned int offset, unsigned int count)
{

    unsigned int *buffer = (unsigned int *)drawdata + offset;

    while (count--)
        *buffer++ = colormap32[color];

}

static unsigned int isoverlap(unsigned int line, unsigned int y, unsigned int h)
{

    return line >= y && line < y + h;

}

static unsigned int testfill(struct element *element, void *data, unsigned int line)
{

    struct element_fill *fill = data;

    return isoverlap(line, fill->size.y, fill->size.h);

}

static void renderfill(struct element *element, void *data, unsigned int line)
{

    struct element_fill *fill = data;

    paint(fill->color, fill->size.x, fill->size.w);

}

static unsigned int testmouse(struct element *element, void *data, unsigned int line)
{

    struct element_mouse *mouse = data;

    return isoverlap(line, mouse->y, MOUSE_HEIGHT);

}

static void rendermouse(struct element *element, void *data, unsigned int line)
{

    struct element_mouse *mouse = data;
    unsigned char *md = mousedata + (line - mouse->y) * MOUSE_WIDTH;
    unsigned int i;

    for (i = 0; i < MOUSE_WIDTH; i++)
    {

        if (md[i] != 0xFF)
            paint(md[i], mouse->x + i, 1);

    }

}

static unsigned int testpanel(struct element *element, void *data, unsigned int line)
{

    struct element_panel *panel = data;

    return isoverlap(line, panel->size.y, panel->size.h);

}

static void renderpanel(struct element *element, void *data, unsigned int line)
{

    struct element_panel *panel = data;
    unsigned int framecolor = panel->active ? COLOR_ACTIVEFRAME : COLOR_PASSIVEFRAME;
    unsigned int backgroundcolor = panel->active ? COLOR_ACTIVEBACK : COLOR_PASSIVEBACK;

    line = (line - panel->size.y);

    if (line > panel->size.h / 2)
        line = panel->size.h - line - 1;

    switch (line)
    {

    case 0:
        paint(COLOR_DARK, panel->size.x, panel->size.w);

        break;

    case 1:
        paint(COLOR_DARK, panel->size.x + 0, 1);
        paint(COLOR_DARK, panel->size.x + panel->size.w - 1, 1);
        paint(framecolor, panel->size.x + 1, panel->size.w - 2);

        break;

    default:
        paint(COLOR_DARK, panel->size.x + 0, 1);
        paint(COLOR_DARK, panel->size.x + panel->size.w - 1, 1);
        paint(framecolor, panel->size.x + 1, 1);
        paint(framecolor, panel->size.x + panel->size.w - 2, 1);
        paint(backgroundcolor, panel->size.x + 2, panel->size.w - 4);

        break;

    }

}

static unsigned int testtext(struct element *element, void *data, unsigned int line)
{

    struct element_text *text = data;

    return isoverlap(line, text->size.y, text->size.h);

}

static unsigned int findrowcount(struct element_text *text, unsigned int offset, unsigned int count, char *string)
{

    unsigned int i;

    for (i = offset; i < count; i++)
    {

        if (string[i] == '\n')
            return i + 1;

    }

    return count;

}

static unsigned int findrowstart(struct element_text *text, unsigned int row, unsigned int count, char *string)
{

    unsigned int i;

    if (!row)
        return 0;

    for (i = 0; i < count; i++)
    {

        if (string[i] == '\n')
        {

            if (!--row)
                return i + 1;

        }

    }

    return 0;

}

static void rendercharline(unsigned char color, struct box *size, unsigned char *bitmapdata)
{

    unsigned int p;

    for (p = 0; p < size->w; p++)
    {

        if (bitmapdata[(p >> 3)] & (0x80 >> (p % 8)))
            paint(color, size->x + p, 1);

    }

}

static void rendercharlineinverted(unsigned char color, struct box *size, unsigned char *bitmapdata)
{

    unsigned int p;

    for (p = 0; p < size->w; p++)
    {

        if (!(bitmapdata[(p >> 3)] & (0x80 >> (p % 8))))
            paint(color, size->x + p, 1);

    }

}

static void rendertextline(struct element_text *text, char *string, unsigned char color, unsigned int rowtop, unsigned int rowline, unsigned int rowstart, unsigned int rowcount)
{

    struct box size;
    unsigned int i;

    size.x = text->size.x;
    size.y = text->size.y + rowtop;

    for (i = rowstart; i < rowcount; i++)
    {

        unsigned short index = (string[i] == '\n') ? pcf_getindex(fontdata, ' ') : pcf_getindex(fontdata, string[i]);
        unsigned int offset = pcf_getbitmapoffset(fontdata, index) + rowline * fontpadding;
        struct pcf_metricsdata metricsdata;

        pcf_readmetricsdata(fontdata, index, &metricsdata);

        size.w = metricsdata.width;
        size.h = metricsdata.ascent + metricsdata.descent;

        if (size.x + size.w >= text->size.x + text->size.w)
            return;

        if (size.y + size.h >= text->size.y + text->size.h)
            return;

        if (rowline < size.h)
        {

            if (text->flow == ELEMENT_TEXTFLOW_INPUT && i == text->cursor)
                rendercharlineinverted(color, &size, fontbitmapdata + offset);
            else
                rendercharline(color, &size, fontbitmapdata + offset);

        }

        size.x += size.w;

    }

}

static void rendertext(struct element *element, void *data, unsigned int line)
{

    struct element_text *text = data;
    unsigned int stringcount = element->count - sizeof (struct element_text);
    char *string = (char *)(text + 1);
    unsigned char color = textcolor[text->type];
    unsigned int row;
    unsigned int rowline;
    unsigned int rowtop;
    unsigned int rowtotal;
    unsigned int rowstart;
    unsigned int rowcount;

    if (!stringcount)
        return;

    line = (line - text->size.y);
    row = line / TEXT_LINEHEIGHT;
    rowline = line % TEXT_LINEHEIGHT;
    rowtop = row * TEXT_LINEHEIGHT;
    rowtotal = ascii_count(string, stringcount, '\n') + 1;

    if (row >= rowtotal)
        return;

    rowstart = findrowstart(text, row, stringcount, string);
    rowcount = findrowcount(text, rowstart, stringcount, string);

    if (!rowcount)
        return;

    rendertextline(text, string, color, rowtop, rowline, rowstart, rowcount);

}

static unsigned int testwindow(struct element *element, void *data, unsigned int line)
{

    struct element_window *window = data;

    return isoverlap(line, window->size.y, window->size.h);

}

static void renderwindow(struct element *element, void *data, unsigned int line)
{

    struct element_window *window = data;
    unsigned int framecolor = window->active ? COLOR_ACTIVEFRAME : COLOR_PASSIVEFRAME;

    line = (line - window->size.y);

    if (line > window->size.h / 2)
        line = window->size.h - line - 1;

    switch (line)
    {

    case 0:
        paint(COLOR_DARK, window->size.x, window->size.w);

        break;

    case 1:
        paint(COLOR_DARK, window->size.x + 0, 1);
        paint(COLOR_DARK, window->size.x + window->size.w - 1, 1);
        paint(framecolor, window->size.x + 1, window->size.w - 2);

        break;

    case 2:
        paint(COLOR_DARK, window->size.x + 0, 1);
        paint(COLOR_DARK, window->size.x + window->size.w - 1, 1);
        paint(framecolor, window->size.x + 1, 1);
        paint(framecolor, window->size.x + window->size.w - 2, 1);
        paint(COLOR_DARK, window->size.x + 2, window->size.w - 4);

        break;

    default:
        paint(COLOR_DARK, window->size.x + 0, 1);
        paint(COLOR_DARK, window->size.x + window->size.w - 1, 1);
        paint(framecolor, window->size.x + 1, 1);
        paint(framecolor, window->size.x + window->size.w - 2, 1);
        paint(COLOR_DARK, window->size.x + 2, 1);
        paint(COLOR_DARK, window->size.x + window->size.w - 3, 1);

        break;

    }

}

static struct element *nextelement(unsigned char *data, unsigned int count, struct element *element)
{

    element = (element) ? (struct element *)((unsigned char *)(element + 1) + element->count) : (struct element *)data;

    if ((unsigned int)element >= (unsigned int)data + count)
        return 0;

    return element;

}

static void destroyelements(unsigned char *data, unsigned int count, struct element *element)
{

    struct element *current = 0;

    while ((current = nextelement(data, count, current)))
    {

        if (current->source == element->source && current->id == element->id)
        {

            current->z = 0;
            current->damaged = 1;

        }

    }

}

static unsigned int insertelement(unsigned char *data, unsigned int count, struct element *element)
{

    destroyelements(elementdata, elementcount, element);

    element->damaged = 1;

    return count + memory_write(data, 0x8000, element, sizeof (struct element) + element->count, count);

}

static unsigned int updateelement(unsigned char *data, unsigned int count, struct element *element)
{

    return count;

}

static unsigned int removeelement(unsigned char *data, unsigned int count, struct element *element)
{

    destroyelements(elementdata, elementcount, element);

    return count;

}

static unsigned int cleanelements(unsigned char *data, unsigned int count)
{

    struct element *current = 0;

    while ((current = nextelement(data, count, current)))
    {

        current->damaged = 0;

        if (!current->z)
        {

            unsigned int length = sizeof (struct element) + current->count;

            memory_copy(current, (unsigned char *)current + length, count - ((unsigned char *)current - data) - length);

            count -= length;

        }

    }

    return count;

}

static unsigned int testline(unsigned char *data, unsigned int count, unsigned int line)
{

    struct element *element = 0;

    while ((element = nextelement(data, count, element)))
    {

        if (element->damaged && tests[element->type](element, element + 1, line))
            return 1;

    }

    return 0;

}

static void render(unsigned char *data, unsigned int count)
{

    unsigned int line;

    file_walkfrom(CALL_L0, CALL_PO, "data");
    file_open(CALL_L0);

    for (line = 0; line < settings.h; line++)
    {

        unsigned int z;

        if (!testline(data, count, line))
            continue;

        for (z = 1; z < 4; z++)
        {

            struct element *element = 0;

            while ((element = nextelement(data, count, element)))
            {

                if (element->z != z)
                    continue;

                if (tests[element->type](element, element + 1, line))
                    renderers[element->type](element, element + 1, line);

            }

        }

        file_seekwriteall(CALL_L0, drawdata, settings.w, settings.w * line);

    }

    file_close(CALL_L0);

}

void main(void)
{

    unsigned char buffer[FUDGE_BSIZE];
    unsigned int count;

    ctrl_setvideosettings(&settings, 1920, 1080, 32);

    if (!file_walk(CALL_L0, "/share/ter-118n.pcf"))
        return;

    file_open(CALL_L0);
    file_read(CALL_L0, fontdata, 0x8000);
    file_close(CALL_L0);

    fontbitmapdata = pcf_getbitmapdata(fontdata);
    fontpadding = pcf_getpadding(fontdata);

    if (!file_walkfrom(CALL_L0, CALL_PO, "ctrl"))
        return;

    file_open(CALL_L0);
    file_seekreadall(CALL_L0, &oldsettings, sizeof (struct ctrl_videosettings), 0);
    file_seekwriteall(CALL_L0, &settings, sizeof (struct ctrl_videosettings), 0);
    file_seekreadall(CALL_L0, &settings, sizeof (struct ctrl_videosettings), 0);
    file_close(CALL_L0);

    switch (settings.bpp)
    {

    case 8:
        paint = paint8;

        if (!file_walkfrom(CALL_L0, CALL_PO, "colormap"))
            return;

        file_open(CALL_L0);
        file_seekwriteall(CALL_L0, colormap8, 3 * 11, 0);
        file_close(CALL_L0);

        break;

    case 32:
        paint = paint32;

        break;

    }

    funcs[ELEMENT_FUNC_INSERT] = insertelement;
    funcs[ELEMENT_FUNC_UPDATE] = updateelement;
    funcs[ELEMENT_FUNC_REMOVE] = removeelement;
    textcolor[ELEMENT_TEXTTYPE_NORMAL] = COLOR_TEXTNORMAL;
    textcolor[ELEMENT_TEXTTYPE_HIGHLIGHT] = COLOR_TEXTLIGHT;
    tests[ELEMENT_TYPE_FILL] = testfill;
    tests[ELEMENT_TYPE_MOUSE] = testmouse;
    tests[ELEMENT_TYPE_PANEL] = testpanel;
    tests[ELEMENT_TYPE_TEXT] = testtext;
    tests[ELEMENT_TYPE_WINDOW] = testwindow;
    renderers[ELEMENT_TYPE_FILL] = renderfill;
    renderers[ELEMENT_TYPE_MOUSE] = rendermouse;
    renderers[ELEMENT_TYPE_PANEL] = renderpanel;
    renderers[ELEMENT_TYPE_TEXT] = rendertext;
    renderers[ELEMENT_TYPE_WINDOW] = renderwindow;

    file_open(CALL_PI);

    while ((count = file_read(CALL_PI, buffer, FUDGE_BSIZE)))
    {

        struct element *element = 0;

        while ((element = nextelement(buffer, count, element)))
            elementcount = funcs[element->func](elementdata, elementcount, element);

        render(elementdata, elementcount);

        elementcount = cleanelements(elementdata, elementcount);

    }

    file_close(CALL_PI);

    if (!file_walkfrom(CALL_L0, CALL_PO, "ctrl"))
        return;

    file_open(CALL_L0);
    file_seekwriteall(CALL_L0, &oldsettings, sizeof (struct ctrl_videosettings), 0);
    file_close(CALL_L0);

}

