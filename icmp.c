#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include "icmp.h"
#include "ip.h"
#include "tbuf.h"
// for test
#include "arp.h"

static void icmp_echo(struct tbuf *in);
static void icmp_tsr(struct tbuf *in);

int icmp_out(struct tbuf *buf)
{
}

void icmp_in(struct tbuf *buf)
{
    struct ethhdr2 *eth;
    struct icmphdr *icmp;
    tbuf_header(buf, 0);
    eth = (struct ethhdr2 *) buf->payload;
    tbuf_header(buf, IP_HLEN);
    icmp = (struct icmphdr *)buf->payload;
    switch (icmp->type) {
        case ICMP_ECHO:
            dump_mac(eth->sm);
            printf("type:%d, code:%d\n", icmp->type, icmp->code);
            icmp_echo(buf);
            break;
        case ICMP_TS:
            icmp_tsr(buf);
            break;
    }
}

void icmp_init()
{
    l3protos[IP_ICMP].type = IP_ICMP;
    l3protos[IP_ICMP].handler = (void *) &icmp_in;
    printf("Initializing ICMP\n"); 
}

static void icmp_echo(struct tbuf *in)
{
    struct iphdr *in_ip;
    struct iphdr *out_ip;
    struct icmphdr *in_icmp;
    struct icmphdr *out_icmp;
    struct tbuf *out;
    __u32 saddr;
    __u32 daddr;
    __u16 tot_len;

    tbuf_header(in, ETH_HLEN);    
    in_ip = (struct iphdr *) in->payload;
    saddr = in_ip->daddr;
    daddr = in_ip->saddr;
    tot_len = in_ip->tot_len;

    out = tbuf_malloc(in->len);
    // just take the input
    memcpy(out->payload, in->payload, in->len);

    tbuf_header(out, IP_HLEN);
    out_icmp = (struct icmphdr *) out->payload;
    out_icmp->type = ICMP_ER;
    out_icmp->check = 0;
    out_icmp->check = htons(checksum_generic(out_icmp, out->len - IP_HLEN));
    
    ip_out(out, in_ip->daddr, in_ip->saddr, 32, IP_ICMP);
}

static void icmp_tsr(struct tbuf *in)
{
}

