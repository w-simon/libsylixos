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
** ��   ��   ��: lwip_vpn.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 05 �� 25 ��
**
** ��        ��: SSL VPN Ӧ�ýӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_VPN_EN > 0)
#include "lwip/netif.h"
#include "lwip/inet.h"
#include "lwip/tcpip.h"
#include "socket.h"
#include "polarssl/ssl.h"
#include "polarssl/havege.h"
#include "lwip_vpnlib.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT     __vpnClientOpen(__PVPN_CONTEXT  pvpnctx,
                        CPCHAR          cpcCACrtFile,
                        CPCHAR          cpcPrivateCrtFile,
                        CPCHAR          cpcKeyFile,
                        CPCHAR          cpcKeyPassword,
                        struct in_addr  inaddr,
                        u16_t           usPort,
                        INT             iSSLTimeoutSec);
INT     __vpnClientClose(__PVPN_CONTEXT  pvpnctx);
INT     __vpnNetifInit(__PVPN_CONTEXT  pvpnctx, UINT8  *pucMac);
PVOID   __vpnNetifProc(PVOID  pvArg);
/*********************************************************************************************************
  shell ����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
INT     __tshellVpnOpen(INT  iArgC, PCHAR  *ppcArgV);
INT     __tshellVpnClose(INT  iArgC, PCHAR  *ppcArgV);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: __vpnNetifDummyInit
** ��������: VPN �������ڿճ�ʼ��
** �䡡��  : netif         ����ӿ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static err_t  __vpnNetifDummyInit (struct netif  *netif)
{
    /*
     *  VPN ����ʹ���������̫�����ݰ���ʽ, ���Ա������Ϊ��̫���ӿ�
     */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    return  (ERR_OK);
}
/*********************************************************************************************************
** ��������: API_INetVpnInit
** ��������: ��ʼ�� VPN ����
** �䡡��  : pcPath        ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_INetVpnInit (VOID)
{
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("vpnopen", __tshellVpnOpen);
    API_TShellFormatAdd("vpnopen",  " [configration file]");
    API_TShellHelpAdd("vpnopen",    "create a VPN net interface.\n");

    API_TShellKeywordAdd("vpnclose", __tshellVpnClose);
    API_TShellFormatAdd("vpnclose",  " [netifname]");
    API_TShellHelpAdd("vpnclose",    "delete a VPN net interface.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
** ��������: API_INetVpnClientCreate
** ��������: ����һ�� VPN �ͻ������ӵ�ָ���ķ�����, ��������������
** �䡡��  : cpcCACrtFile                  CA ֤���ļ�     .pem or .crt
**           cpcPrivateCrtFile             ˽��֤���ļ�    .pem or .crt
**           cpcKeyFile                    ˽����Կ�ļ�    .pem or .key
**           cpcKeyPassword                ˽����Կ�ļ���������, �����Կ�ļ�����������, ��Ϊ NULL
**           cpcServerIp                   ������ ip ��ַ,       ����: "123.234.123.234"
**           cpcServerClientIp             VPN ����������ַ      ����: "192.168.0.12"
**           cpcServerClientMask           VPN ������������      ����: "255.255.255.0"
**           cpcServerClientGw             VPN ������������      ����: "192.168.0.1"
**           usPort                        VPN �������˿� (�����ֽ���)
**           iSSLTimeoutSec                SSL ͨ�ų�ʱʱ��
**           iVerifyOpt                    SSL ��֤ѡ��
**           pucMac                        6 ���ֽڵ��������� MAC ��ַ.
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetVpnClientCreate (CPCHAR          cpcCACrtFile,
                              CPCHAR          cpcPrivateCrtFile,
                              CPCHAR          cpcKeyFile,
                              CPCHAR          cpcKeyPassword,
                              CPCHAR          cpcServerIp,
                              CPCHAR          cpcServerClientIp,
                              CPCHAR          cpcServerClientMask,
                              CPCHAR          cpcServerClientGw,
                              UINT16          usPort,
                              INT             iSSLTimeoutSec,
                              INT             iVerifyOpt,
                              UINT8          *pucMac)
{
    __PVPN_CONTEXT          pvpnctx;
    INT                     iError;
    struct in_addr          inaddrServer;

    ip_addr_t               ipaddrIp;
    ip_addr_t               ipaddrMask;
    ip_addr_t               ipaddrGw;

    LW_CLASS_THREADATTR     threadattr;


    if (pucMac == LW_NULL) {                                            /*  û���������� MAC ��ַ       */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if ((cpcServerIp         == LW_NULL) ||
        (cpcServerClientIp   == LW_NULL) ||
        (cpcServerClientMask == LW_NULL) ||
        (cpcServerClientGw   == LW_NULL)) {                             /*  û�з��������ͻ�����ַ      */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (iVerifyOpt != SSL_VERIFY_NONE) {
        if ((cpcCACrtFile == LW_NULL) ||
            (cpcPrivateCrtFile == LW_NULL)) {                           /*  ȱ��֤���ļ�                */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    inaddrServer.s_addr = inet_addr(cpcServerIp);                       /*  ���������ַ                */
    ipaddrIp.addr       = ipaddr_addr(cpcServerClientIp);
    ipaddrMask.addr     = ipaddr_addr(cpcServerClientMask);
    ipaddrGw.addr       = ipaddr_addr(cpcServerClientGw);

    pvpnctx = (__PVPN_CONTEXT)__SHEAP_ALLOC(sizeof(__VPN_CONTEXT));     /*  ���� VPN ������             */
    if (pvpnctx == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pvpnctx, sizeof(__VPN_CONTEXT));
    pvpnctx->VPNCTX_iVerifyOpt = iVerifyOpt;                            /*  ��¼��֤��ʽ                */

    iError = __vpnClientOpen(pvpnctx,
                             cpcCACrtFile,
                             cpcPrivateCrtFile,
                             cpcKeyFile,
                             cpcKeyPassword,
                             inaddrServer,
                             usPort,
                             iSSLTimeoutSec);                           /*  �����������������          */
    if (iError != ERROR_NONE) {
        __SHEAP_FREE(pvpnctx);
        return  (PX_ERROR);
    }

    iError = __vpnNetifInit(pvpnctx, pucMac);                           /*  ������������ӿ�            */
    if (iError != ERROR_NONE) {
        __vpnClientClose(pvpnctx);
        __SHEAP_FREE(pvpnctx);
        return  (PX_ERROR);
    }

    iError = netifapi_netif_add(&pvpnctx->VPNCTX_netif,
                                &ipaddrIp,
                                &ipaddrMask,
                                &ipaddrGw,
                                (PVOID)pvpnctx,
                                __vpnNetifDummyInit,
                                tcpip_input);                           /*  ���� VPN ����ӿ�           */
    if (iError != ERROR_NONE) {
        __vpnClientClose(pvpnctx);
        __SHEAP_FREE(pvpnctx);
        return  (PX_ERROR);
    }

    netifapi_netif_common(&pvpnctx->VPNCTX_netif,
                          netif_set_link_up, LW_NULL);                  /*  ������������                */
    netifapi_netif_common(&pvpnctx->VPNCTX_netif,
                          netif_set_up, LW_NULL);                       /*  ʹ������                    */

    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_NET_VPN_STK_SIZE,
                        LW_PRIO_T_BUS,
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                        (PVOID)pvpnctx);
    if (API_ThreadCreate("t_vpnproc", __vpnNetifProc, &threadattr, LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
        netifapi_netif_common(&pvpnctx->VPNCTX_netif, netif_remove, LW_NULL);
        __vpnClientClose(pvpnctx);
        __SHEAP_FREE(pvpnctx);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetVpnClientDelete
** ��������: ɾ��һ�� VPN �������������ͷ������Դ
** �䡡��  : pcNetifName   ����ӿ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetVpnClientDelete (CPCHAR   pcNetifName)
{
    __PVPN_CONTEXT          pvpnctx;
    struct netif           *netif;

    if (pcNetifName == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    netif = netif_find((PCHAR)pcNetifName);
    if (netif == LW_NULL) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    if ((pcNetifName[0] != __VPN_IFNAME0) ||
        (pcNetifName[1] != __VPN_IFNAME1)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    pvpnctx = (__PVPN_CONTEXT)netif->state;
    if (pvpnctx->VPNCTX_iMode != __VPN_SSL_CLIENT) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    return  (shutdown(pvpnctx->VPNCTX_iSocket, 2));                     /*  �Ͽ� ssl ��������           */
                                                                        /*  VPN �����߳��Զ��˳�        */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_VPN_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
