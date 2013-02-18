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

static __u8 ip_buf[MTU_LEN];

int ip_in(struct tbuf *buf)
{
    struct iphdr *ip;
    __u16 checksum;
    ip = (struct iphdr *)buf->payload;
    checksum = checksum_generic((__u8 *)ip, ip->ihl * 4);

    //printf("protocol type: %d, checksum: %x\n", ip->protocol, checksum);
    // Discard bogus packets
    if (checksum) {
        printf("screwed packet, dropped\n");
        goto end;
    }

    tbuf_header(buf, IP_HLEN);
    if (l3protos[ip->protocol].type == ip->protocol) {
        l3protos[ip->protocol].handler(buf);
    }

end:
    tbuf_free(buf);
    return 0;
}

int ip_out(struct tbuf *buf, __u32 src, __u32 dst, __u8 ttl,
        __u8 protocol)
{
    struct sockaddr_in sin;
    struct iphdr *ip;
    // rewind to the head of payload 
    tbuf_header(buf, 0);
    ip = (struct iphdr *)buf->payload;
    ip->version = 4;
    ip->ttl = ttl; 
    ip->saddr = src;
    ip->daddr = dst;
    ip->protocol = protocol;
    ip->check = checksum_generic((__u8 *)ip, ip->ihl * 4);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = dst;
    lowlevel_send(buf, sin);

    tbuf_free(buf);
    return 0;
}

struct tbuf *lowlevel_recv()
{
    int size = 0;
    struct tbuf *buf;
    size = recv(l3_recv_sock, ip_buf, MTU_LEN, 0);    
    buf = tbuf_malloc(size);
    memcpy(buf->payload, ip_buf, size);
    return buf;
}

int lowlevel_send(struct tbuf *buf, struct sockaddr_in sin)
{
    int ret;
    ret = sendto(l3_send_sock, buf->payload, buf->len, 0, &sin, sizeof(sin));
    return ret;
}

static void *recv_thread(void *args)
{
    int ret;
    fd_set fds;
    struct tbuf *buf;

    FD_ZERO(&fds);
    FD_SET(l3_recv_sock, &fds);

    while(1) {
        ret = select(l3_recv_sock + 1, &fds, NULL, NULL, NULL);
        if (ret == 0) {
            continue;
        } else if (ret < 0) {
            break;
        }

        buf = lowlevel_recv();
        if (buf != NULL) {
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

    if ((l3_recv_sock = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0 ||
            (l3_send_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
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
