/**
 * @file
 * Lwip platform independent driver interface.
 * This set of driver interface shields the netif details, 
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 * 
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */
 
#ifndef __NETDEV_H
#define __NETDEV_H

#include "net/if.h"
#include "lwip/pbuf.h"
#include "lwip/sockets.h"
#include "sys/param.h"

struct netdev;

/*
 * network driver functions.
 */
struct netdev_funcs {
  /* initialize function */
  int  (*init)(struct netdev *netdev);

  /* basice functions can initialize with NULL */
  int  (*up)(struct netdev *netdev);
  int  (*down)(struct netdev *netdev);
  int  (*remove)(struct netdev *netdev);
  
  /* netdev ioctl 
   * cmd: SIOCSIFMTU:    arg is struct ifreq *pifreq, set MTU      (pifreq->ifr_mtu)
   *      SIOCSIFFLAGS:  arg is struct ifreq *pifreq, set PROMISC  (pifreq->ifr_flags & IFF_PROMISC)
   *      SIOCSIFHWADDR: arg is struct ifreq *pifreq, set hwaddr   (pifreq->ifr_hwaddr[]) */
  int  (*ioctl)(struct netdev *netdev, int cmd, void *arg);

  /* netdev add or del mac filter */
#define NETDRV_MACFILTER_DEL  0
#define NETDRV_MACFILTER_ADD  1
  int  (*macfilter)(struct netdev *netdev, int op, struct sockaddr *addr);
  
  /* netdev transmit a packet, and if success return 0 or return -1. */
  int  (*transmit)(struct netdev *netdev, struct pbuf *p);
  
  /* netdev receive a packet, system will call this function receive a packet. */
  void (*receive)(struct netdev *netdev, int (*input)(struct netdev *, struct pbuf *));
  
  /* reserve for futrue */
  void *reserved[8];
};

/*
 * network device struct.
 */
typedef struct netdev {
#define NETDEV_MAGIC    0xf7e34a81
  UINT32 magic_no;  /* MUST be NETDEV_MAGIC */

  char  dev_name[IF_NAMESIZE];  /* user network device name (such as igb* rtl* also call initialize with '\0') */
  char  if_name[IF_NAMESIZE];   /* add to system netif name (such as 'en') */
  char *if_hostname;
  
#define NETDEV_INIT_LOAD_PARAM     0x01   /* load netif parameter when add to system */
#define NETDEV_INIT_LOAD_DNS       0x02   /* load dns parameter when add to system */
#define NETDEV_INIT_IPV6_AUTOCFG   0x04
#define NETDEV_INIT_AS_DEFAULT     0x08
  UINT32 init_flags;
  
#define NETDEV_CHKSUM_GEN_IP       0x0001   /* tcp/ip stack will generate checksum IP, UDP, TCP, ICMP, ICMP6 */
#define NETDEV_CHKSUM_GEN_UDP      0x0002
#define NETDEV_CHKSUM_GEN_TCP      0x0004
#define NETDEV_CHKSUM_GEN_ICMP     0x0008
#define NETDEV_CHKSUM_GEN_ICMP6    0x0010
#define NETDEV_CHKSUM_CHECK_IP     0x0100   /* tcp/ip stack will check checksum IP, UDP, TCP, ICMP, ICMP6 */
#define NETDEV_CHKSUM_CHECK_UDP    0x0200
#define NETDEV_CHKSUM_CHECK_TCP    0x0400
#define NETDEV_CHKSUM_CHECK_ICMP   0x0800
#define NETDEV_CHKSUM_CHECK_ICMP6  0x1000
#define NETDEV_CHKSUM_ENABLE_ALL   0xffff   /* tcp/ip stack will gen/check all chksum */
#define NETDEV_CHKSUM_DISABLE_ALL  0x0000   /* tcp/ip stack will not gen/check all chksum */
  UINT32 chksum_flags;

#define NETDEV_TYPE_RAW         0
#define NETDEV_TYPE_ETHERNET    1
  UINT32 net_type;
  
  UINT64 speed; /* link layer speed bps */
  UINT32 mtu;   /* link layer max packet length */
  
  UINT8 hwaddr_len;                     /* link layer address length MUST 6 or 8 */
  UINT8 hwaddr[NETIF_MAX_HWADDR_LEN];   /* link layer address */
  
  struct netdev_funcs *drv; /* netdev driver */
  
  void *priv;   /* user network device private data */
  
  /* the following member is used by system, driver not used! */
  int if_flags;
  ULONG sys[256];  /* reserve for netif */
} netdev_t;

/* netdev driver call the following functions add / delete a network interface,
 * if this device not in '/etc/if_param.ini' ip, netmask, gw is default configuration.
 * 'if_flags' defined in net/if.h such as IFF_UP, IFF_BROADCAST, IFF_RUNNING, IFF_NOARP, IFF_MULTICAST, IFF_PROMISC ... */
int  netdev_add(netdev_t *netdev, const char *ip, const char *netmask, const char *gw, int if_flags);
int  netdev_delete(netdev_t *netdev);
int  netdev_index(netdev_t *netdev, unsigned int *index);

/* if netdev link status changed has been detected, 
 * driver must call the following functions 
 * linkup 1: linked up  0:not link */
int  netdev_set_linkup(netdev_t *netdev, int linkup, UINT64 speed);
int  netdev_get_linkup(netdev_t *netdev, int *linkup);

/* netdev linkup poll function 
 * NOTICE: one netdev can ONLY add one linkup_poll function.
 *         when netdev removed system will delete the linkup poll automatically. */
int  netdev_linkup_poll_add(netdev_t *netdev, void  (*linkup_poll)(netdev_t *));
int  netdev_linkup_poll_delete(netdev_t *netdev, void  (*linkup_poll)(netdev_t *));

typedef enum {
  LINK_INPUT = 0,   /* packet input */
  LINK_OUTPUT       /* packet output */
} netdev_inout;

/* if netdev detected a packet in netdev buffer, driver can call this function to receive this packet.
   qen:0 do not use netjob queue 1:use netjob queue */
int  netdev_notify(struct netdev *netdev, netdev_inout inout, int q_en);
int  netdev_notify_ex(struct netdev *netdev, netdev_inout inout, int q_en, unsigned int qindex);
int  netdev_notify_clear(struct netdev *netdev);
int  netdev_notify_clear_ex(struct netdev *netdev, unsigned int qindex);

/* netdev statistical information update functions */
void netdev_statinfo_total_add(netdev_t *netdev, netdev_inout inout, UINT32 bytes);
void netdev_statinfo_ucasts_inc(netdev_t *netdev, netdev_inout inout);
void netdev_statinfo_mcasts_inc(netdev_t *netdev, netdev_inout inout);
void netdev_statinfo_discards_inc(netdev_t *netdev, netdev_inout inout);
void netdev_statinfo_errors_inc(netdev_t *netdev, netdev_inout inout);

/* netdev link statistical information update functions */
void netdev_linkinfo_err_inc(netdev_t *netdev);
void netdev_linkinfo_lenerr_inc(netdev_t *netdev);
void netdev_linkinfo_chkerr_inc(netdev_t *netdev);
void netdev_linkinfo_memerr_inc(netdev_t *netdev);
void netdev_linkinfo_drop_inc(netdev_t *netdev);
void netdev_linkinfo_recv_inc(netdev_t *netdev);
void netdev_linkinfo_xmit_inc(netdev_t *netdev);

/* netdev input buffer get 
 * reserve ETH_PAD_SIZE + SIZEOF_VLAN_HDR size. */
struct pbuf *netdev_pbuf_alloc(UINT16 len);
void netdev_pbuf_free(struct pbuf *p);
UINT8 *netdev_pbuf_push(struct pbuf *p, UINT16 len);
UINT8 *netdev_pbuf_pull(struct pbuf *p, UINT16 len);

#endif /* __NETDEV_H */
