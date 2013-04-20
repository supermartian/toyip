#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <netinet/in.h>

#include "tbuf.h"
#include "iface.h"
#include "icmp.h"
#include "ip.h"
#include "checksum.h"
#include "iface.h"
#include "arp.h"

void ip_to_string(char *ip, __u32 yiaddr)
{
    int a, b, c, d;
    if (ip == NULL) {
        return;
    }
    a = yiaddr >> 24;
    b = (yiaddr << 8) >> 24;
    c = (yiaddr << 16) >> 24;
    d = (yiaddr << 24) >> 24;
    if (a > 255 || b > 255 || c > 255 || d > 255 || a < 0 || b < 0 || c < 0
    || d < 0)
        return;
    sprintf(ip, "%d.%d.%d.%d", d, c, b, a);
}

void dump_ip(__u32 addr)
{
    char ip[30];
    ip_to_string(ip, addr);
    printf("ip: %s\n", ip);
}

int ip_in(struct tbuf *buf)
{
    struct iphdr *ip;
    __u16 checksum;

    tbuf_header(buf, ETH_HLEN);
    ip = (struct iphdr *) buf->payload;
    checksum = checksum_generic((__u8 *)ip, ip->ihl * 4);

    // Discard bogus packets
    if (checksum) {
        printf("screwed packet, dropped\n");
        return 0;
    }

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
    struct ethhdr2 *ether;
    struct route *dstroute;
    struct iface *dstdev;

    // rewind to the IP head of payload 
    tbuf_header(buf, ETH_HLEN);
    ip = (struct iphdr *)buf->payload;
    ip->version = 4;
    ip->ttl = ttl; 
    ip->saddr = src;
    ip->daddr = dst;
    ip->protocol = protocol;
    ip->check = 0;
    ip->check = htons(checksum_generic((__u8 *)ip, ip->ihl * 4));

    // rewind to the ethernet head of payload
    tbuf_header(buf, 0);
    ether = (struct ethhdr2 *) buf->payload;

    memset(&sin, 0, sizeof(sin));
    ether_out(buf, ETHTYPE_IP, get_iface_by_name("wlan0"));

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
    arp_init();
    l2_init();
    config_iface(get_iface_by_name("wlan0"), 1500, htonl(0xC0A80122), htonl(0xffffff00), htonl(0XC0A80101));
    while(1){
        sleep(10);
    }
}
