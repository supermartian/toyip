#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_
#include <linux/types.h>
#include "ip.h"

__u16 checksum_generic(__u8 *buf, __u16 len);
__u16 fast_ip_checksum(struct iphdr *hdr);

#endif
