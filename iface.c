#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "iface.h"
#include "arp.h"
#include "tbuf.h"

#define IFLIST_REPLY_BUFFER 8192

struct nl_req_s {
    struct nlmsghdr hdr;
    struct rtgenmsg gen;
};

static void dump_faces();
static int get_faces();
static void add_iface(struct nlmsghdr *h);

static void dump_faces()
{
    struct iface *face;
    char ip[20];
    face = face_list->next;
    while (face != NULL) {
        ip_to_string(ip, face->ipaddr);
        printf("Interface %d: hwaddr %x:%x:%x:%x:%x:%x ipaddr: %s mtu: %d name: %s\n",
                face->id,
                face->hwaddr[0], face->hwaddr[1], face->hwaddr[2], face->hwaddr[3], face->hwaddr[4], face->hwaddr[5],
                ip,
                face->mtu, face->name);
        face = face->next;
    }
}

static int get_faces()
{
    int fd;
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    int end = 0;
    struct sockaddr_nl kernel;
    struct msghdr rtnl_msg;
    struct iovec io;
    struct nl_req_s req;
    memset(&rtnl_msg, 0, sizeof(rtnl_msg));
    memset(&kernel, 0, sizeof(kernel));
    memset(&req, 0, sizeof(req));
    kernel.nl_family = AF_NETLINK; /* fill-in kernel address (destination) */

    req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
    req.hdr.nlmsg_type = RTM_GETLINK;
    req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    req.hdr.nlmsg_seq = 1;
    req.hdr.nlmsg_pid = getpid();
    req.gen.rtgen_family = AF_PACKET; /*  no preferred AF, we will get *all* interfaces */

    io.iov_base = &req;
    io.iov_len = req.hdr.nlmsg_len;
    rtnl_msg.msg_iov = &io;
    rtnl_msg.msg_iovlen = 1;
    rtnl_msg.msg_name = &kernel;
    rtnl_msg.msg_namelen = sizeof(kernel);

    struct sockaddr_nl local;
    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;
    local.nl_pid = getpid(); /* can be obtained using getpid*/
    local.nl_groups = 0;

    if (bind(fd, (struct sockaddr *) &local, sizeof(local)) < 0) {
        perror("crap...");
    }

    sendmsg(fd, (struct msghdr *) &rtnl_msg, 0);

    char reply[IFLIST_REPLY_BUFFER]; /* a large buffer */

    while (!end) {
        int len;
        struct nlmsghdr *msg_ptr;    /* pointer to current part */
        struct msghdr rtnl_reply;    /* generic msghdr structure */
        struct iovec io_reply;
        memset(&io_reply, 0, sizeof(io_reply));
        memset(&rtnl_reply, 0, sizeof(rtnl_reply));
 
        io.iov_base = reply;
        io.iov_len = IFLIST_REPLY_BUFFER;
        rtnl_reply.msg_iov = &io;
        rtnl_reply.msg_iovlen = 1;
        rtnl_reply.msg_name = &kernel;
        rtnl_reply.msg_namelen = sizeof(kernel);
 
        len = recvmsg(fd, &rtnl_reply, 0); /* read lots of data */
 
        for (msg_ptr = (struct nlmsghdr *) reply; NLMSG_OK(msg_ptr, len); msg_ptr = NLMSG_NEXT(msg_ptr, len))
        {
            if (msg_ptr->nlmsg_type == RTM_NEWLINK) {
                add_iface(msg_ptr);
            } else if (msg_ptr->nlmsg_type == NLMSG_DONE) {
                end = 1;
                break;
            }
        }
    }

    return 0;
}

static void add_iface(struct nlmsghdr *h)
{
    struct ifinfomsg *iface_msg;
    struct iface *face = malloc(sizeof(struct iface));
    struct rtattr *attribute;
    int len;

    iface_msg = NLMSG_DATA(h);
    len = h->nlmsg_len - NLMSG_LENGTH(sizeof(*iface_msg));
    face->id = iface_msg->ifi_index;

    for (attribute = IFLA_RTA(iface_msg); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
    {
         switch(attribute->rta_type)
         {
              case IFLA_IFNAME:
                  strcpy(face->name, (char *) RTA_DATA(attribute));
                  printf("face: %s\n", face->name);

                  // loopback is no needed
                  if (!strcmp(face->name, "lo")) {
                      free(face);
                      return;
                  }
              break;
              case IFLA_ADDRESS:
                  memcpy(face->hwaddr, (__u8 *) RTA_DATA(attribute), HWADDR_LEN);
              break;
              case IFLA_BROADCAST:
                  memcpy(face->broadcast, (__u8 *) RTA_DATA(attribute), HWADDR_LEN);
              break;
              case IFLA_MTU:
                  memcpy(&(face->mtu), RTA_DATA(attribute), sizeof(__u32));
              break;
              default:
              break;
         }
    }

    face->next = face_list->next;
    face_list->next = face;
}

struct iface* get_iface_by_name(char *name)
{
    struct iface *face = NULL;
    for (face = face_list->next; face != NULL; face = face->next) {
        if (!strcmp(name, face->name)) {
            return face;
        }
    }

    return NULL;
}

struct iface* get_iface_by_id(__u16 id)
{
    struct iface *face;
    for (face = face_list->next; face != NULL; face = face->next) {
        if (id == face->id) {
            return face;
        }
    }

    return NULL;
}

struct iface* get_iface_by_ip(__u32 ip)
{
    struct iface *face;
    for (face = face_list->next; face != NULL; face = face->next) {
        if (ip == face->ipaddr) {
            return face;
        }
    }

    return NULL;
}

struct iface* get_iface_by_mac(__u8 *mac)
{
    struct iface *face;
    for (face = face_list->next; face != NULL; face = face->next) {
        if (!memcmp(face->hwaddr, mac, sizeof(__u8) * HWADDR_LEN)) {
            return face;
        }
    }

    return NULL;
}


void config_iface(struct iface *dev, __u32 mtu, __u32 ipaddr, __u32 netmask, __u32 gateway)
{
    struct tbuf *arpbuf;

    dev->mtu = mtu;
    dev->ipaddr = ipaddr;
    dev->netmask = netmask;
    dev->gateway = gateway;
    printf("Configurating %s...\n", dev->name);
    dump_faces();

    arpbuf = build_arp(htonl(dev->ipaddr), 
            dev->hwaddr, 
            htonl(dev->ipaddr), 
            NULL, 
            ARP_REQUEST);
    lowlevel_send(arpbuf, dev);
}

void iface_init()
{
    face_list = malloc(sizeof(struct iface));
    face_list->next = NULL;

    get_faces();
    dump_faces();
}
