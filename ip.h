#ifndef _IP_H_
#define _IP_H_

#include <linux/types.h>

#include "tbuf.h"

#define MTU_LEN 1500
#define PROTO_NUM 32

int l3_recv_sock;
int l3_send_sock;
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
};

#define IP_ICMP 1
#define IP_UDP  17
#define IP_TCP  6

typedef void *(*l3_handler)(struct tbuf *buf);

struct l3proto {
    __u8 type;
    l3_handler handler; 
} l3protos[PROTO_NUM];

int ip_out(struct tbuf *buf, __u32 src, __u32 dst, __u8 ttl,
        __u8 protocol);
int ip_in(struct tbuf *buf);
int ip_init();
#endif
