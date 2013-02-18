#include <stdlib.h>
#include "icmp.h"
#include "ip.h"
#include "tbuf.h"

static void icmp_echo(struct tbuf *in);
static void icmp_tsr(struct tbuf *in);

int icmp_out(struct tbuf *buf)
{
}

void icmp_in(struct tbuf *buf)
{
    struct icmphdr *icmp;
    icmp = (struct icmphdr *)buf->payload;
    switch (icmp->type) {
        case ICMP_ECHO:
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
    l3protos[IP_ICMP].handler = &icmp_in;
    printf("init icmp\n"); 
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
    in_ip = in->payload;
    saddr = in_ip->daddr;
    daddr = in_ip->saddr;
    tot_len = in_ip->tot_len;

    out = tbuf_malloc(in->len);
    memcpy(out->payload, in->payload, in->len);

    tbuf_header(out, IP_HLEN);
    out_icmp = out->payload;
    out_icmp->type = ICMP_ER;
    out_icmp->check = 0;
    out_icmp->check = htons(checksum_generic(out_icmp, out->len - IP_HLEN));
    
    ip_out(out, in_ip->daddr, in_ip->saddr, 32, IP_ICMP);
}

static void icmp_tsr(struct tbuf *in)
{
    struct iphdr *in_ip;
    struct icmphdr *out;
    struct icmphdr *icmp;
    __u32 *etime;
    struct tbuf *buf;
    
    buf = tbuf_malloc(sizeof(struct icmphdr) + sizeof(struct iphdr)
            + sizeof(__u32) * 3);
    tbuf_header(buf, IP_HLEN);
    out = buf->payload;
    tbuf_header(in, IP_HLEN);
    icmp = in->payload;

    out->type = ICMP_TSR;
    out->code = 0;
    out->id = icmp->id;
    out->seqno = icmp->seqno;
    etime = (__u32 *)out;
    etime++;
    *etime = ((__u32 *)icmp->seqno + 1);
    etime++;
    *etime = htonl(time(NULL));
    etime++;
    *etime = htonl(time(NULL));
    out->check = checksum_generic(out, sizeof(struct icmphdr) + sizeof(__u32) * 3);

    tbuf_header(in, 0);
    in_ip = in->payload;
    ip_out(out, in_ip->daddr, in_ip->saddr, 32, IP_ICMP);
}

