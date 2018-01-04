/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: pim6.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 01 �� 02 ��
**
** ��        ��: include/netinet6/pim6.
*********************************************************************************************************/

#ifndef __NETINET6_PIM6_H
#define __NETINET6_PIM6_H

#include "lwip/opt.h"
#include "lwip/def.h"

#if LWIP_IPV6 && LWIP_IPV6_FORWARD && LWIP_IPV6_MLD

#include "sys/types.h"
#include "sys/endian.h"
#include "lwip/inet.h"
#include "net/if.h"

/*********************************************************************************************************
  PIM packet header
  
  the PIM message type, currently they are:
  Hello, Register, Register-Stop, Join/Prune,
  Bootstrap, Assert, Graft (PIM-DM only),
  Graft-Ack (PIM-DM only), C-RP-Adv
*********************************************************************************************************/

struct pim {
#if defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
    u_char  pim_type:4,     
            pim_ver:4;                                          /* PIM version number; 2 for PIMv2      */
#else
    u_char  pim_ver:4,                                          /* PIM version                          */
            pim_type:4;                                         /* PIM type                             */
#endif
    u_char  pim_rsv;                                            /* Reserved                             */
    u_short pim_cksum;                                          /* IP style check sum                   */
};

/*********************************************************************************************************
  KAME-related name backward compatibility
*********************************************************************************************************/

#define PIM_VERSION         2
#define PIM_MINLEN          8                                   /* PIM message min. length              */
#define PIM6_REG_MINLEN     (PIM_MINLEN + 40)                   /* PIM Register hdr + inner IPv6 hdr    */

/*********************************************************************************************************
  Message types
*********************************************************************************************************/

#define PIM_REGISTER        1                                   /* PIM Register type is 1               */

/*********************************************************************************************************
  PIM-Register message flags
*********************************************************************************************************/

#define PIM_NULL_REGISTER   0x40000000U                         /* The Null-Register bit (host-order)   */

/*********************************************************************************************************
  PIM statistics kept in the kernel
*********************************************************************************************************/

struct pim6stat {
    uint64_t    pim6s_rcv_total;                                /* total PIM messages received          */
    uint64_t    pim6s_rcv_tooshort;                             /* received with too few bytes          */
    uint64_t    pim6s_rcv_badsum;                               /* received with bad checksum           */
    uint64_t    pim6s_rcv_badversion;                           /* received bad PIM version             */
    uint64_t    pim6s_rcv_registers;                            /* received registers                   */
    uint64_t    pim6s_rcv_badregisters;                         /* received invalid registers           */
    uint64_t    pim6s_snd_registers;                            /* sent registers                       */
};

/*********************************************************************************************************
  Identifiers for PIM sysctl nodes
*********************************************************************************************************/

#define PIM6CTL_STATS    1                                      /* statistics (read-only)               */

#endif                                                          /* LWIP_IPV6 && LWIP_IPV6_FORWARD &&    */
                                                                /* LWIP_IPV6_MLD                        */
#endif                                                          /*  __NETINET6_PIM6_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
