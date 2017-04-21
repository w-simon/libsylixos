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

#define  __SYLIXOS_KERNEL
#include "lwip/pbuf.h"
#include "lwip/inet.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "net/if_param.h"
#include "net/if_ether.h"

#include "string.h"
#include "netdev.h"

#define NETDEV_INIT(netdev)             if ((netdev)->drv->init) { (netdev)->drv->init((netdev)); }
#define NETDEV_UP(netdev)               if ((netdev)->drv->up) { (netdev)->drv->up((netdev)); }
#define NETDEV_DOWN(netdev)             if ((netdev)->drv->down) { (netdev)->drv->down((netdev)); }
#define NETDEV_REMOVE(netdev)           if ((netdev)->drv->remove) { (netdev)->drv->remove((netdev)); }
#define NETDEV_IOCTL(netdev, a, b)      if ((netdev)->drv->ioctl) { (netdev)->drv->ioctl((netdev), (a), (b)); }
#define NETDEV_PROMISC(netdev, a, b)    if ((netdev)->drv->promisc) { (netdev)->drv->promisc((netdev), (a), (b)); }
#define NETDEV_MACFILTER(netdev, a, b)  if ((netdev)->drv->macfilter) { (netdev)->drv->macfilter((netdev), (a), (b)); }
#define NETDEV_TRANSMIT(netdev, a)      (netdev)->drv->transmit((netdev), a)
#define NETDEV_RECEIVE(netdev, input)   (netdev)->drv->receive((netdev), input)

/* lwip netif up hook function */
static void  netdev_netif_up (struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  
  NETDEV_UP(netdev);
  netdev->if_flags |= IFF_UP;
}

/* lwip netif down hook function */
static void  netdev_netif_down (struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  
  NETDEV_DOWN(netdev);
  netdev->if_flags &= ~IFF_UP;
}

/* lwip netif remove hook function */
static void  netdev_netif_remove (struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  
  NETDEV_REMOVE(netdev);
}

/* lwip netif ioctl hook function */
static int  netdev_netif_ioctl (struct netif *netif, int cmd, void *arg)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  
  if (netdev->drv->ioctl) {
    return (netdev->drv->ioctl(netdev, cmd, arg));
  }
  
  return (-1);
}

/* lwip netif igmp mac filter hook function */
static err_t  netdev_netif_igmp_mac_filter (struct netif *netif,
                                            const ip4_addr_t *group, 
                                            enum netif_mac_filter_action action)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  struct sockaddr_in inaddr;
  
  inaddr.sin_len    = sizeof(struct sockaddr_in);
  inaddr.sin_family = AF_INET;
  inet_addr_from_ip4addr(&inaddr.sin_addr, group);
  
  if (action == NETIF_DEL_MAC_FILTER) {
    NETDEV_MACFILTER(netdev, NETDRV_MACFILTER_DEL, (struct sockaddr *)&inaddr);
    
  } else {
    NETDEV_MACFILTER(netdev, NETDRV_MACFILTER_ADD, (struct sockaddr *)&inaddr);
  }
  
  return (0);
}

/* lwip netif mld mac filter hook function */
static err_t  netdev_netif_mld_mac_filter (struct netif *netif,
                                           const ip6_addr_t *group, 
                                           enum netif_mac_filter_action action)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  struct sockaddr_in6 in6addr;
  
  in6addr.sin6_len    = sizeof(struct sockaddr_in6);
  in6addr.sin6_family = AF_INET6;
  inet6_addr_from_ip6addr(&in6addr.sin6_addr, group);
  
  if (action == NETIF_DEL_MAC_FILTER) {
    NETDEV_MACFILTER(netdev, NETDRV_MACFILTER_DEL, (struct sockaddr *)&in6addr);
    
  } else {
    NETDEV_MACFILTER(netdev, NETDRV_MACFILTER_ADD, (struct sockaddr *)&in6addr);
  }
  
  return (0);
}

/* lwip netif rawoutput hook function */
static err_t  netdev_netif_rawoutput4 (struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  int ret;

  ret = NETDEV_TRANSMIT(netdev, p);
  if (ret < 0) {
    return (ERR_IF);
  }
  
  return (ERR_OK);
}

static err_t  netdev_netif_rawoutput6 (struct netif *netif, struct pbuf *p, const ip6_addr_t *ip6addr)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  int ret;

  ret = NETDEV_TRANSMIT(netdev, p);
  if (ret < 0) {
    return (ERR_IF);
  }
  
  return (ERR_OK);
}

/* lwip netif linkoutput hook function */
static err_t  netdev_netif_linkoutput (struct netif *netif, struct pbuf *p)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  int ret;

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE);
#endif

  ret = NETDEV_TRANSMIT(netdev, p);
  
#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE);
#endif

  if (ret < 0) {
    return (ERR_IF);
  }
  
  return (ERR_OK);
}

/* lwip netif linkinput hook function */
static int  netdev_netif_linkinput (netdev_t *netdev, struct pbuf *p)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE);
#endif

  if (netif->input(p, netif)) {
    return (-1);
    
  } else {
    return (0);
  }
}

/* lwip netif add call back function */
static err_t  netdev_netif_init (struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state);

#if LWIP_NETIF_HOSTNAME
  netif->hostname = netdev->if_hostname;
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = netdev->if_name[0];
  netif->name[1] = netdev->if_name[1];

  switch (netdev->net_type) {
  
  case NETDEV_TYPE_ETHERNET:
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, (u32_t)netdev->speed);
    netif->flags = NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    break;
  
  default:
    MIB2_INIT_NETIF(netif, snmp_ifType_other, (u32_t)netdev->speed);
    netif->flags = 0;
    netif->output = netdev_netif_rawoutput4;
#if LWIP_IPV6
  netif->output_ip6 = netdev_netif_rawoutput6;
#endif /* LWIP_IPV6 */
    break;
  }
  
  netif->linkoutput = netdev_netif_linkoutput;

  netif->mtu = (u16_t)netdev->mtu;
  
  netif->chksum_flags = (u16_t)netdev->chksum_flags;
  
  if (netdev->init_flags & NETDEV_INIT_IPV6_AUTOCFG) {
    netif->ip6_autoconfig_enabled = 1;
  }
  
  if (netdev->if_flags & IFF_UP) {
    netif->flags |= NETIF_FLAG_UP;
  }
  
  if (netdev->if_flags & IFF_BROADCAST) {
    netif->flags |= NETIF_FLAG_BROADCAST;
  }
  
  if (netdev->if_flags & IFF_POINTOPOINT) {
    netif->flags &= ~NETIF_FLAG_BROADCAST;
  }
  
  if (netdev->if_flags & IFF_RUNNING) {
    netif->flags |= NETIF_FLAG_LINK_UP;
  }
  
  if (netdev->if_flags & IFF_MULTICAST) {
    netif->flags |= NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
#if LWIP_IPV4 && LWIP_IGMP
    netif->igmp_mac_filter = netdev_netif_igmp_mac_filter;
#endif /* LWIP_IPV4 && LWIP_IGMP */
#if LWIP_IPV6 && LWIP_IPV6_MLD
    netif->mld_mac_filter = netdev_netif_mld_mac_filter;
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  
  if (netdev->if_flags & IFF_NOARP) {
    netif->flags &= ~NETIF_FLAG_ETHARP;
  }
  
#if LWIP_NETIF_REMOVE_CALLBACK
  netif->remove_callback = netdev_netif_remove;
#endif /* LWIP_NETIF_REMOVE_CALLBACK */
  
  netif->up = netdev_netif_up;
  netif->down = netdev_netif_down;
  netif->ioctl = netdev_netif_ioctl;
  
  if (netdev->if_flags & IFF_PROMISC) {
    netif->flags2 = NETIF_FLAG2_PROMISC;
  } else {
    netif->flags2 = 0;
  }
  
  if (netdev->drv->init) { 
    if (netdev->drv->init(netdev) < 0) {
      return (ERR_IF);
    } 
  }
  
  /* Update netif hwaddr */
  netif->hwaddr_len = (u8_t)((netdev->hwaddr_len < NETIF_MAX_HWADDR_LEN)
                    ? netdev->hwaddr_len
                    : NETIF_MAX_HWADDR_LEN);

  SMEMCPY(netif->hwaddr, netdev->hwaddr, netif->hwaddr_len);
  
  if (netdev->if_flags & IFF_UP) {
    NETDEV_UP(netdev);
  }
  
#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  return (ERR_OK);
}

/* netdev driver call the following functions add a network interface,
 * if this device not in '/etc/if_param.ini' ip, netmask, gw is default configuration.
 * 'if_flags' defined in net/if.h such as IFF_UP, IFF_BROADCAST, IFF_RUNNING, IFF_NOARP, IFF_MULTICAST, IFF_PROMISC ... */
int  netdev_add (netdev_t *netdev, const char *ip, const char *netmask, const char *gw, int if_flags)
{
  ip4_addr_t ip4, netmask4, gw4;
  struct netif *netif;
  struct netdev_funcs *drv;
  void  *ifparam;
  int  i, enable, def;
  char macstr[32];
  int  mac[NETIF_MAX_HWADDR_LEN];

  if (!netdev || (netdev->magic_no != NETDEV_MAGIC) || !netdev->drv) {
    return (-1);
  }
  
  if ((netdev->if_name[0] == '\0') || (netdev->if_name[1] == '\0')) {
    return (-1);
  }
  
  if ((netdev->hwaddr_len != 6) && (netdev->hwaddr_len != 8)) {
    return (-1);
  }
  
  drv = netdev->drv;
  if (!drv->transmit || !drv->receive) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  lib_bzero(netif, sizeof(struct netif));
  
  if (ip) {
    ip4.addr = inet_addr(ip);
  } else {
    ip4.addr = IPADDR_ANY;
  }
  
  if (netmask) {
    netmask4.addr = inet_addr(netmask);
  } else {
    netmask4.addr = IPADDR_ANY;
  }
  
  if (gw) {
    gw4.addr = inet_addr(gw);
  } else {
    gw4.addr = IPADDR_ANY;
  }
  
  netdev->if_flags = if_flags;
  
  if (netdev->init_flags & NETDEV_INIT_LOAD_PARAM) {
    ifparam = if_param_load(netdev->dev_name);
    if (ifparam) {
      if_param_getenable(ifparam, &enable);
      if (enable) {
        netdev->if_flags |= IFF_UP;
      } else {
        netdev->if_flags &= ~IFF_UP;
      }
    
      if_param_getdefault(ifparam, &def);
      if (def) {
        netdev->init_flags |= NETDEV_INIT_AS_DEFAULT;
      } else {
        netdev->init_flags &= ~NETDEV_INIT_AS_DEFAULT;
      }
      
      if_param_getipaddr(ifparam, &ip4);
      if_param_getnetmask(ifparam, &netmask4);
      if_param_getgw(ifparam, &gw4);

      if (!if_param_getmac(ifparam, macstr, sizeof(macstr))) {
        if (netdev->hwaddr_len == 6) {
          if (sscanf(macstr, "%x:%x:%x:%x:%x:%x", 
                     &mac[0], &mac[1], &mac[2], 
                     &mac[3], &mac[4], &mac[5]) == 6) {
            for (i = 0; i < 6; i++) {
              netdev->hwaddr[i] = (UINT8)mac[i];
            }
          }
        } else {
          if (sscanf(macstr, "%x:%x:%x:%x:%x:%x:%x:%x", 
                     &mac[0], &mac[1], &mac[2], &mac[3], 
                     &mac[4], &mac[5], &mac[6], &mac[7]) == 8) {
            for (i = 0; i < 8; i++) {
              netdev->hwaddr[i] = (UINT8)mac[i];
            }
          }
        }
      }
      
      if_param_unload(ifparam);
    }
  }
  
  if (netdev->init_flags & NETDEV_INIT_LOAD_DNS) {
    if_param_syncdns();
  }
  
  if (netifapi_netif_add(netif, &ip4, &netmask4, &gw4, netdev, netdev_netif_init, tcpip_input)) {
    return (-1);
  }
  
  netif_create_ip6_linklocal_address(netif, 1);
  
  if (netdev->init_flags & NETDEV_INIT_AS_DEFAULT) {
    netifapi_netif_set_default(netif);
  }
  
  return (0);
}

/* netdev driver call the following functions delete a network interface */
int  netdev_delete (netdev_t *netdev)
{
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  
  netifapi_netif_remove(netif);
  
  return (0);
}

/* netdev driver get netdev index */
int  netdev_index (netdev_t *netdev, unsigned int *index)
{
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  
  if (index) {
    *index = netif->num;
    return (0);
    
  } else {
    return (-1);
  }
}

/* netdev find (MUST in NETIF_LOCK mode) */
netdev_t *netdev_find_by_ifname (const char *if_name)
{
  struct netif *netif;
  netdev_t     *netdev;
  
  if (!if_name) {
    return (NULL);
  }
  
  netif = netif_find(if_name);
  if (netif) {
    netdev = (netdev_t *)(netif->state);
    if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
      return (netdev);
    }
  }
  
  return (NULL);
}

netdev_t *netdev_find_by_devname (const char *dev_name)
{
  struct netif *netif;
  netdev_t     *netdev;

  if (!dev_name) {
    return (NULL);
  }
  
  for (netif = netif_list; netif != NULL; netif = netif->next) {
    netdev = (netdev_t *)(netif->state);
    if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
      if (lib_strcmp(netdev->dev_name, dev_name) == 0) {
        return (netdev);
      }
    }
  }
  
  return (NULL);
}

/* if netdev link status changed has been detected, 
 * driver must call the following functions */
int  netdev_set_linkup (netdev_t *netdev, int linkup, UINT64 speed)
{
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  
  if (linkup) {
    netif->ts = sys_jiffies();
    netdev->speed = speed;
    
    netifapi_netif_set_link_up(netif);
    netdev->if_flags |= IFF_RUNNING;

    if (speed > 0xffffffff) {
      netif->link_speed = 0;
    } else {
      netif->link_speed = (u32_t)speed;
    }
  
  } else {
    netifapi_netif_set_link_down(netif);
    netdev->if_flags &= ~IFF_RUNNING;
  }
  
  return (0);
}

int  netdev_get_linkup (netdev_t *netdev, int *linkup)
{
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  
  if (linkup) {
    if (netif_is_link_up(netif)) {
      *linkup = 1;
    } else {
      *linkup = 0;
    }
  }
  
  return (0);
}

/* netdev linkup poll function 
 * NOTICE: one netdev can ONLY add one linkup_poll function.
 *         when netdev removed driver must delete poll function manually. */
int  netdev_linkup_poll_add (netdev_t *netdev, void  (*linkup_poll)(netdev_t *))
{
#if LW_CFG_HOTPLUG_EN > 0
  if (netdev && linkup_poll) {
    return (hotplugPollAdd(linkup_poll, netdev));
  }
#endif /* LW_CFG_HOTPLUG_EN */
  return (-1);
}

int  netdev_linkup_poll_delete (netdev_t *netdev, void  (*linkup_poll)(netdev_t *))
{
#if LW_CFG_HOTPLUG_EN > 0
  if (netdev && linkup_poll) {
    return (hotplugPollDelete(linkup_poll, netdev));
  }
#endif /* LW_CFG_HOTPLUG_EN */
  return (-1);
}

/* if netdev detected a packet in netdev buffer, driver can call this function to receive this packet.
   notify:0 can transmit 1: can receive 
   qen:0 do not use netjob queue 1:use netjob queue */
int  netdev_notify (struct netdev *netdev, netdev_inout inout, int q_en)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  if (inout != LINK_INPUT) {
    return (0);
  }
  
  if (q_en) {
    if (netJobAdd(netdev->drv->receive, netdev, 
                  (void *)netdev_netif_linkinput, 0, 0, 0, 0) == 0) {
      return (0);
    
    } else {
      return (-1);
    }
  }
  
  NETDEV_RECEIVE(netdev, netdev_netif_linkinput);
  
  return (0);
}

int  netdev_notify_ex (struct netdev *netdev, netdev_inout inout, int q_en, unsigned int qindex)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  if (inout != LINK_INPUT) {
    return (0);
  }
  
  if (q_en) {
    if (netJobAddEx(qindex, netdev->drv->receive, netdev, 
                    (void *)netdev_netif_linkinput, 0, 0, 0, 0) == 0) {
      return (0);
    
    } else {
      return (-1);
    }
  }
  
  NETDEV_RECEIVE(netdev, netdev_netif_linkinput);
  
  return (0);
}

int  netdev_notify_clear (struct netdev *netdev)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netJobDelete(2, netdev->drv->receive, netdev, 
               (void *)netdev_netif_linkinput, 0, 0, 0, 0);
               
  return (0);
}

int  netdev_notify_clear_ex (struct netdev *netdev, unsigned int qindex)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netJobDeleteEx(qindex, 2, netdev->drv->receive, netdev, 
                 (void *)netdev_netif_linkinput, 0, 0, 0, 0);
               
  return (0);
}

/* netdev statistical information update functions in:1 input 0:output */
void netdev_statinfo_total_add (netdev_t *netdev, netdev_inout inout, UINT32 bytes)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_add_ifinoctets(netif, bytes);
  
  } else {
    snmp_add_ifoutoctets(netif, bytes);
  }
}

void netdev_statinfo_ucasts_inc (netdev_t *netdev, netdev_inout inout)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_inc_ifinucastpkts(netif);
  
  } else {
    snmp_inc_ifoutucastpkts(netif);
  }
}

void netdev_statinfo_mcasts_inc (netdev_t *netdev, netdev_inout inout)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_inc_ifinnucastpkts(netif);
  
  } else {
    snmp_inc_ifoutnucastpkts(netif);
  }
}

void netdev_statinfo_discards_inc (netdev_t *netdev, netdev_inout inout)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_inc_ifindiscards(netif);
  
  } else {
    snmp_inc_ifoutdiscards(netif);
  }
}

void netdev_statinfo_errors_inc (netdev_t *netdev, netdev_inout inout)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_inc_ifinerrors(netif);
  
  } else {
    snmp_inc_ifouterrors(netif);
  }
}

/* netdev link statistical information update functions */
void netdev_linkinfo_err_inc (netdev_t *netdev)
{
  LINK_STATS_INC(link.err);
}

void netdev_linkinfo_lenerr_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.lenerr);
}

void netdev_linkinfo_chkerr_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.chkerr);
}

void netdev_linkinfo_memerr_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.memerr);
}

void netdev_linkinfo_drop_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.drop);
}

void netdev_linkinfo_recv_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.recv);
}

void netdev_linkinfo_xmit_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.xmit);
}

/* netdev input buffer get 
 * reserve: ETH_PAD_SIZE + SIZEOF_VLAN_HDR size. */
struct pbuf *netdev_pbuf_alloc (UINT16 len)
{
  u16_t reserve = ETH_PAD_SIZE + SIZEOF_VLAN_HDR;
  struct pbuf *p = pbuf_alloc(PBUF_RAW, (u16_t)(len + reserve), PBUF_POOL);
  
  if (p) {
    pbuf_header(p, (u16_t)-reserve);
  }
  
  return (p);
}

void netdev_pbuf_free (struct pbuf *p)
{
  pbuf_free(p);
}

/* netdev input buffer push */
UINT8 *netdev_pbuf_push (struct pbuf *p, UINT16 len)
{
  if (p) {
    if (pbuf_header(p, (s16_t)len) == 0) {
      return ((UINT8 *)p->payload);
    }
  }
  
  return (NULL);
}

/* netdev input buffer pop */
UINT8 *netdev_pbuf_pull (struct pbuf *p, UINT16 len)
{
  if (p) {
    if (pbuf_header(p, (s16_t)-len) == 0) {
      return ((UINT8 *)p->payload);
    }
  }
  
  return (NULL);
}

/* netdev buffer get vlan info */
int netdev_pbuf_vlan_present (struct pbuf *p)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);
  
  return (ethhdr->type == PP_HTONS(ETHTYPE_VLAN));
}

int netdev_pbuf_vlan_id (struct pbuf *p, UINT16 *vlanid)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);

  if ((ethhdr->type == PP_HTONS(ETHTYPE_VLAN)) && (p->len >= ETH_HLEN + 4)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);
    if (vlanid) {
      *vlanid = vlan->prio_vid;
    }
    return (0);
  }
  
  return (-1);
}

int netdev_pbuf_vlan_proto (struct pbuf *p, UINT16 *vlanproto)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);

  if ((ethhdr->type == PP_HTONS(ETHTYPE_VLAN)) && (p->len >= ETH_HLEN + 4)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);
    if (vlanproto) {
      *vlanproto = vlan->tpid;
    }
    return (0);
  }
  
  return (-1);
}

#if LW_CFG_NET_DEV_PROTO_ANALYSIS > 0

/* netdev buffer get ethernet & vlan header */
struct eth_hdr *netdev_pbuf_ethhdr (struct pbuf *p, int *hdrlen)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);
  
  if (hdrlen) {
    *hdrlen = ETH_HLEN;
  }
  
  return (ethhdr);
}

struct eth_vlan_hdr *netdev_pbuf_vlanhdr (struct pbuf *p, int *hdrlen)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);

  if (ethhdr->type == PP_HTONS(ETHTYPE_VLAN) && (p->len >= ETH_HLEN + 4)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);
    if (hdrlen) {
      *hdrlen = 4;
    }
    return (vlan);
  }
  
  return (NULL);
}

/* netdev buffer get proto header */
struct ip_hdr *netdev_pbuf_iphdr (struct pbuf *p, int offset, int *hdrlen)
{
  struct pbuf *q;
  u16_t out_offset;

  q = pbuf_skip(p, (u16_t)offset, &out_offset);
  if (!q) {
    return (NULL);
  }
  
  if (q->len >= (u16_t)out_offset + IP_HLEN) {
    struct ip_hdr *iphdr = (struct ip_hdr *)((u8_t *)q->payload + out_offset);
    if (hdrlen) {
      *hdrlen = IPH_HL(iphdr) << 2;
    }
    return (iphdr);
  }
  
  return (NULL);
}

struct ip6_hdr *netdev_pbuf_ip6hdr (struct pbuf *p, int offset, int *hdrlen, int *tothdrlen, int *tproto)
{
  struct pbuf *q;
  u16_t out_offset;
  
  q = pbuf_skip(p, (u16_t)offset, &out_offset);
  if (!q) {
    return (NULL);
  }
  
  if (q->len >= (u16_t)out_offset + IP6_HLEN) {
    struct ip6_hdr *ip6hdr = (struct ip6_hdr *)((u8_t *)q->payload + offset);
    if (hdrlen) {
      *hdrlen = IP6_HLEN;
    }
    if (tothdrlen) {
      u8_t *hdr;
      int hlen, nexth;
    
      *tothdrlen = IP6_HLEN;
      hdr = (u8_t *)ip6hdr + IP6_HLEN;
      nexth = IP6H_NEXTH(ip6hdr);
      while (nexth != IP6_NEXTH_NONE) {
        switch (nexth) {
        
        case IP6_NEXTH_HOPBYHOP:
        case IP6_NEXTH_DESTOPTS:
        case IP6_NEXTH_ROUTING:
          nexth = *hdr;
          hlen = 8 * (1 + *(hdr + 1));
          (*tothdrlen) += hlen;
          hdr += hlen;
          break;
        
        case IP6_NEXTH_FRAGMENT:
          nexth = *hdr;
          hlen = 8;
          (*tothdrlen) += hlen;
          hdr += hlen;
          break;
          
        default:
          goto out;
          break;
        }
      }
out:
      if (tproto) {
        *tproto = nexth;
      }
    }
    return (ip6hdr);
  }
  
  return (NULL);
}

struct tcp_hdr *netdev_pbuf_tcphdr (struct pbuf *p, int offset, int *hdrlen)
{
  struct pbuf *q;
  u16_t out_offset;

  q = pbuf_skip(p, (u16_t)offset, &out_offset);
  if (!q) {
    return (NULL);
  }
  
  if (q->len >= (u16_t)out_offset + TCP_HLEN) {
    struct tcp_hdr *tcphdr = (struct tcp_hdr *)((u8_t *)q->payload + out_offset);
    if (hdrlen) {
      *hdrlen = TCPH_HDRLEN(tcphdr) << 2;
    }
    return (tcphdr);
  }
  
  return (NULL);
}

struct udp_hdr *netdev_pbuf_udphdr (struct pbuf *p, int offset, int *hdrlen)
{
  struct pbuf *q;
  u16_t out_offset;

  q = pbuf_skip(p, (u16_t)offset, &out_offset);
  if (!q) {
    return (NULL);
  }
  
  if (q->len >= (u16_t)out_offset + UDP_HLEN) {
    struct udp_hdr *udphdr = (struct udp_hdr *)((u8_t *)q->payload + out_offset);
    if (hdrlen) {
      *hdrlen = UDP_HLEN;
    }
    return (udphdr);
  }
  
  return (NULL);
}

#endif /* LW_CFG_NET_DEV_PROTO_ANALYSIS */
/*
 * end
 */
