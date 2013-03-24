#include "arp.h"
#include "tbuf.h"
#include "l2.h"

static void do_arp(struct tbuf *buf)
{
    struct arphdr *arp;
    tbuf_header(buf, ETH_HLEN);

    arp = buf->payload;
    printf("arp htype: %d\n", htons(arp->htype));
}

int ether_in(struct tbuf *buf)
{
    struct ethhdr2 *eth;
    struct arphdr *arp;

    tbuf_header(buf, 0);
    eth = (struct ethhdr2 *) buf->payload;

    printf("ether in: %x\n", htons(eth->ht));
    switch (htons(eth->ht)) {
        case ETHTYPE_ARP:
            do_arp(buf);
            break;
        case ETHTYPE_IP:
            ip_in(buf);
            break;
        default:
            break;
    }
}

int ether_out(struct tbuf *buf)
{
/*
 *  1. lookup arp
 */

    lowlevel_send(buf);
}
