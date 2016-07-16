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
** ��   ��   ��: route.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 15 ��
**
** ��        ��: SylixOS ����·�ɽӿ�.
*********************************************************************************************************/

#ifndef __SYS_ROUTE_H
#define __SYS_ROUTE_H

#include "netinet/in.h"
#include "netinet/in6.h"
#include "sys/types.h"
#include "net/if.h"

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

/*********************************************************************************************************
  route_add/change type
*********************************************************************************************************/

#define ROUTE_TYPE_HOST             0                                   /*  destination is a host       */
#define ROUTE_TYPE_NET              1                                   /*  destination is a net        */
#define ROUTE_TYPE_DEFAULT          2                                   /*  default netif               */

/*********************************************************************************************************
  route_get flag (U H G flag)
*********************************************************************************************************/

#define ROUTE_RTF_UP                0x01                                /* route useable                */
#define ROUTE_RTF_GATEWAY           0x02                                /* destination is a gateway     */
#define ROUTE_RTF_HOST              0x04                                /* host entry (net otherwise)   */

/*********************************************************************************************************
  route_get msgbuf
*********************************************************************************************************/

struct route_msg {
    u_char          rm_flag;                                            /*  ����                        */
    int             rm_metric;                                          /*  ��ʱδʹ��                  */
    struct in_addr  rm_dst;                                             /*  Ŀ�ĵ�ַ��Ϣ                */
    struct in_addr  rm_gw;                                              /*  ������Ϣ                    */
    struct in_addr  rm_mask;                                            /*  ����������Ϣ                */
    struct in_addr  rm_if;                                              /*  ����ӿڵ�ַ                */
    char            rm_ifname[IF_NAMESIZE];                             /*  ����ӿ���                  */
};

/*********************************************************************************************************
  api (only for ipv4)
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

LW_API int  route_add(struct in_addr *pinaddr, struct in_addr *pinaddrGw, int  type, const char *ifname);
LW_API int  route_delete(struct in_addr *pinaddr);
LW_API int  route_change(struct in_addr *pinaddr, struct in_addr *pinaddrGw, int  type, const char *ifname);
LW_API int  route_getnum(void);                                         /*  ���·�ɱ�������            */
LW_API int  route_get(u_char flag, struct route_msg  *msgbuf, size_t  num);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
#endif                                                                  /*  __SYS_ROUTE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
