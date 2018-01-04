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
** ��   ��   ��: lwip_netif.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 07 �� 30 ��
**
** ��        ��: lwip ���ڼ������Ľ�.
                 lwip netif_add() ������������ӿڼ�����, �� netif_remove() ��û�д���.
                 
** BUG:
2011.02.13  netif_remove_hook() �м���� npf detach �Ĳ���, ȷ�� attach �� detach �ɶԲ���.
2011.03.10  �� _G_ulNetIfLock ����, posix net/if.h ��Ҫ����.
2011.07.04  �����·�ɱ�Ļص�����.
2013.04.16  netif_remove_hook ��Ҫж�� DHCP ���ݽṹ.
2013.09.24  �Ƴ�����ӿڼ���� auto ip �Ļ���.
2014.03.22  ����ͨ��������, ���ٵõ� netif.
*********************************************************************************************************/
#define  __SYLIXOS_RTHOOK
#define  __SYLIXOS_KERNEL
#define  __NETIF_MAIN_FILE
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/err.h"
#include "lwip_if.h"
#include "net/if.h"
#if LW_CFG_NET_ROUTER > 0
#include "net/route.h"
#include "route/af_route.h"
#include "route/ip4_route.h"
#include "route/ip6_route.h"
#if LW_CFG_NET_BALANCING > 0
#include "net/sroute.h"
#include "balancing/ip4_sroute.h"
#include "balancing/ip6_sroute.h"
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
#if LW_CFG_NET_MROUTER > 0
#include "mroute/ip4_mrt.h"
#endif                                                                  /*  LW_CFG_NET_MROUTER > 0      */
#if LW_CFG_NET_FLOWCTL_EN > 0
#include "flowctl/net_flowctl.h"
#endif
/*********************************************************************************************************
  ����ӿ������궨��
*********************************************************************************************************/
#define __LW_NETIF_MAX_NUM      LW_CFG_NET_DEV_MAX
#define __LW_NETIF_USED         1
#define __LW_NETIF_UNUSED       0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static struct netif *_G_pnetifArray[__LW_NETIF_MAX_NUM];
static UINT          _G_uiNetifNum = 0;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#if LW_CFG_NET_NPF_EN > 0
VOID  __npfNetifRemoveHook(struct netif  *pnetif);
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */
/*********************************************************************************************************
** ��������: netif_add_hook
** ��������: ��������ӿڻص�����, ��������ӿں� (�����������е���)
** �䡡��  : pvNetif     ����ӿ�
** �䡡��  : ERR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  netif_add_hook (PVOID  pvNetif)
{
    struct netif  *netif = (struct netif *)pvNetif;
    INT            i;
    
    if (_G_ulNetifLock == 0) {
        _G_ulNetifLock =  API_SemaphoreMCreate("netif_lock", LW_PRIO_DEF_CEILING, 
                                               LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_INHERIT_PRIORITY |
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    for (i = 0; i < __LW_NETIF_MAX_NUM; i++) {
        if (_G_pnetifArray[i] == LW_NULL) {
            _G_pnetifArray[i] =  netif;
            netif->num        =  (UINT8)i;
            _G_uiNetifNum++;
            break;
        }
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    if (i >= __LW_NETIF_MAX_NUM) {
        return  (ERR_USE);
    }
    
#if LW_CFG_NET_ROUTER > 0
    rt_netif_add_hook(netif);                                           /*  ����·�ɱ���Ч��־          */
#if LWIP_IPV6
    rt6_netif_add_hook(netif);
#endif
#if LW_CFG_NET_BALANCING > 0
    srt_netif_add_hook(netif);
#if LWIP_IPV6
    srt6_netif_add_hook(netif);
#endif
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
    route_hook_netif_ann(netif, 0);
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
#if LW_CFG_NET_FLOWCTL_EN > 0
    fcnet_netif_attach(netif);
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */
    
    return  (ERR_OK);
}
/*********************************************************************************************************
** ��������: netif_remove_hook
** ��������: ɾ������ӿڻص�����. (�����������е���)
** �䡡��  : pvNetif     ����ӿ�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  netif_remove_hook (PVOID  pvNetif)
{
    struct netif  *netif = (struct netif *)pvNetif;
    INT            iNum  = (INT)netif->num;
    
#if LW_CFG_NET_ROUTER > 0
    rt_netif_remove_hook(netif);                                        /*  ����·�ɱ���Ч��־          */
#if LWIP_IPV6
    rt6_netif_remove_hook(netif);
#endif
#if LW_CFG_NET_BALANCING > 0
    srt_netif_remove_hook(netif);
#if LWIP_IPV6
    srt6_netif_remove_hook(netif);
#endif
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#if LW_CFG_NET_MROUTER > 0
    ip4_mrt_if_detach(netif);
#endif                                                                  /*  LW_CFG_NET_MROUTER > 0      */
    route_hook_netif_ann(netif, 1);
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
#if LW_CFG_NET_FLOWCTL_EN > 0
    fcnet_netif_detach(netif);
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */
    
    if (iNum < __LW_NETIF_MAX_NUM) {
        LWIP_NETIF_LOCK();                                              /*  �����ٽ���                  */
        _G_pnetifArray[iNum] = LW_NULL;
        _G_uiNetifNum--;
        LWIP_NETIF_UNLOCK();                                            /*  �˳��ٽ���                  */
    }
    
#if LW_CFG_NET_NPF_EN > 0
    __npfNetifRemoveHook(netif);
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */

#if LWIP_DHCP > 0
    if (netif_dhcp_data(netif)) {
        dhcp_stop(netif);                                               /*  �ر� DHCP ���� UDP ���ƿ�   */
        dhcp_cleanup(netif);                                            /*  ���� DHCP �ڴ�              */
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

#if LWIP_AUTOIP > 0
    if (netif_autoip_data(netif)) {
        mem_free(netif_autoip_data(netif));                             /*  ���� AUTOIP �ڴ�            */
        netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_AUTOIP, NULL);
    }
#endif                                                                  /*  LWIP_AUTOIP > 0             */
}
/*********************************************************************************************************
** ��������: netif_updown_hook
** ��������: ����ӿ�ʹ�ܽ��ܻص�.
** �䡡��  : pvNetif    ����ӿ�
**           up         1: ʹ�� 0: ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID netif_updown_hook (PVOID  pvNetif, INT up)
{
    struct netif  *netif = (struct netif *)pvNetif;
    
#if LW_CFG_NET_ROUTER > 0
    if (!up) {
        rt_netif_invcache_hook(netif);
#if LWIP_IPV6
        rt6_netif_invcache_hook(netif);
#endif
    }
    route_hook_netif_updown(netif, up);
#endif
}
/*********************************************************************************************************
** ��������: netif_link_updown_hook
** ��������: ����ӿ� link ����״̬�ص�.
** �䡡��  : pvNetif    ����ӿ�
**           linkup     1: ���� 0: ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID netif_link_updown_hook (PVOID  pvNetif, INT linkup)
{
    struct netif  *netif = (struct netif *)pvNetif;
    
#if LW_CFG_NET_ROUTER > 0
    if (!linkup) {
        rt_netif_invcache_hook(netif);
#if LWIP_IPV6
        rt6_netif_invcache_hook(netif);
#endif
    }
#endif
}
/*********************************************************************************************************
** ��������: netif_set_addr_hook
** ��������: ����ӿ����õ�ַ.
** �䡡��  : pvNetif    ����ӿ�
**           pvIpaddr   �µ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID netif_set_addr_hook (PVOID  pvNetif, const PVOID pvIpaddr)
{
    struct netif  *netif = (struct netif *)pvNetif;
    
#if LW_CFG_NET_ROUTER > 0
    route_hook_netif_ipv4(netif, (ip4_addr_t *)pvIpaddr);
#endif
}
/*********************************************************************************************************
** ��������: netif_set_addr6_hook
** ��������: ����ӿ����õ�ַ.
** �䡡��  : pvNetif    ����ӿ�
**           pvIpaddr   �µ�ַ
**           iIndex     ��ַ�±�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID netif_set_addr6_hook (PVOID  pvNetif, const PVOID pvIp6addr, INT iIndex)
{
    struct netif  *netif = (struct netif *)pvNetif;
    
#if LW_CFG_NET_ROUTER > 0
    const ip6_addr_t  *pip6addr;

    pip6addr = netif_ip6_addr(netif, iIndex);
    if (!ip6_addr_isany(pip6addr)) {
        route_hook_netif_ipv6(netif, pip6addr, RTM_DELADDR);
    }
    
    pip6addr = (ip6_addr_t *)pvIp6addr;
    if (!ip6_addr_isany(pip6addr)) {
        route_hook_netif_ipv6(netif, pip6addr, RTM_NEWADDR);
    }
#endif
}
/*********************************************************************************************************
** ��������: netif_set_maddr_hook
** ��������: ����ӿ� ��� / ɾ�� �鲥��ַ.
** �䡡��  : pvNetif    ����ӿ�
**           pvIpaddr   �µ�ַ
**           iAdd       �Ƿ�Ϊ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID netif_set_maddr_hook (PVOID  pvNetif, const PVOID pvIpaddr, INT iAdd)
{
    struct netif  *netif = (struct netif *)pvNetif;
    
#if LW_CFG_NET_ROUTER > 0
    route_hook_maddr_ipv4(netif, (ip4_addr_t *)pvIpaddr, (u_char)(iAdd ? RTM_NEWMADDR : RTM_DELMADDR));
#endif
}
/*********************************************************************************************************
** ��������: netif_set_addr6_hook
** ��������: ����ӿ� ��� / ɾ�� �鲥��ַ.
** �䡡��  : pvNetif    ����ӿ�
**           pvIpaddr   �µ�ַ
**           iIndex     ��ַ�±�
**           iAdd       �Ƿ�Ϊ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID netif_set_maddr6_hook (PVOID  pvNetif, const PVOID pvIp6addr,  INT iAdd)
{
    struct netif  *netif = (struct netif *)pvNetif;
    
#if LW_CFG_NET_ROUTER > 0
    route_hook_maddr_ipv6(netif, (ip6_addr_t *)pvIp6addr, (u_char)(iAdd ? RTM_NEWMADDR : RTM_DELMADDR));
#endif
}
/*********************************************************************************************************
** ��������: netif_get_num
** ��������: �������ӿ�����.
** �䡡��  : NONE
** �䡡��  : ����ӿ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT netif_get_num (VOID)
{
    return  (_G_uiNetifNum);
}
/*********************************************************************************************************
** ��������: netif_get_by_index
** ��������: ͨ�� index �������ӿڽṹ. (û�м���)
** �䡡��  : uiIndex       index
** �䡡��  : ����ӿ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID netif_get_by_index (UINT uiIndex)
{
    if (uiIndex < __LW_NETIF_MAX_NUM) {
        return  (_G_pnetifArray[uiIndex]);
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: netif_get_flags
** ��������: ͨ�� index �������ӿڽṹ. (û�м���)
** �䡡��  : uiIndex       index
** �䡡��  : ����ӿ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  netif_get_flags (struct netif *pnetif)
{
    INT  iFlags = 0;

    if (pnetif->flags & NETIF_FLAG_UP) {
        iFlags |= IFF_UP;
    }
    if (pnetif->flags & NETIF_FLAG_BROADCAST) {
        iFlags |= IFF_BROADCAST;
    } else {
        iFlags |= IFF_POINTOPOINT;
    }
    if (pnetif->flags & NETIF_FLAG_LINK_UP) {
        iFlags |= IFF_RUNNING;
    }
    if (pnetif->flags & NETIF_FLAG_IGMP) {
        iFlags |= IFF_MULTICAST;
    }
    if ((pnetif->flags & NETIF_FLAG_ETHARP) == 0) {
        iFlags |= IFF_NOARP;
    }
    if (netif_ip4_addr(pnetif)->addr == ntohl(INADDR_LOOPBACK)) {
        iFlags |= IFF_LOOPBACK;
    }
    if ((pnetif->flags2 & NETIF_FLAG2_PROMISC)) {
        iFlags |= IFF_PROMISC;
    }
    if ((pnetif->flags2 & NETIF_FLAG2_ALLMULTI)) {
        iFlags |= IFF_ALLMULTI;
    }
    
    return  (iFlags);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
