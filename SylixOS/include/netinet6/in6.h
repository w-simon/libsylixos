/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: in6.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 09 月 03 日
**
** 描        述: include/netinet6/in6 .
*********************************************************************************************************/

#ifndef __NETINET6_IN6_H
#define __NETINET6_IN6_H

#include "lwip/inet.h"
#include "lwip/sockets.h"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN    46
#endif

#include "string.h"

#define IN6_ARE_ADDR_EQUAL(a, b)            \
    (memcmp(&(a)->s6_addr[0], &(b)->s6_addr[0], sizeof(struct in6_addr)) == 0)

/*********************************************************************************************************
 * Unspecified
*********************************************************************************************************/
#define IN6_IS_ADDR_UNSPECIFIED(a)    \
    ((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[12]) == 0))

/*********************************************************************************************************
 * Loopback
*********************************************************************************************************/
#define IN6_IS_ADDR_LOOPBACK(a)        \
    ((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[12]) == ntohl(1)))

/*********************************************************************************************************
 * IPv4 compatible
*********************************************************************************************************/
#define IN6_IS_ADDR_V4COMPAT(a)        \
    ((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[12]) != 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[12]) != ntohl(1)))

/*********************************************************************************************************
 * Mapped
*********************************************************************************************************/
#define IN6_IS_ADDR_V4MAPPED(a)              \
    ((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&    \
     (*(const uint32_t *)(const void *)(&(a)->s6_addr[8]) == ntohl(0x0000ffff)))

/*********************************************************************************************************
 * Unicast Scope
 * Note that we must check topmost 10 bits only, not 16 bits (see RFC2373).
*********************************************************************************************************/

#define IN6_IS_ADDR_LINKLOCAL(a)            \
    (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0x80))
    
#define IN6_IS_ADDR_SITELOCAL(a)            \
    (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0xc0))

/*********************************************************************************************************
 * Multicast
*********************************************************************************************************/

#define IN6_IS_ADDR_MULTICAST(a)            ((a)->s6_addr[0] == 0xff)

#define IN6_IS_ADDR_MC_NODELOCAL(addr)      \
        (IN6_IS_ADDR_MULTICAST(addr)        \
        && (((const uint8_t *)(addr))[1] & 0xf) == 0x1)
 
#define IN6_IS_ADDR_MC_LINKLOCAL(addr)      \
        (IN6_IS_ADDR_MULTICAST (addr)       \
        && (((const uint8_t *)(addr))[1] & 0xf) == 0x2)
 
#define IN6_IS_ADDR_MC_SITELOCAL(addr)      \
        (IN6_IS_ADDR_MULTICAST(addr)        \
        && (((const uint8_t *)(addr))[1] & 0xf) == 0x5)
 
#define IN6_IS_ADDR_MC_ORGLOCAL(addr)       \
        (IN6_IS_ADDR_MULTICAST(addr)        \
        && (((const uint8_t *)(addr))[1] & 0xf) == 0x8)
 
#define IN6_IS_ADDR_MC_GLOBAL(addr)         \
        (IN6_IS_ADDR_MULTICAST(addr)        \
        && (((const uint8_t *)(addr))[1] & 0xf) == 0xe)

/*********************************************************************************************************
 * ipv6 Options and types for UDP multicast traffic handling
*********************************************************************************************************/

#ifndef IPV6_MULTICAST_IF
#define IPV6_MULTICAST_IF	17
#define IPV6_MULTICAST_HOPS	18
#define IPV6_MULTICAST_LOOP	19
#define IPV6_JOIN_GROUP		20
#define IPV6_LEAVE_GROUP	21
#endif

/*********************************************************************************************************
 * Advanced API (RFC3542) (1)
*********************************************************************************************************/

#define IPV6_RECVPKTINFO    49
#define IPV6_PKTINFO        50
#define IPV6_RECVHOPLIMIT   51
#define IPV6_HOPLIMIT       52

/*********************************************************************************************************
 * ipv6 packet information
*********************************************************************************************************/

struct in6_pktinfo {
    struct in6_addr  ipi6_addr;                                         /* src/dst IPv6 address         */
    unsigned int     ipi6_ifindex;                                      /* send/recv interface index    */
};

/*********************************************************************************************************
 * ipv6 MTU information
*********************************************************************************************************/

struct ip6_mtuinfo {
    struct sockaddr_in6 ip6m_addr;                                      /* dst address including zone ID*/
    uint32_t            ip6m_mtu;                                       /* path MTU in host byte order  */
};

/*********************************************************************************************************
 * ipv6 space
*********************************************************************************************************/

struct ipv6_mreq {
    struct in6_addr  ipv6mr_multiaddr;                                  /* IPv6 multicast address.      */
    unsigned         ipv6mr_interface;                                  /* Interface index.             */
};

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;
extern const struct in6_addr in6addr_nodelocal_allnodes;
extern const struct in6_addr in6addr_linklocal_allnodes;

#endif                                                                  /*  __NETINET6_IN6_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
