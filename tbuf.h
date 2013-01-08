#ifndef _TBUF_H
#define _TBUF_H

#include <linux/types.h>
struct tbuf {
    __u16 len;
    __u8 *payload;
};

#endif
