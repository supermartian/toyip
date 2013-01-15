#include "icmp.h"
#include "ip.h"
#include "tbuf.h"

int icmp_out(struct tbuf *buf)
{
}

void icmp_in(struct tbuf *buf)
{
    struct icmphdr *icmp;
    icmp = (struct icmphdr *)buf->payload;
    printf("type:%d, code:%d\n", icmp->type, icmp->code);
    switch (icmp->type) {
        case ICMP_ECHO:
            break;
        case ICMP_TS:
            icmp_tsr(buf);
            break;
    }
}

static void icmp_tsr(struct tubf *in)
{
    struct iphdr *in_ip;
    struct icmphdr *out;
    struct icmphdr *icmp;
    __u32 *time;
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
    time = (__u32 *)out;
    time++;
    *time = ((__u32 *)icmp->seqno + 1);
    time++;
    *time = htonl(time(NULL));
    time++;
    *time = htonl(time(NULL));
    out->check = checksum_generic(out, sizeof(struct icmphdr) + sizeof(__u32) * 3);

    tbuf_header(in, 0);
    in_ip = in->payload;
    ip_out(out, in_ip->daddr, in_ip->saddr, 32, IP_ICMP);
}

void icmp_init()
{
    l3protos[IP_ICMP].type = IP_ICMP;
    l3protos[IP_ICMP].handler = &icmp_in;
    printf("init icmp\n"); 
}
