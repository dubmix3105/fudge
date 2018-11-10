#include <fudge.h>
#include <abi.h>

static unsigned int onstop(union event_message *imessage, union event_message *omessage)
{

    event_reply(omessage, imessage, EVENT_DATA);
    event_append(omessage, 13, "Hello world!\n");
    event_send(omessage);

    return 1;

}

static unsigned int onkill(union event_message *imessage, union event_message *omessage)
{

    return 1;

}

void main(void)
{

    unsigned int status = 0;
    union event_message imessage;
    union event_message omessage;

    event_open();

    while (!status)
    {

        switch (event_read(&imessage))
        {

        case EVENT_STOP:
            status = onstop(&imessage, &omessage);

            break;

        case EVENT_KILL:
            status = onkill(&imessage, &omessage);

            break;

        }

    }

    event_close();

}

