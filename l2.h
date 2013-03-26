#ifndef _L2_H_
#define _L2_H_

#define FILTER_SIZE 128

int lowlevel_send(struct tbuf *buf, struct iface *dev);
int l2_init();
#endif
