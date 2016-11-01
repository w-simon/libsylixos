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
** ��   ��   ��: lwip_netifnum.c
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
#include "lwip_route.h"
#include "lwip_if.h"
/*********************************************************************************************************
  ����ӿ������궨��
*********************************************************************************************************/
#define __LW_NETIF_MAX_NUM              10
#define __LW_NETIF_USED                 1
#define __LW_NETIF_UNUSED               0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static struct netif        *_G_pnetifArray[__LW_NETIF_MAX_NUM];
static UINT                 _G_uiNetifNum = 0;
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
    
    if (i < __LW_NETIF_MAX_NUM) {
        rt_netif_add_hook(netif);                                       /*  ����·�ɱ���Ч��־          */
        return  (ERR_OK);
    
    } else {
        return  (ERR_USE);
    }
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
    
    rt_netif_remove_hook(netif);                                        /*  ����·�ɱ���Ч��־          */
    
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
** ��������: netif_get_num
** ��������: �������ӿ�����.
** �䡡��  : NONE
** �䡡��  : ����ӿ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT netif_get_num (VOID)
{
    UINT    uiNum;

    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    uiNum = _G_uiNetifNum;
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */

    return  (uiNum);
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

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
