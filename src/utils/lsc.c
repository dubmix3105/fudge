#include <fudge.h>
#include <abi.h>

static void list(struct event_channel *channel, unsigned int descriptor)
{

    struct record record;

    file_open(descriptor);
    event_reply(channel, EVENT_DATA);

    while (file_readall(descriptor, &record, sizeof (struct record)))
    {

        char num[FUDGE_NSIZE];

        if (event_avail(&channel->o) < record.length + 3 + 16)
        {

            event_place(channel->o.header.target, channel);
            event_reset(&channel->o);

        }

        event_append(&channel->o, ascii_wzerovalue(num, FUDGE_NSIZE, record.id, 16, 8, 0), num);
        event_append(&channel->o, 1, " ");
        event_append(&channel->o, ascii_wzerovalue(num, FUDGE_NSIZE, record.size, 16, 8, 0), num);
        event_append(&channel->o, 1, " ");
        event_append(&channel->o, record.length, record.name);
        event_append(&channel->o, 1, "\n");

        if (!file_step(descriptor))
            break;

    }

    event_place(channel->o.header.target, channel);
    file_close(descriptor);

}

static unsigned int onempty(struct event_channel *channel)
{

    list(channel, FILE_PW);

    return 0;

}

static unsigned int onfile(struct event_channel *channel)
{

    struct event_file *file = event_getdata(channel);

    list(channel, file->descriptor);

    return 0;

}

void main(void)
{

    struct event_channel channel;

    event_initsignals();
    event_setsignal(EVENT_EMPTY, onempty);
    event_setsignal(EVENT_FILE, onfile);

    while (event_listen(&channel));

}

