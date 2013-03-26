#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "ip.h"
#include "iface.h"
#include "arp.h"
#include "tbuf.h"
#include "l2.h"

void dump_mac(__u8 *haddr)
{
    printf("haddr: %x:%x:%x:%x:%x:%x\n", haddr[0],
            haddr[1],
            haddr[2],
            haddr[3],
            haddr[4],
            haddr[5]);
}

static void dump_arp_entry(struct arpentry *entry)
{
    char ip[30];
    ip_to_string(ip, entry->addr);
    printf("haddr: %x:%x:%x:%x:%x:%x\nipaddr: %s\n", entry->haddr[0],
            entry->haddr[1],
            entry->haddr[2],
            entry->haddr[3],
            entry->haddr[4],
            entry->haddr[5],
            ip);
}

static struct tbuf *build_arp(__u32 saddr, __u8 *shaddr, __u32 daddr, __u8 *dhaddr, int op);
static void update_arp_entry(__u32 addr, __u8 *haddr, struct iface *dev);
static void update_arp_entry(__u32 addr, __u8 *haddr, struct iface *dev)
{
    struct arpentry *entry;
    struct arpentry *new;
    entry = GETARP();

    while (entry) {
        if (addr == entry->addr) {
            memcpy(entry->haddr, haddr, sizeof(__u8) * HWADDR_LEN);
            if (entry->status == ARP_PENDING) {
                entry->status = ARP_COMPLETE;

                // send out pending packets
                struct arp_queue *q = entry->queue;
                struct arp_queue *n;
                while (q) {
                    n = q->next;
                    free(q);
                    q = n;
                }
            }
            return;
        }
        entry = entry->next;
    }

    // We can't find a match, create a new one
    if (entry == NULL) {
        new = malloc(sizeof(struct arpentry));
        new->addr = addr;
        memcpy(new->haddr, haddr, sizeof(__u8) * HWADDR_LEN);
        new->queue = NULL;
        new->time = time(NULL);
        new->status = ARP_COMPLETE;

        ADDARP(new);
        dump_arp_entry(new);
    }
}

static void do_arp(struct tbuf *buf)
{
    struct iface *self;
    struct arphdr *arp;
    struct tbuf *arpbuf;
    tbuf_header(buf, ETH_HLEN);

    arp = (struct arphdr *) buf->payload;

    switch (ntohs(arp->op)) {
        case ARP_REQUEST:
            printf("Arp Request for: ");
            dump_ip(arp->daddr);
            self = get_iface_by_ip(arp->daddr);

            if (self != NULL) {
                arpbuf = build_arp(self->ipaddr, self->hwaddr, arp->saddr, arp->shaddr, ARP_REPLY);
                printf("Arp out\n");
                lowlevel_send(arpbuf, self);
            }
            break;
        case ARP_REPLY:
            self = get_iface_by_mac(arp->dhaddr);

            // Not send to me
            if (self == NULL) {
                printf("No matched interface is found.\n");
                return;
            }

            update_arp_entry(ntohl(arp->saddr), arp->shaddr, self);
            break;
        default:
            break;
    }
}

static struct tbuf *build_arp(__u32 saddr, __u8 *shaddr, __u32 daddr, __u8 *dhaddr, int op)
{
    struct iface *dev;
    struct tbuf *arpbuf;
    struct ethhdr2 *eth;
    struct arphdr *arp;

    arpbuf = tbuf_malloc(sizeof(struct ethhdr2) + sizeof(struct arphdr));
    if (arpbuf == NULL) {
        return arpbuf;
    }

    tbuf_header(arpbuf, 0);
    eth = (struct ethhdr2 *) arpbuf->payload;
    tbuf_header(arpbuf, ETH_HLEN);
    arp = (struct arphdr *) arpbuf->payload;

    eth->ht = htons(ETHTYPE_ARP);
    arp->htype = htons(0x1);
    arp->ptype = htons(0x0800);
    arp->hlen = 6;
    arp->plen = 4;
    arp->op = htons(op);

    if (dhaddr == NULL) {
        memset(eth->dm, 0xff, HWADDR_LEN);
        memset(arp->dhaddr, 0x0, HWADDR_LEN);
    } else {
        memcpy(eth->dm, dhaddr, HWADDR_LEN);
        memcpy(arp->dhaddr, dhaddr, HWADDR_LEN);
    }

    arp->daddr = htonl(daddr);
    arp->saddr = htonl(saddr);
    memcpy(eth->sm, shaddr, HWADDR_LEN);
    memcpy(arp->shaddr, shaddr, HWADDR_LEN);

    return arpbuf;
}

int ether_in(struct tbuf *buf)
{
    struct ethhdr2 *eth;
    struct arphdr *arp;

    tbuf_header(buf, 0);
    eth = (struct ethhdr2 *) buf->payload;

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

int ether_out(struct tbuf *buf, int type, struct iface *dev)
{
/*
 *  1. lookup arp
 *  2. send arp
 *  3. cache the packets, wait for the arp reply
 *  4. send the packets
 */
    struct arpentry *arp;
    struct ethhdr2 *eth;
    struct iphdr *ip;
    __u32 daddr;
    __u8 *dhaddr;

    tbuf_header(buf, 0);
    eth = (struct ethhdr2 *) buf->payload;
    memcpy(eth->sm, dev->hwaddr, HWADDR_LEN);
    eth->ht = htons(type);

    tbuf_header(buf, ETH_HLEN);
    ip = (struct iphdr *) buf->payload;
    daddr = ntohl(ip->daddr);

    arp = arp_lookup(daddr);
    if (arp != NULL && arp->status == ARP_COMPLETE) {
        printf("ether out\n");
        dhaddr = arp->haddr;
        memcpy(eth->dm, dhaddr, HWADDR_LEN);
        lowlevel_send(buf, dev);
        return 1;
    } else {
        if (arp == NULL) {
            printf("creating entry\n");
            struct arpentry *e;
            e = malloc(sizeof(struct arpentry));
            e->status = ARP_PENDING;
            e->addr = daddr;
            e->queue = malloc(sizeof(struct arp_queue));
            e->queue->next = NULL;
            e->queue->buf = buf;
            ADDARP(e);

            struct tbuf *arpbuf;
            printf("Arp response out\n");
            dump_ip(dev->ipaddr);
            arpbuf = build_arp(dev->ipaddr, dev->hwaddr, daddr, NULL, ARP_REQUEST);
            lowlevel_send(arpbuf, dev);
            return 1;
        } else {
            printf("adding to queue\n");
            struct arp_queue *q;
            q = malloc(sizeof(struct arp_queue));
            q->buf = buf;
            q->next = arp->queue->next;
            arp->queue->next = q;
            return 0;
        }
    }
}

struct arpentry *arp_lookup(__u32 addr)
{
    struct arpentry *entry;
    entry = GETARP();

    while (entry) {
        if (entry->addr == addr) {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

int arp_cleanup()
{
    struct arpentry *entry;
    entry = arp_table;
    while (entry) {
        if (entry->next != NULL) {
            if ((time(NULL) - entry->next->time) >= ARP_PENDING_TIMEOUT &&
                    entry->next->status == ARP_PENDING) {
                struct arp_queue *queue = entry->next->queue;
                struct arp_queue *n;
                while (queue) {
                    // free the pending buffers
                    n = queue->next;
                    tbuf_free(queue->buf);
                    free(queue);
                    queue = n;
                }

                // free the entry
                struct arpentry *e = entry->next;
                entry->next = entry->next->next;
                free(e);
            }
        }

        entry = entry->next;
    }
}

int arp_init()
{
    arp_table = malloc(sizeof(struct arpentry));
    arp_table->next = NULL;
}
