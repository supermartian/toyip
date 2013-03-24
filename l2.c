#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>

#include "tbuf.h"
#include "icmp.h"
#include "ip.h"
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
    ether_in(buf);
    return buf;
}

int lowlevel_send(struct tbuf *buf, struct route *dstroute)
{
    int ret;
    struct sockaddr sa;

    memset(&sa, 0, sizeof(sa));
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

    printf("l2 initialized\n");
    pthread_create(&rcv_thread, NULL, recv_thread, NULL);
    pthread_join(rcv_thread, &thread_res);

    return 0;
}
