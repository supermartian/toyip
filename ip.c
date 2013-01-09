#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <net/ethernet.h>

#include "tbuf.h"
#include "icmp.h"
#include "ip.h"

int ip_in(struct tbuf *buf)
{
    struct iphdr *ip;
    ip = (struct iphdr *)buf->payload;

    printf("protocol type: %d\n", ip->protocol);

    tbuf_header(buf, IP_HLEN);
    if (l3protos[ip->protocol].type == ip->protocol) {
        l3protos[ip->protocol].handler(buf);
    }
    tbuf_free(buf);
    return 0;
}

int ip_out(struct tbuf *buf, __u32 src, __u32 dst, __u8 ttl,
        __u8 protocol)
{
    struct iphdr *ip = (struct iphdr *)buf->payload;
    ip->ttl = ttl;
    ip->saddr = htonl(src);
    ip->daddr = htonl(dst);
    ip->protocol = protocol;

    lowlevel_send(buf);
    return 0;
}

int lowlevel_recv(struct tbuf *buf)
{
    int size = 0;
    size = recv(l3_sock, buf->payload, MTU_LEN, 0);
    buf->len = size;
    return size;
}

int lowlevel_send(struct tbuf *buf)
{
    return send(l3_sock, buf->payload, buf->len, 0);
}

static void *recv_thread(void *args)
{
    int ret;
    fd_set fds;
    struct timeval tv; 
    struct tbuf *buf;

    FD_ZERO(&fds);
    FD_SET(l3_sock, &fds);

    while(1) {
        tv.tv_sec = 1;

        ret = select(l3_sock + 1, &fds, NULL, NULL, &tv);
        if (ret == 0) {
            continue;
        } else if (ret < 0) {
            break;
        }

        if ((buf = tbuf_malloc(MTU_LEN)) != NULL) {
            buf = tbuf_malloc(MTU_LEN);
            lowlevel_recv(buf);
            ip_in(buf);
        } else {
            // tbuf cannot be alloced
            continue;
        }
    }
}

int ip_init()
{
    void *thread_res;
    pthread_t rcv_thread;
    memset(l3protos, 0, PROTO_NUM);
    icmp_init();

    if ((l3_sock = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) {
        perror("L3 could not be initialized");
        exit(1);
    }

    pthread_create(&rcv_thread, NULL, recv_thread, NULL);
    pthread_join(rcv_thread, &thread_res);

    return 0;
}

int main()
{
    ip_init();
}
