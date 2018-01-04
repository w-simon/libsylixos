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
** 文   件   名: lwip_route.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 07 月 01 日
**
** 描        述: lwip sylixos 路由表.
*********************************************************************************************************/

#ifndef __LWIP_ROUTE_H
#define __LWIP_ROUTE_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#if LW_CFG_SHELL_EN > 0
INT  __tshellRoute(INT  iArgC, PCHAR  *ppcArgV);
VOID __tshellRouteInit(VOID);
#if LW_CFG_NET_BALANCING > 0
VOID __tshellSrouteInit(VOID);
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __LWIP_ROUTE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
