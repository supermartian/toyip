#ifndef _IFACE_H_
#define _IFACE_H_

#define HWADDR_LEN 6
struct iface {
    struct iface *next;
    __u16 id;
    __u8 hwaddr[HWADDR_LEN];
    __u8 broadcast[HWADDR_LEN];
    __u32 mtu;
    __u32 ipaddr;
    __u32 netmask;
    __u32 gateway;
    char name[10];
};

struct iface* face_list;
#define GETFACE() \
    face_list->next

struct iface* get_iface_by_name(char *name);
struct iface* get_iface_by_id(__u16 id);
struct iface* get_iface_by_ip(__u32 ip);
struct iface* get_iface_by_mac(__u8 *mac);
void iface_init();
#endif
