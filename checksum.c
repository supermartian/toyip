#include <linux/types.h>

#include "ip.h"

// regard ip header as 20 octects always
__u16 fast_ip_checksum(struct iphdr *hdr)
{
    __u32 result = 0;
    __u16 *p = hdr;
    int i = 0;
    for (i = 0; i < 10; i++) {
        result += (__u32) *p; 
        p++;
    }
    result = (result >> 16) + (result & 0x0000ffffUL);
    result = result >> 16 ? result + 1 : result;
    result = htons(~result);

    return (__u16) result;
}

__u16 checksum_generic(__u8 *buf, __u16 len)
{
    int odd;
    __u16 l;
    __u32 result = 0;
    __u16 *p = buf;
    odd = len & 0x1;
    l = len - odd;

    while (l) {
        result += (__u32) *p; 
        p++;
        l -= 2;
    }

    // if odd, padding with zero at last 8 bits;
    if (odd) {
        result += ((*buf) << 8 & 0xffUL); 
    }

    result = (result >> 16) + (result & 0x0000ffffUL);
    result = result >> 16 ? result + 1 : result;
    result = htons(~result);

    return (__u16) result;
}
