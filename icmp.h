#ifndef _ICMP_H_
#define _ICMP_H_

#include <linux/types.h>

#include "tbuf.h"
#include "ip.h"

#define ICMP_ER 0      /* echo reply */
#define ICMP_DUR 3     /* destination unreachable */
#define ICMP_SQ 4      /* source quench */
#define ICMP_RD 5      /* redirect */
#define ICMP_ECHO 8    /* echo */
#define ICMP_TE 11     /* time exceeded */
#define ICMP_PP 12     /* parameter problem */
#define ICMP_TS 13     /* timestamp */
#define ICMP_TSR 14    /* timestamp reply */
#define ICMP_IRQ 15    /* information request */
#define ICMP_IR 16     /* information reply */

struct icmphdr {
    __u8 type;
    __u8 code;
    __u16 check;
    __u32 id;
    __u32 seqno;
}__attribute__((packed));

int icmp_out(struct tbuf *buf);
void icmp_in(struct tbuf *buf);
void icmp_init();
#endif
