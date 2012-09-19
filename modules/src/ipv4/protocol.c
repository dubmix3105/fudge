#include <memory.h>
#include <vfs.h>
#include <net/net.h>
#include <ipv4/ipv4.h>

void ipv4_protocol_init(struct ipv4_protocol *protocol)
{

    memory_clear(protocol, sizeof (struct ipv4_protocol));

    protocol->base.name = "ipv4";
/*    protocol->read_data = read_data; */
/*    protocol->write_data = write_data; */

}

