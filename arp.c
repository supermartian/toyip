#include "arp.h"
#include "tbuf.h"
#include "l2.h"

int ether_in(struct tbuf *buf)
{
    struct ethhdr2 *eth;
    struct arphdr *arp;

    tbuf_header(buf, 0);
    eth = (struct ethhdr2 *)buf->payload;

    printf("now");
    switch (eth->ht) {
        case ETHTYPE_ARP:
    //        do_arp(buf);
            break;
        case ETHTYPE_IP:
            ip_in(buf);
            break;
        default:
            break;
    }

    tbuf_free(buf);
}

int ether_out(struct tbuf *buf)
{
/*
 *  1. lookup arp
 */

    lowlevel_send(buf);
}
