#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/filter.h>

#include "tbuf.h"
#include "icmp.h"
#include "ip.h"
#include "l2.h"
#include "checksum.h"
#include "iface.h"
#include "arp.h"

static __u8 ip_buf[MTU_LEN];
int l2_sock;

struct tbuf *lowlevel_recv()
{
    int size = 0;
    struct tbuf *buf;
    size = recv(l2_sock, ip_buf, MTU_LEN, 0);    
    buf = tbuf_malloc(size);
    memcpy(buf->payload, ip_buf, size);
    tbuf_header(buf, 0);
    ether_in(buf);
    return buf;
}

int lowlevel_send(struct tbuf *buf, struct iface *dev)
{
    int ret;
    struct sockaddr sa;

    memset(&sa, 0, sizeof(sa));
    memcpy(sa.sa_data, dev->name, strlen(dev->name));
    tbuf_header(buf, 0);
    ret = sendto(l2_sock, buf->payload, buf->len, 0, &sa, sizeof(sa));
    tbuf_free(buf);
    return ret;
}

static void *recv_thread(void *args)
{
    int ret;
    fd_set fds;
    struct tbuf *buf;

    FD_ZERO(&fds);
    FD_SET(l2_sock, &fds);

    while(1) {
        ret = select(l2_sock + 1, &fds, NULL, NULL, NULL);
        if (ret == 0) {
            continue;
        } else if (ret < 0) {
            break;
        }

        buf = lowlevel_recv();
        if (buf != NULL) {
            ether_in(buf);
            tbuf_free(buf);
        } else {
            // tbuf cannot be alloced
            continue;
        }
    }
}

int l2_init()
{
    void *thread_res;
    pthread_t rcv_thread;

    if ((l2_sock = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ALL))) < 0) {
        perror("L2 could not be initialized");
        exit(1);
    }

    struct sock_fprog fprog;
    struct sock_filter filter[FILTER_SIZE];
    memset(filter, 0x0, sizeof(filter));
    int ins = 0;
    int *end = malloc(sizeof(int));
    struct iface *dev;
    dev = GETFACE();

    // Filter out the outgoing packets using BPF
    while (dev != NULL) {
        __u32 *h32 = (__u32 *) (dev->hwaddr + 2);
        __u16 *l16 = (__u16 *) (dev->hwaddr);

        dump_mac(dev->hwaddr);
        filter[ins].code = BPF_LD | BPF_W | BPF_ABS;
        filter[ins].k = 2;
        ins++;
        filter[ins].code = BPF_JMP | BPF_JEQ | BPF_K;
        filter[ins].k = htonl(*h32);
        filter[ins].jt = 0;
        filter[ins].jf = 2;
        ins++;
        filter[ins].code = BPF_LD | BPF_H | BPF_ABS;
        filter[ins].k = 0x0;
        ins++;
        filter[ins].code = BPF_JMP | BPF_JEQ | BPF_K;
        filter[ins].k = htons(*l16);
        filter[ins].jt = FILTER_SIZE - ins - 2;
        filter[ins].jf = 1;
        ins++;
        dev = dev->next;
    }

    // Broadcast is needed
    filter[ins].code = BPF_LD | BPF_W | BPF_ABS;
    filter[ins].k = 2;
    ins++;
    filter[ins].code = BPF_JMP | BPF_JEQ | BPF_K;
    filter[ins].k = 0xffffffff;
    filter[ins].jt = 0;
    filter[ins].jf = 2;
    ins++;
    filter[ins].code = BPF_LD | BPF_H | BPF_ABS;
    filter[ins].k = 0x0;
    ins++;
    filter[ins].code = BPF_JMP | BPF_JEQ | BPF_K;
    filter[ins].k = 0xffff;
    filter[ins].jt = FILTER_SIZE - ins - 2;
    filter[ins].jf = 1;
    ins++;

    filter[ins].code = BPF_RET;
    filter[ins].k = 0;
    filter[FILTER_SIZE - 1].code = BPF_RET;
    filter[FILTER_SIZE - 1].k = 0x0000ffff;
    
    fprog.filter = filter;
    fprog.len = FILTER_SIZE; 

    setsockopt(l2_sock, SOL_SOCKET, SO_ATTACH_FILTER, &fprog, sizeof(fprog));
    perror("Initializing BPF");

    printf("L2 initialized\n");
    pthread_create(&rcv_thread, NULL, recv_thread, NULL);
    pthread_join(rcv_thread, &thread_res);

    return 0;
}
