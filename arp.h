#ifndef _ARP_H_
#define _ARP_H_

#include <linux/types.h>

#define ETHTYPE_ARP 0x0806
#define ETHTYPE_IP  0x0800

#define ARP_REQUEST     1
#define ARP_REPLY       2

#define ARP_PENDING     0
#define ARP_COMPLETE    1

// 3 minutes timeout
#define ARP_PENDING_TIMEOUT 180 

struct ethhdr2 {
    __u8 dm[6];
    __u8 sm[6];
    __u16 ht;
}__attribute__((packed));

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
}__attribute__((packed));

struct arp_queue {
    struct arp_queue *next;
    struct tbuf *buf;
};

struct arpentry {
    struct arpentry *next;
    struct arp_queue *queue; 
    __u8 haddr[6];
    __u16 status;
    __u32 addr;
    time_t time;
};

struct arpentry *arp_table;
#define GETARP() \
    arp_table->next
#define ADDARP(entry) \
    entry->next = arp_table->next; \
    arp_table->next = entry
int ether_in(struct tbuf *buf);
int ether_out(struct tbuf *buf, int type, struct iface *dev);
struct arpentry *arp_lookup(__u32 addr);
int arp_cleanup();
void dump_mac(__u8 *haddr);
#endif
