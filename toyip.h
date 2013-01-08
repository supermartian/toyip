#ifndef _TOYIP_H_
#define _TOYIP_H

#include <linux/types.h>

#include "tbuf.h"

#define MTU_LEN 1500
#define PROTO_NUM 32

int l3_sock;
struct iphdr {
        __u8     ihl:4,
             version:4;
        __u8    tos;
        __be16  tot_len;
        __be16  id; 
        __be16  frag_off;
        __u8    ttl;
        __u8    protocol;
        __sum16 check;
        __be32  saddr;
        __be32  daddr;
         /*The options start here. */
};

#define IP_ICMP 1
#define IP_UDP  1
#define IP_TCP  1

typedef void *(*l3_handler)(struct tbuf *buf);

struct l3proto {
    __u8 type;
    l3_handler handler; 
} l3protos[PROTO_NUM];

int ip_out(struct tbuf *buf);
int ip_in(struct tbuf *buf);
int ip_init();
#endif
