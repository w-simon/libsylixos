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
** 文   件   名: igmp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 08 月 23 日
**
** 描        述: include/netinet/igmp.
*********************************************************************************************************/

#ifndef __NETINET_IGMP_H
#define __NETINET_IGMP_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netinet/in.h>

struct igmp {
    u_int8_t        igmp_type;                                      /* IGMP type                        */
    u_int8_t        igmp_code;                                      /* routing code                     */
    u_int16_t       igmp_cksum;                                     /* checksum                         */
    struct in_addr  igmp_group;                                     /* group address                    */
} __attribute__((__packed__));

#define IGMP_MINLEN         8

/*********************************************************************************************************
  Message types, including version number.
*********************************************************************************************************/

#define IGMP_MEMBERSHIP_QUERY       0x11                            /* membership query                 */
#define IGMP_V1_MEMBERSHIP_REPORT   0x12                            /* Ver. 1 membership report         */
#define IGMP_V2_MEMBERSHIP_REPORT   0x16                            /* Ver. 2 membership report         */
#define IGMP_V2_LEAVE_GROUP         0x17                            /* Leave-group message              */

#define IGMP_DVMRP                  0x13                            /* DVMRP routing message            */
#define IGMP_PIM                    0x14                            /* PIM routing message              */
#define IGMP_TRACE                  0x15

#define IGMP_MTRACE_RESP            0x1e                            /* traceroute resp.(to sender)      */
#define IGMP_MTRACE                 0x1f                            /* mcast traceroute messages        */

#define IGMP_MAX_HOST_REPORT_DELAY  10                              /* max delay for response to        */
                                                                    /*  query (in seconds) according    */
                                                                    /*  to RFC1112                      */
#define IGMP_TIMER_SCALE            10                              /* denotes that the igmp code field */
                                                                    /* specifies time in 10th of seconds*/

/*********************************************************************************************************
  States for the IGMP v2 state table.
*********************************************************************************************************/

#define IGMP_DELAYING_MEMBER    1
#define IGMP_IDLE_MEMBER        2
#define IGMP_LAZY_MEMBER        3
#define IGMP_SLEEPING_MEMBER    4
#define IGMP_AWAKENING_MEMBER   5

/*********************************************************************************************************
  States for IGMP router version cache.
*********************************************************************************************************/

#define IGMP_v1_ROUTER      1
#define IGMP_v2_ROUTER      2

/*********************************************************************************************************
  The following four defininitions are for backwards compatibility.
  They should be removed as soon as all applications are updated to
  use the new constant names.
*********************************************************************************************************/

#define IGMP_HOST_MEMBERSHIP_QUERY      IGMP_MEMBERSHIP_QUERY
#define IGMP_HOST_MEMBERSHIP_REPORT     IGMP_V1_MEMBERSHIP_REPORT
#define IGMP_HOST_NEW_MEMBERSHIP_REPORT IGMP_V2_MEMBERSHIP_REPORT
#define IGMP_HOST_LEAVE_MESSAGE         IGMP_V2_LEAVE_GROUP

#endif                                                              /*  __NETINET_IGMP_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
