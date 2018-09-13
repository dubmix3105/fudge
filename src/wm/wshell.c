#include <abi.h>
#include <fudge.h>
#include <event/base.h>
#include <event/wm.h>
#include <math/box.h>
#include <keymap/keymap.h>
#include <widget/widget.h>

static struct widget_textbox content;
static unsigned int quit;
static unsigned int keymod = KEYMOD_NONE;
static char outputdata[FUDGE_BSIZE];
static struct ring output;
static char inputdata1[FUDGE_BSIZE];
static struct ring input1;
static char inputdata2[FUDGE_BSIZE];
static struct ring input2;
static char textdata[FUDGE_BSIZE];
static struct ring text;
static unsigned int totalrows;
static unsigned int visiblerows;

static void updatecontent(struct event_header *header)
{

    content.length = ring_count(&text) + ring_count(&input1) + ring_count(&input2) + 1;
    content.cursor = ring_count(&text) + ring_count(&input1);

    widget_update(&output, &content, WIDGET_Z_MIDDLE, header->destination, WIDGET_TYPE_TEXTBOX, sizeof (struct widget_textbox) + content.length, content.size.x, content.size.y, content.size.w, content.size.h);
    ring_write(&output, &content, sizeof (struct widget_textbox));
    ring_copy(&output, &text);
    ring_copy(&output, &input1);
    ring_copy(&output, &input2);
    ring_write(&output, "\n", 1);

}

static void removecontent(struct event_header *header)
{

    widget_remove(&output, &content, WIDGET_Z_MIDDLE, header->destination);

}

static void removerows(unsigned int count)
{

    while (count--)
    {

        ring_skip(&text, ring_find(&text, '\n') + 1);

        totalrows--;

    }

}

static void copybuffer(void *buffer, unsigned int count)
{

    char *b = buffer;
    unsigned int i;

    for (i = 0; i < count; i++)
    {

        if (!ring_avail(&text))
            removerows(1);

        if (b[i] == '\n')
        {

            totalrows++;

            if (totalrows >= visiblerows)
                removerows(totalrows - visiblerows + 1);

        }

        ring_write(&text, &b[i], 1);

    }

}

static void copyring(struct ring *ring)
{

    char buffer[FUDGE_BSIZE];
    unsigned int count;
    unsigned int head = ring->head;
    unsigned int tail = ring->tail;

    while ((count = ring_read(ring, buffer, FUDGE_BSIZE)))
        copybuffer(buffer, count);

    ring->head = head;
    ring->tail = tail;

}

static unsigned int interpretbuiltin(unsigned int count, char *command)
{

    if (memory_match(command, "exit", 4))
    {

        quit = 1;

        return 1;

    }

    if (memory_match(command, "cd ", 3))
    {

        command[count - 1] = '\0';

        if (file_walk(FILE_L8, command + 3))
        {

            file_duplicate(FILE_PW, FILE_L8);
            file_duplicate(FILE_CW, FILE_L8);

        }

        return 1;

    }

    return 0;

}

static void interpretslang(unsigned int count, char *command)
{

    char buffer[FUDGE_BSIZE];

    if (!file_walk(FILE_CP, "/bin/slang"))
        return;

    if (!file_walk(FILE_LA, "/system/pipe/clone"))
        return;

    if (!file_walk(FILE_LB, "/system/pipe/clone"))
        return;

    file_open(FILE_LA);
    file_open(FILE_LB);
    file_walkfrom(FILE_CI, FILE_LA, "idata");
    file_walkfrom(FILE_LC, FILE_LA, "odata");
    file_walkfrom(FILE_LD, FILE_LB, "idata");
    file_walkfrom(FILE_CO, FILE_LB, "odata");
    call_spawn();
    file_open(FILE_LC);
    file_writeall(FILE_LC, command, count);
    file_close(FILE_LC);
    file_open(FILE_LD);

    while ((count = file_read(FILE_LD, buffer, FUDGE_BSIZE)))
        copybuffer(buffer, count);

    file_close(FILE_LD);
    file_close(FILE_LB);
    file_close(FILE_LA);

}

static void interpret(struct ring *ring)
{

    char command[FUDGE_BSIZE];
    unsigned int count = ring_read(ring, command, FUDGE_BSIZE);

    if (count < 2)
        return;

    if (!interpretbuiltin(count, command))
        interpretslang(count, command);

}

static void moveleft(unsigned int steps)
{

    char buffer[FUDGE_BSIZE];

    ring_writereverse(&input2, buffer, ring_readreverse(&input1, buffer, steps));

}

static void moveright(unsigned int steps)
{

    char buffer[FUDGE_BSIZE];

    ring_write(&input1, buffer, ring_read(&input2, buffer, steps));

}

static void onkill(struct event_header *header, void *data)
{

    quit = 1;

}

static void onwmkeypress(struct event_header *header, void *data)
{

    struct event_wmkeypress *wmkeypress = data;
    struct keymap *keymap = keymap_load(KEYMAP_US);
    struct keycode *keycode = keymap_getkeycode(keymap, wmkeypress->scancode, keymod);

    keymod = keymap_modkey(wmkeypress->scancode, keymod);

    switch (wmkeypress->scancode)
    {

    case 0x0E:
        ring_skipreverse(&input1, 1);
        updatecontent(header);

        break;

    case 0x1C:
        ring_move(&input1, &input2);
        ring_write(&input1, keycode->value, keycode->length);
        copyring(&input1);
        interpret(&input1);
        copybuffer("$ ", 2);
        updatecontent(header);

        break;

    case 0x16:
        if (keymod & KEYMOD_CTRL)
            ring_reset(&input1);
        else
            ring_write(&input1, keycode->value, keycode->length);

        updatecontent(header);

        break;

    case 0x25:
        if (keymod & KEYMOD_CTRL)
            ring_reset(&input2);
        else
            ring_write(&input1, keycode->value, keycode->length);

        updatecontent(header);

        break;

    case 0x26:
        if (keymod & KEYMOD_CTRL)
            removerows(totalrows);
        else
            ring_write(&input1, keycode->value, keycode->length);

        updatecontent(header);

        break;

    case 0x47:
        moveleft(ring_count(&input1));
        updatecontent(header);

        break;

    case 0x4B:
        moveleft(1);
        updatecontent(header);

        break;

    case 0x4D:
        moveright(1);
        updatecontent(header);

        break;

    case 0x4F:
        moveright(ring_count(&input2));
        updatecontent(header);

        break;

    default:
        ring_write(&input1, keycode->value, keycode->length);
        updatecontent(header);

        break;

    }

}

static void onwmkeyrelease(struct event_header *header, void *data)
{

    struct event_wmkeyrelease *wmkeyrelease = data;

    keymod = keymap_modkey(wmkeyrelease->scancode, keymod);

}

static void onwmresize(struct event_header *header, void *data)
{

    struct event_wmresize *wmresize = data;

    box_setsize(&content.size, wmresize->x, wmresize->y, wmresize->w, wmresize->h);
    box_resize(&content.size, wmresize->padding);

    visiblerows = content.size.h / wmresize->lineheight;

    if (totalrows >= visiblerows)
        removerows(totalrows - visiblerows + 1);

}

static void onwmshow(struct event_header *header, void *data)
{

    updatecontent(header);

}

static void onwmhide(struct event_header *header, void *data)
{

    removecontent(header);

}

void main(void)
{

    ring_init(&output, FUDGE_BSIZE, outputdata);
    ring_init(&input1, FUDGE_BSIZE, inputdata1);
    ring_init(&input2, FUDGE_BSIZE, inputdata2);
    ring_init(&text, FUDGE_BSIZE, textdata);
    widget_inittextbox(&content);
    copybuffer("$ ", 2);

    if (!file_walk(FILE_L0, "/system/event"))
        return;

    if (!file_walk(FILE_L1, "/system/wm/event"))
        return;

    file_open(FILE_L0);
    file_open(FILE_L1);
    event_sendwmmap(FILE_L1, EVENT_ADDR_BROADCAST);

    while (!quit)
    {

        struct event event;

        event_read(FILE_L0, &event);

        switch (event.header.type)
        {

        case EVENT_KILL:
            onkill(&event.header, event.data);

            break;

        case EVENT_WMKEYPRESS:
            onwmkeypress(&event.header, event.data);

            break;

        case EVENT_WMKEYRELEASE:
            onwmkeyrelease(&event.header, event.data);

            break;

        case EVENT_WMRESIZE:
            onwmresize(&event.header, event.data);

            break;

        case EVENT_WMSHOW:
            onwmshow(&event.header, event.data);

            break;

        case EVENT_WMHIDE:
            onwmhide(&event.header, event.data);

            break;

        }

        if (ring_count(&output))
        {

            event_sendwmflush(FILE_L1, EVENT_ADDR_BROADCAST, ring_count(&output), outputdata);
            ring_reset(&output);

        }

    }

    event_sendwmunmap(FILE_L1, EVENT_ADDR_BROADCAST);
    file_close(FILE_L1);
    file_close(FILE_L0);

}

