#include "icmp.h"
#include "ip.h"
#include "tbuf.h"

int icmp_out(struct tbuf *buf)
{
}

void icmp_in(struct tbuf *buf)
{
    printf("handling\n"); 
}

void icmp_init()
{
    l3protos[IP_ICMP].type = IP_ICMP;
    l3protos[IP_ICMP].handler = &icmp_in;
    printf("init icmp\n"); 
}
