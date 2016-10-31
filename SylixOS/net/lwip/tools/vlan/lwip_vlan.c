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
** ��   ��   ��: lwip_vlan.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 07 �� 13 ��
**
** ��        ��: lwip vlan ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "../../lwip_if.h"
#include "lwip/ip.h"
#include "netif/etharp.h"
/*********************************************************************************************************
  ����ӿ������궨��
*********************************************************************************************************/
#if LW_CFG_NET_VLAN_EN > 0
#define __LW_NETIF_MAX_NUM              10
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static INT          _G_iNetifVlanIdTbl[__LW_NETIF_MAX_NUM];
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
/*********************************************************************************************************
** ��������: etharp_vlan_set_hook
** ��������: ����������̫������� VLAN ID (LWIP_HOOK_VLAN_SET)
** �䡡��  : pvNetif             ��̫����ӿ�.
             pvPBuf              ��̫���ݱ���.
             pvEthSrc            Դ��ַ
             pvEthDst            Ŀ�ĵ�ַ
             usEthType           ��������.
** �䡡��  : vlan id or -1
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  etharp_vlan_set_hook (PVOID pvNetif, PVOID pvPBuf,
                           const PVOID pvEthSrc, const PVOID pvEthDst, UINT16  usEthType)
{
#if LW_CFG_NET_VLAN_EN > 0
    REGISTER struct netif *netif = (struct netif *)pvNetif;
    
    if (netif->num < __LW_NETIF_MAX_NUM) {
        if (_G_iNetifVlanIdTbl[netif->num] > 0) {
            return  ((s32_t)_G_iNetifVlanIdTbl[netif->num]);
        }
    }
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
    
    return  (-1);
}
/*********************************************************************************************************
** ��������: etharp_vlan_check_hook
** ��������: ���������̫������� VLAN ID (LWIP_HOOK_VLAN_SET)
** �䡡��  : pvNetif             ��̫����ӿ�.
             pvEthhdr            ��̫���ݰ�ͷ.
             pvVlanhdr           VLAN ���ݰ�ͷ.
** �䡡��  : 0: ������
**           1: ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  etharp_vlan_check_hook (PVOID pvNetif, PVOID pvEthhdr, PVOID pvVlanhdr)
{
#if LW_CFG_NET_VLAN_EN > 0
    REGISTER struct netif        *netif   = (struct netif *)pvNetif;
    REGISTER struct eth_vlan_hdr *vlanhdr = (struct eth_vlan_hdr *)pvVlanhdr;

    if (netif->num < __LW_NETIF_MAX_NUM) {
        if (_G_iNetifVlanIdTbl[netif->num] > 0) {
            if (vlanhdr->prio_vid != htons((u16_t)_G_iNetifVlanIdTbl[netif->num])) {
                return  (0);
            }
        }
    }
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */

    return  (1);
}
/*********************************************************************************************************
** ��������: API_VlanSet
** ��������: ����һ������ӿڵ� VLAN ID
** �䡡��  : pcEthIf           ��̫����ӿ���.
             iVlanId           VLAN ID (-1 ��ʾ�����κ� VLAN ID ͨ��)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_NET_VLAN_EN > 0

LW_API  
INT  API_VlanSet (CPCHAR  pcEthIf, INT  iVlanId)
{
    struct netif  *pnetif;

    if (!pcEthIf) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LWIP_NETIF_LOCK();
    pnetif = netif_find((char *)pcEthIf);
    if (!pnetif) {
        LWIP_NETIF_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    
    if (pnetif->num < __LW_NETIF_MAX_NUM) {
        _G_iNetifVlanIdTbl[pnetif->num] = iVlanId;
    }
    LWIP_NETIF_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VlanGet
** ��������: ��ȡһ������ӿڵ� VLAN ID
** �䡡��  : pcEthIf           ��̫����ӿ���.
             piVlanId          VLAN ID (-1 ��ʾ�����κ� VLAN ID ͨ��)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VlanGet (CPCHAR  pcEthIf, INT  *piVlanId)
{
    struct netif  *pnetif;

    if (!pcEthIf || !piVlanId) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LWIP_NETIF_LOCK();
    pnetif = netif_find((char *)pcEthIf);
    if (!pnetif) {
        LWIP_NETIF_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    
    if (pnetif->num < __LW_NETIF_MAX_NUM) {
        *piVlanId = _G_iNetifVlanIdTbl[pnetif->num];
    }
    LWIP_NETIF_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VlanShow
** ��������: ��ʾ��������ӿڵ� VLAN ID
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VlanShow (VOID)
{
    static const CHAR   cVlanInfoHdr[] = "\n"
    "INDEX  VLAN ID\n"
    "----- ---------\n";
    
    INT     i;
    
    printf(cVlanInfoHdr);
    
    LWIP_NETIF_LOCK();
    for (i = 0; i < __LW_NETIF_MAX_NUM; i++) {
        if (_G_iNetifVlanIdTbl[i] >= 0) {
            printf("%5d %9d\n", i, _G_iNetifVlanIdTbl[i]);
        }
    }
    LWIP_NETIF_UNLOCK();
}
/*********************************************************************************************************
** ��������: __tshellVlan
** ��������: ϵͳ���� "vlan"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellVlan (INT  iArgC, PCHAR  *ppcArgV)
{
    INT    iError;
    INT    iVlanId;
    
    if (iArgC == 1) {
        API_VlanShow();
        return  (ERROR_NONE);
    }
    
    if (lib_strcmp(ppcArgV[1], "set") == 0) {
        if (iArgC != 4) {
            fprintf(stderr, "argments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        if ((sscanf(ppcArgV[3], "%d", &iVlanId) != 1) ||
            (iVlanId < 0) || (iVlanId > 65535)) {
            fprintf(stderr, "VLAN ID error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        iError = API_VlanSet(ppcArgV[2], iVlanId);
        if (iError < 0) {
            fprintf(stderr, "VLAN ID Set error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    } else if (lib_strcmp(ppcArgV[1], "clear") == 0) {
        if (iArgC != 3) {
            fprintf(stderr, "argments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        iVlanId = -1;
        iError = API_VlanSet(ppcArgV[2], iVlanId);
        if (iError < 0) {
            fprintf(stderr, "VLAN ID Clear error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    } else {
        fprintf(stderr, "argments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __netVlanInit
** ��������: ��ʼ�� vlan ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __netVlanInit (VOID)
{
    INT     i;
    
    for (i = 0; i < __LW_NETIF_MAX_NUM; i++) {
        _G_iNetifVlanIdTbl[i] = -1;
    }
    
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("vlan", __tshellVlan);
    API_TShellFormatAdd("vlan", " [{set | clear}] [netifname]");
    API_TShellHelpAdd("vlan",   "show | set | clear net interface VLAN ID.\n"
                                "eg. vlan               show all vlan id.\n"
                                "    vlan set en0 3     set netif:en0 VLAN ID 3.\n"
                                "    vlan clear en0     clear netif:en0 VLAN ID.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}

#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
