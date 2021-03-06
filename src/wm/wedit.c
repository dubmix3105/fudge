#include <fudge.h>
#include <abi.h>
#include <widget.h>

static struct widget_textbox content;
static struct widget_text status;
static unsigned int keymod = KEYMOD_NONE;
static char outputdata[FUDGE_BSIZE];
static struct ring output;
static char inputdata1[FUDGE_BSIZE];
static struct ring input1;
static char inputdata2[FUDGE_BSIZE];
static struct ring input2;

static void updatecontent(void)
{

    content.length = ring_count(&input1) + ring_count(&input2) + 1;
    content.cursor = ring_count(&input1);

    widget_update(&output, &content, WIDGET_Z_MIDDLE, WIDGET_TYPE_TEXTBOX, sizeof (struct widget_textbox) + content.length, &content.size);
    ring_write(&output, &content, sizeof (struct widget_textbox));
    ring_copy(&output, &input1);
    ring_copy(&output, &input2);
    ring_write(&output, "\n", 1);

}

static void updatestatus(void)
{

    status.length = 18;

    widget_update(&output, &status, WIDGET_Z_MIDDLE, WIDGET_TYPE_TEXT, sizeof (struct widget_text) + status.length, &status.size);
    ring_write(&output, &status, sizeof (struct widget_text));
    ring_write(&output, "^S: Save, ^Q: Quit", 18);

}

static void removecontent(void)
{

    widget_remove(&output, &content, WIDGET_Z_MIDDLE);

}

static void removestatus(void)
{

    widget_remove(&output, &status, WIDGET_Z_MIDDLE);

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

static void moveup(void)
{

    unsigned int offset1;
    unsigned int offset2;

    offset1 = ring_findreverse(&input1, '\n');

    moveleft(ring_findreverse(&input1, '\n') + 1);

    offset2 = ring_findreverse(&input1, '\n');

    moveleft(offset2 - (offset1 < offset2 ? offset1 : offset2));

}

static void movedown(void)
{

    unsigned int offset1;
    unsigned int offset2;

    offset1 = ring_findreverse(&input1, '\n');

    moveright(ring_find(&input2, '\n') + 1);

    offset2 = ring_find(&input2, '\n');

    moveright(offset1 < offset2 ? offset1 : offset2);

}

static unsigned int readfile(unsigned int visiblerows)
{

    char buffer[FUDGE_BSIZE];
    unsigned int rows = 0;
    unsigned int count;

    file_open(FILE_G1);

    while ((count = file_read(FILE_G1, buffer, FUDGE_BSIZE)))
    {

        unsigned int i;

        for (i = 0; i < count; i++)
        {

            ring_write(&input2, &buffer[i], 1);

            if (buffer[i] == '\n')
            {

                rows++;

                if (rows >= visiblerows)
                    return rows;

            }

        }

    }

    file_close(FILE_G1);

    return rows;

}

static void onfile(struct channel *channel, void *mdata, unsigned int msize)
{

    if (!file_walk2(FILE_G1, mdata))
        return;

}

static void onwmconfigure(struct channel *channel, void *mdata, unsigned int msize)
{

    struct event_wmconfigure *wmconfigure = mdata;

    ring_reset(&input1);
    ring_reset(&input2);
    box_setsize(&content.size, wmconfigure->x, wmconfigure->y, wmconfigure->w, wmconfigure->h - (wmconfigure->lineheight + 2 * wmconfigure->padding));
    box_resize(&content.size, wmconfigure->padding);
    box_setsize(&status.size, wmconfigure->x, wmconfigure->y + wmconfigure->h - (wmconfigure->lineheight + 2 * wmconfigure->padding), wmconfigure->w, (wmconfigure->lineheight + 2 * wmconfigure->padding));
    box_resize(&status.size, wmconfigure->padding);
    readfile(content.size.h / wmconfigure->lineheight);

}

static void onwmkeypress(struct channel *channel, void *mdata, unsigned int msize)
{

    struct event_wmkeypress *wmkeypress = mdata;
    struct keymap *keymap = keymap_load(KEYMAP_US);
    struct keycode *keycode = keymap_getkeycode(keymap, wmkeypress->scancode, keymod);

    keymod = keymap_modkey(wmkeypress->scancode, keymod);

    switch (wmkeypress->scancode)
    {

    case 0x0E:
        ring_skipreverse(&input1, 1);

        break;

    case 0x47:
        moveleft(ring_findreverse(&input1, '\n'));

        break;

    case 0x48:
        moveup();

        break;

    case 0x4B:
        moveleft(1);

        break;

    case 0x4D:
        moveright(1);

        break;

    case 0x4F:
        moveright(ring_find(&input2, '\n'));

        break;

    case 0x50:
        movedown();

        break;

    default:
        ring_write(&input1, keycode->value, keycode->length);

        break;

    }

    updatecontent();
    updatestatus();

}

static void onwmkeyrelease(struct channel *channel, void *mdata, unsigned int msize)
{

    struct event_wmkeyrelease *wmkeyrelease = mdata;

    keymod = keymap_modkey(wmkeyrelease->scancode, keymod);

}

static void onwmshow(struct channel *channel, void *mdata, unsigned int msize)
{

    updatecontent();
    updatestatus();

}

static void onwmhide(struct channel *channel, void *mdata, unsigned int msize)
{

    removecontent();
    removestatus();

}

static void onwmclose(struct channel *channel, void *mdata, unsigned int msize)
{

    unsigned int id = channel_reply(channel, EVENT_WMUNMAP);

    channel_place(channel, id);
    channel_close(channel);

}

static void onany(struct channel *channel, void *mdata, unsigned int msize)
{

    if (ring_count(&output))
    {

        channel_request(channel, EVENT_DATA);
        channel_append(channel, ring_count(&output), outputdata);
        channel_write(channel, FILE_G0);
        ring_reset(&output);

    }

}

void main(void)
{

    struct channel channel;

    channel_init(&channel);
    channel_setsignal(&channel, EVENT_ANY, onany);
    channel_setsignal(&channel, EVENT_FILE, onfile);
    channel_setsignal(&channel, EVENT_WMCONFIGURE, onwmconfigure);
    channel_setsignal(&channel, EVENT_WMKEYPRESS, onwmkeypress);
    channel_setsignal(&channel, EVENT_WMKEYRELEASE, onwmkeyrelease);
    channel_setsignal(&channel, EVENT_WMSHOW, onwmshow);
    channel_setsignal(&channel, EVENT_WMHIDE, onwmhide);
    channel_setsignal(&channel, EVENT_WMCLOSE, onwmclose);
    ring_init(&output, FUDGE_BSIZE, outputdata);
    ring_init(&input1, FUDGE_BSIZE, inputdata1);
    ring_init(&input2, FUDGE_BSIZE, inputdata2);
    widget_inittextbox(&content);
    widget_inittext(&status, WIDGET_TEXTTYPE_HIGHLIGHT);

    if (!file_walk2(FILE_G0, "/system/multicast"))
        return;

    file_open(FILE_G0);
    channel_request(&channel, EVENT_WMMAP);
    channel_write(&channel, FILE_G0);
    channel_listen(&channel);
    file_close(FILE_G0);

}

