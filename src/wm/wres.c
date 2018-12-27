#include <fudge.h>
#include <abi.h>
#include <widget/widget.h>
#include <widget/render.h>

static unsigned int oninit(struct event_channel *channel)
{

    event_request(channel, EVENT_WMMAP, 0);
    file_writeall(FILE_G0, &channel->o, channel->o.header.length);

    return 0;

}

static unsigned int onkill(struct event_channel *channel)
{

    event_request(channel, EVENT_WMUNMAP, 0);
    file_writeall(FILE_G0, &channel->o, channel->o.header.length);

    return 1;

}

static unsigned int onwmmousepress(struct event_channel *channel)
{

    struct event_wmmousepress *wmmousepress = event_getdata(&channel->i);

    switch (wmmousepress->button)
    {

    case 0x01:
        if (!file_walk2(FILE_L0, "/system/video/if:0/ctrl"))
            break;

        render_init();
        render_setvideo(FILE_L0, 1920, 1080, 4);

        break;

    }

    return 0;

}

void main(void)
{

    unsigned int status = 0;
    struct event_channel channel;

    if (!file_walk2(FILE_G0, "/system/multicast"))
        return;

    file_open(FILE_G0);

    while (!status)
    {

        switch (event_pick(&channel))
        {

        case EVENT_INIT:
            status = oninit(&channel);

            break;

        case EVENT_KILL:
            status = onkill(&channel);

            break;

        case EVENT_WMMOUSEPRESS:
            status = onwmmousepress(&channel);

            break;

        }

    }

    file_close(FILE_G0);

}

