#include <fudge.h>
#include <abi.h>

static void onredirect(struct channel *channel, void *mdata, unsigned int msize)
{

    struct event_redirect *redirect = mdata;

    channel_setredirect(channel, redirect->type, redirect->id);

}

static void ontimertick(struct channel *channel, void *mdata, unsigned int msize)
{

    unsigned int id = channel_reply(channel, EVENT_DATA);

    channel_appendstring(channel, "HEJ!\n");
    channel_place(channel, id);

}

void main(void)
{

    struct channel channel;

    if (!file_walk2(FILE_G0, "/system/timer/if:0/event"))
        return;

    if (!file_walk2(FILE_G1, "/system/timer/if:0/ctrl"))
        return;

    file_open(FILE_G0);
    channel_init(&channel);
    channel_setsignal(&channel, EVENT_REDIRECT, onredirect);
    channel_setsignal(&channel, EVENT_TIMERTICK, ontimertick);
    channel_listen(&channel);
    file_close(FILE_G0);

}

