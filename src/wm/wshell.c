#include <fudge.h>
#include <abi.h>
#include <widget.h>

static struct widget_textbox content;
static unsigned int keymod = KEYMOD_NONE;
static char outputdata[FUDGE_BSIZE];
static struct ring output;
static char promptdata[FUDGE_BSIZE];
static struct ring prompt;
static char inputdata1[FUDGE_BSIZE];
static struct ring input1;
static char inputdata2[FUDGE_BSIZE];
static struct ring input2;
static char textdata[FUDGE_BSIZE];
static struct ring text;
static unsigned int totalrows;
static unsigned int visiblerows;
static unsigned int idcomplete;
static unsigned int idslang;

static void updatecontent(void)
{

    content.length = ring_count(&text) + ring_count(&prompt) + ring_count(&input1) + ring_count(&input2) + 1;
    content.cursor = ring_count(&text) + ring_count(&prompt) + ring_count(&input1);

    widget_update(&output, &content, WIDGET_Z_MIDDLE, WIDGET_TYPE_TEXTBOX, sizeof (struct widget_textbox) + content.length, &content.size);
    ring_write(&output, &content, sizeof (struct widget_textbox));
    ring_copy(&output, &text);
    ring_copy(&output, &prompt);
    ring_copy(&output, &input1);
    ring_copy(&output, &input2);
    ring_write(&output, "\n", 1);

}

static void removecontent(void)
{

    widget_remove(&output, &content, WIDGET_Z_MIDDLE);

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

static unsigned int interpretbuiltin(unsigned int count, char *data)
{

    if (memory_match(data, "cd ", 3))
    {

        data[count - 1] = '\0';

        if (file_walk2(FILE_L0, data + 3))
        {

            file_duplicate(FILE_PW, FILE_L0);
            file_duplicate(FILE_CW, FILE_L0);

        }

        return 1;

    }

    return 0;

}

static void interpret(struct channel *channel, struct ring *ring)
{

    char data[FUDGE_BSIZE];
    unsigned int count = ring_read(ring, data, FUDGE_BSIZE);

    if (count < 2)
        return;

    if (interpretbuiltin(count, data))
        return;

    channel_request(channel, EVENT_DATA);
    channel_append(channel, count, data);
    channel_place(channel, idslang);

}

static void complete(struct channel *channel, struct ring *ring)
{

    char data[FUDGE_BSIZE];
    unsigned int count = ring_read(ring, data, FUDGE_BSIZE);

    channel_request(channel, EVENT_DATA);
    channel_append(channel, count, data);
    channel_place(channel, idcomplete);

}

static void ondata(struct channel *channel, void *mdata, unsigned int msize)
{

    if (channel->source == idcomplete)
    {

        if (memory_findbyte(mdata, msize, '\n') < msize - 1)
        {

            copyring(&prompt);
            copybuffer("\n", 1);
            copybuffer(mdata, msize);

        }

        else
        {

            ring_write(&input1, mdata, msize - 1);

        }

    }

    else if (channel->source == idslang)
    {

        struct job_status status;
        struct job jobs[32];

        status.start = mdata;
        status.end = status.start + msize;

        while (status.start < status.end)
        {

            unsigned int njobs = job_parse(&status, jobs, 32);

            job_run(channel, jobs, njobs);

        }

    }

    else
    {

        copybuffer(mdata, msize);

    }

    updatecontent();

}

static void onwmconfigure(struct channel *channel, void *mdata, unsigned int msize)
{

    struct event_wmconfigure *wmconfigure = mdata;

    box_setsize(&content.size, wmconfigure->x, wmconfigure->y, wmconfigure->w, wmconfigure->h);
    box_resize(&content.size, wmconfigure->padding);

    visiblerows = content.size.h / wmconfigure->lineheight;

    if (totalrows >= visiblerows)
        removerows(totalrows - visiblerows + 1);

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

    case 0x0F:
        ring_move(&input1, &input2);
        complete(channel, &input1);

        break;

    case 0x1C:
        ring_move(&input1, &input2);
        ring_write(&input1, keycode->value, keycode->length);
        copyring(&prompt);
        copyring(&input1);
        interpret(channel, &input1);

        break;

    case 0x16:
        if (keymod & KEYMOD_CTRL)
            ring_reset(&input1);
        else
            ring_write(&input1, keycode->value, keycode->length);

        break;

    case 0x25:
        if (keymod & KEYMOD_CTRL)
            ring_reset(&input2);
        else
            ring_write(&input1, keycode->value, keycode->length);

        break;

    case 0x26:
        if (keymod & KEYMOD_CTRL)
            removerows(totalrows);
        else
            ring_write(&input1, keycode->value, keycode->length);

        break;

    case 0x47:
        moveleft(ring_count(&input1));

        break;

    case 0x4B:
        moveleft(1);

        break;

    case 0x4D:
        moveright(1);

        break;

    case 0x4F:
        moveright(ring_count(&input2));

        break;

    default:
        ring_write(&input1, keycode->value, keycode->length);

        break;

    }

    updatecontent();

}

static void onwmkeyrelease(struct channel *channel, void *mdata, unsigned int msize)
{

    struct event_wmkeyrelease *wmkeyrelease = mdata;

    keymod = keymap_modkey(wmkeyrelease->scancode, keymod);

}

static void onwmshow(struct channel *channel, void *mdata, unsigned int msize)
{

    updatecontent();

}

static void onwmhide(struct channel *channel, void *mdata, unsigned int msize)
{

    removecontent();

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
    channel_setsignal(&channel, EVENT_DATA, ondata);
    channel_setsignal(&channel, EVENT_WMCONFIGURE, onwmconfigure);
    channel_setsignal(&channel, EVENT_WMKEYPRESS, onwmkeypress);
    channel_setsignal(&channel, EVENT_WMKEYRELEASE, onwmkeyrelease);
    channel_setsignal(&channel, EVENT_WMSHOW, onwmshow);
    channel_setsignal(&channel, EVENT_WMHIDE, onwmhide);
    channel_setsignal(&channel, EVENT_WMCLOSE, onwmclose);
    ring_init(&output, FUDGE_BSIZE, outputdata);
    ring_init(&prompt, FUDGE_BSIZE, promptdata);
    ring_init(&input1, FUDGE_BSIZE, inputdata1);
    ring_init(&input2, FUDGE_BSIZE, inputdata2);
    ring_init(&text, FUDGE_BSIZE, textdata);
    ring_write(&prompt, "$ ", 2);
    widget_inittextbox(&content);

    if (!file_walk2(FILE_G0, "/system/multicast"))
        return;

    if (!file_walk2(FILE_CP, "/bin/complete"))
        return;

    idcomplete = call_spawn(FILE_CP);

    if (!file_walk2(FILE_CP, "/bin/slang"))
        return;

    idslang = call_spawn(FILE_CP);

    file_open(FILE_G0);
    channel_request(&channel, EVENT_WMMAP);
    channel_write(&channel, FILE_G0);
    channel_listen(&channel);
    channel_request(&channel, EVENT_DONE);
    channel_place(&channel, idcomplete);
    channel_place(&channel, idslang);
    file_close(FILE_G0);

}

