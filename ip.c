#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>

#include "tbuf.h"
#include "icmp.h"
#include "ip.h"
#include "checksum.h"
#include "iface.h"
#include "arp.h"

int ip_in(struct tbuf *buf)
{
    struct iphdr *ip;
    __u16 checksum;
    ip = (struct iphdr *)buf->payload;
    checksum = checksum_generic((__u8 *)ip, ip->ihl * 4);

    //printf("protocol type: %d, checksum: %x\n", ip->protocol, checksum);
    // Discard bogus packets
    if (checksum) {
        printf("screwed packet, dropped\n");
        return 0;
    }

    tbuf_header(buf, IP_HLEN);
    if (l3protos[ip->protocol].type == ip->protocol) {
        l3protos[ip->protocol].handler(buf);
    }

    return 0;
}

int ip_out(struct tbuf *buf, __u32 src, __u32 dst, __u8 ttl,
        __u8 protocol)
{
    struct sockaddr_in sin;
    struct iphdr *ip;
    struct ethhdr *ether;
    struct route *dstroute;

    // rewind to the IP head of payload 
    tbuf_header(buf, ETH_HLEN);
    ip = (struct iphdr *)buf->payload;
    ip->version = 4;
    ip->ttl = ttl; 
    ip->saddr = src;
    ip->daddr = dst;
    ip->protocol = protocol;
    ip->check = checksum_generic((__u8 *)ip, ip->ihl * 4);

    // rewind to the ethernet head of payload
    tbuf_header(buf, 0);
    ether = (struct ether *)buf->payload;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = dst;
    lowlevel_send(buf, dstroute);

    tbuf_free(buf);
    return 0;
}

void set_default_route(struct route* gw)
{
    struct route* p = GETROUTE();
    __u32 flag = 0;

    while(p->next != NULL) {
        if (gw == p) {
            p->isdefault = 1;
            flag = 1;
        } else {
            p->isdefault = 0;
        }
        p = p->next;
    }

    // Add this to linked list
    if (!flag) {
        gw->next = p->next;
        p->next = gw;
    }

    default_route = gw;
}

struct route* ip_out_route(__u32 daddr)
{
    struct route* p = GETROUTE();

    while(p->next != NULL) {
        if (ip_netcmp(daddr, p->mask, daddr)) {
            return p;
        }
        p = p->next;        
    }

    return default_route;
}

int ip_init()
{
    // Initialize route table
    route_table = malloc(sizeof(struct route));
    route_table->next = NULL;

    memset(l3protos, 0, PROTO_NUM);
    icmp_init();

    return 0;
}

int main()
{
    ip_init();
    iface_init();
    l2_init();
}
