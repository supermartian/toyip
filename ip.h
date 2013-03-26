#ifndef _IP_H_
#define _IP_H_

#include <linux/types.h>

#include "tbuf.h"
#include "iface.h"

#define MTU_LEN 1500
#define PROTO_NUM 32

struct iphdr {
    __u8 ihl:4,
         version:4;
    __u8   tos;
    __u16  tot_len;
    __u16  id; 
    __u16  frag_off;
    __u8   ttl;
    __u8   protocol;
    __u16  check;
    __u32  saddr;
    __u32  daddr;
     /*The options start here. */
}__attribute__((packed));

#define IP_ICMP 1
#define IP_UDP  17
#define IP_TCP  6

struct route {
    struct route* next;
    struct iface* dev;
    __u32 isdefault;
    __u32 gateway;
    __u32 daddr;
    __u32 mask;
};
struct route *route_table;
struct route *default_route;
#define GETROUTE() \
    route_table->next
#define ip_netcmp(ip, mask, net) \
    (net & mask) == (ip & mask)

typedef void *(*l3_handler)(struct tbuf *buf);

struct l3proto {
    __u8 type;
    l3_handler handler; 
} l3protos[PROTO_NUM];


void ip_to_string(char *ip, __u32 yiaddr);
void dump_ip(__u32 addr);
void set_default_route(struct route *gw);
int ip_out(struct tbuf *buf, __u32 src, __u32 dst, __u8 ttl,
        __u8 protocol);
int ip_in(struct tbuf *buf);
int ip_init();
#endif
