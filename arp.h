#ifndef _ARP_H_
#define _ARP_H_

#include <linux/types.h>

#define ETHTYPE_ARP 0x0806
#define ETHTYPE_IP  0x0800
#define ARP_REQUEST 1
#define ARP_REPLY   2

struct ethhdr2 {
    __u8 dm[6];
    __u8 sm[6];
    __u16 ht;
};

struct arphdr {
    __u16 htype;
    __u16 ptype;
    __u8 hlen;
    __u8 plen;
    __u16 op;
    __u8 shaddr[6];
    __u32 saddr;
    __u8 dhaddr[6];
    __u32 daddr;
};

struct arpentry {
    struct arpentry *next;
    struct tbuf *buffer; 
    __u8 haddr[6];
    __u32 addr;
};

struct arpentry *arp_table;

int ether_in(struct tbuf *buf);
int ether_out(struct tbuf *buf);
int arp_lookup(__u32 addr, __u8 *haddr);
#endif
