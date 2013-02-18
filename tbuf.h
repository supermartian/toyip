#ifndef _TBUF_H
#define _TBUF_H

#include <linux/types.h>

#define TBUF_OK 0
#define TBUF_FAIL 1

#define ETH_HLEN 0
#define IP_HLEN 20
#define TRANS_HLEN 20

struct tbuf {
    // should have more
    __u16 len;
    __u8 *start;
    __u8 *payload;
};

int tbuf_free(struct tbuf *buf);
struct tbuf *tbuf_malloc(__u16 len);
int tbuf_header(struct tbuf *buf, __u16 offset);
int tbuf_copy(struct tbuf *dst, struct tbuf *src);
#endif
