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
** ��   ��   ��: lwip_nat.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 03 �� 19 ��
**
** ��        ��: lwip NAT ֧�ְ�.

** BUG:
2011.03.06  ���� gcc 4.5.1 ��� warning.
2011.07.30  nats ������������ն�(����: telnet)�б�����, ��Ϊ��Ҫʹ�� NAT ��, printf ������մ�ӡ������
            ��, ����� netproto ������ռ NAT ��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_NAT_EN > 0)
#include "socket.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip_natlib.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
VOID        __natPoolCreate(VOID);
VOID        __natTimer(VOID);
INT         __natMapAdd(ip4_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto);
INT         __natMapDelete(ip4_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto);
INT         __natAliasAdd(const ip4_addr_t  *pipaddrAlias, 
                          const ip4_addr_t  *ipaddrSLocalIp,
                          const ip4_addr_t  *ipaddrELocalIp);
INT         __natAliasDelete(const ip4_addr_t  *pipaddrAlias);

#if LW_CFG_PROCFS_EN > 0
VOID        __procFsNatInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#if LW_CFG_SHELL_EN > 0
static INT  __tshellNat(INT  iArgC, PCHAR  ppcArgV[]);
static INT  __tshellNatMap(INT  iArgC, PCHAR  ppcArgV[]);
static INT  __tshellNatAlias(INT  iArgC, PCHAR  ppcArgV[]);
static INT  __tshellNatShow(INT  iArgC, PCHAR  ppcArgV[]);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  NAT ���������� AP ��������ӿ�
*********************************************************************************************************/
extern struct netif        *_G_pnetifNatLocal;
extern struct netif        *_G_pnetifNatAp;
/*********************************************************************************************************
  NAT ������
*********************************************************************************************************/
extern LW_OBJECT_HANDLE     _G_ulNatOpLock;
/*********************************************************************************************************
  NAT ˢ�¶�ʱ��
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_ulNatTimer;
/*********************************************************************************************************
** ��������: API_INetNatInit
** ��������: internet NAT ��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_INetNatInit (VOID)
{
    static   BOOL   bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return;
    }
    
    __natPoolCreate();                                                  /*  ���� NAT ���ƿ黺���       */
    
    _G_ulNatOpLock = API_SemaphoreMCreate("nat_lock", LW_PRIO_DEF_CEILING, 
                                          LW_OPTION_INHERIT_PRIORITY |
                                          LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_OBJECT_GLOBAL,
                                          LW_NULL);                     /*  ���� NAT ������             */
                                          
    _G_ulNatTimer = API_TimerCreate("nat_timer", LW_OPTION_ITIMER | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    
    API_TimerStart(_G_ulNatTimer, 
                   (LW_TICK_HZ * 60),
                   LW_OPTION_AUTO_RESTART,
                   (PTIMER_CALLBACK_ROUTINE)__natTimer,
                   LW_NULL);                                            /*  ÿ����ִ��һ��              */
    
#if LW_CFG_PROCFS_EN > 0
    __procFsNatInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
    
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("nat", __tshellNat);
    API_TShellFormatAdd("nat",  " [stop] | {[LAN netif] [WAN netif]}");
    API_TShellHelpAdd("nat",   "start or stop NAT network.\n"
                               "eg. nat wl2 en1 (start NAT network use wl2 as LAN, en1 as WAN)\n"
                               "    nat stop    (stop NAT network)\n");
    
    API_TShellKeywordAdd("natalias", __tshellNatAlias);
    API_TShellFormatAdd("natalias",  " {[add] [alias] [LAN start] [LAN end]} | {[del] [alias]}");
    API_TShellHelpAdd("natalias",    "add or delete NAT alias.\n"
                                     "eg. natalias add 11.22.33.44 192.168.1.2 192.168.1.3  (add alias to 192..2 ~ 192..3)\n"
                                     "    natalias del 11.22.33.44                          (delete alias)\n");
                                     
    API_TShellKeywordAdd("natmap", __tshellNatMap);
    API_TShellFormatAdd("natmap",  " {[add] | [del]} [WAN port] [LAN port] [LAN IP] [protocol]");
    API_TShellHelpAdd("natmap",    "add or delete NAT maps.\n"
                                     "eg. natmap add 80 80 192.168.1.2 tcp (map webserver as 192..2)\n"
                                     "    natmap del 80 80 192.168.1.2 tcp (unmap webserver as 192..2)\n");
                                     
    API_TShellKeywordAdd("nats", __tshellNatShow);
    API_TShellHelpAdd("nats",   "show NAT networking infomation.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    bIsInit = LW_TRUE;
}
/*********************************************************************************************************
** ��������: API_INetNatStart
** ��������: ���� NAT (��������ӿڱ���Ϊ lwip Ĭ�ϵ�·�ɽӿ�)
** �䡡��  : pcLocalNetif          ������������ӿ���
**           pcApNetif             ��������ӿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetNatStart (CPCHAR  pcLocalNetif, CPCHAR  pcApNetif)
{
    struct netif   *pnetifLocal;
    struct netif   *pnetifAp;

    if (!pcLocalNetif || !pcApNetif) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_G_pnetifNatLocal || _G_pnetifNatAp) {                          /*  NAT ���ڹ���                */
        _ErrorHandle(EISCONN);
        return  (PX_ERROR);
    }
    
    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    pnetifLocal = netif_find((PCHAR)pcLocalNetif);
    pnetifAp    = netif_find((PCHAR)pcApNetif);
    
    if (!pnetifLocal || !pnetifAp) {                                    /*  ������ӿڲ�����            */
        __NAT_OP_UNLOCK();                                              /*  ���� NAT ����               */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();
    _G_pnetifNatLocal = pnetifLocal;                                    /*  ��������ӿ�                */
    _G_pnetifNatAp    = pnetifAp;
    __KERNEL_EXIT();
    
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetNatStop
** ��������: ֹͣ NAT (ע��: ɾ�� NAT ����ӿ�ʱ, ����Ҫ��ֹͣ NAT ����ӿ�)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetNatStop (VOID)
{
    if (!_G_pnetifNatLocal || !_G_pnetifNatAp) {                        /*  NAT û�й���                */
        _ErrorHandle(ENOTCONN);
        return  (PX_ERROR);
    }
    
    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    
    __KERNEL_ENTER();
    _G_pnetifNatLocal = LW_NULL;
    _G_pnetifNatAp    = LW_NULL;
    __KERNEL_EXIT();
    
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetNatMapAdd
** ��������: ������������ӳ��
** �䡡��  : pcLocalIp         ���� IP
**           usLocalPort       ���� �˿�
**           usAssPort         ӳ�� �˿�
**           ucProto           Э�� IP_PROTO_TCP / IP_PROTO_UDP
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetNatMapAdd (CPCHAR  pcLocalIp, UINT16  usLocalPort, UINT16  usAssPort, UINT8  ucProto)
{
    ip4_addr_t ipaddr;
    INT        iRet;
    
    if (!pcLocalIp) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (usAssPort >= LW_CFG_NET_NAT_MIN_PORT) {
        _ErrorHandle(EADDRINUSE);
        return  (PX_ERROR);
    }
    
    if ((ucProto != IPPROTO_TCP) && (ucProto != IPPROTO_UDP)) {
        _ErrorHandle(ENOPROTOOPT);
        return  (PX_ERROR);
    }
    
    if (!ip4addr_aton(pcLocalIp, &ipaddr)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iRet = __natMapAdd(&ipaddr, htons(usLocalPort), htons(usAssPort), ucProto);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_INetNatMapDelete
** ��������: ɾ����������ӳ��
** �䡡��  : pcLocalIp         ���� IP
**           usLocalPort       ���� �˿�
**           usAssPort         ӳ�� �˿�
**           ucProto           Э�� IP_PROTO_TCP / IP_PROTO_UDP
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetNatMapDelete (CPCHAR  pcLocalIp, UINT16  usLocalPort, UINT16  usAssPort, UINT8  ucProto)
{
    ip4_addr_t ipaddr;
    INT        iRet;
    
    if (!pcLocalIp) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (usAssPort >= LW_CFG_NET_NAT_MIN_PORT) {
        _ErrorHandle(EADDRINUSE);
        return  (PX_ERROR);
    }
    
    if ((ucProto != IPPROTO_TCP) && (ucProto != IPPROTO_UDP)) {
        _ErrorHandle(ENOPROTOOPT);
        return  (PX_ERROR);
    }
    
    if (!ip4addr_aton(pcLocalIp, &ipaddr)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iRet = __natMapDelete(&ipaddr, htons(usLocalPort), htons(usAssPort), ucProto);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_INetNatAliasAdd
** ��������: NAT ����������� (���� IP)
** �䡡��  : pcAliasIp         ���� IP
**           pcSLocalIp        ���� IP ��ʼλ��
**           pcELocalIp        ���� IP ����λ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetNatAliasAdd (CPCHAR  pcAliasIp, CPCHAR  pcSLocalIp, CPCHAR  pcELocalIp)
{
    ip4_addr_t  ipaddrAlias;
    ip4_addr_t  ipaddrSLocalIp;
    ip4_addr_t  ipaddrELocalIp;
    
    INT        iRet;
    
    if (!pcAliasIp || !pcSLocalIp || !pcELocalIp) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!ip4addr_aton(pcAliasIp, &ipaddrAlias)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!ip4addr_aton(pcSLocalIp, &ipaddrSLocalIp)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!ip4addr_aton(pcELocalIp, &ipaddrELocalIp)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iRet = __natAliasAdd(&ipaddrAlias, &ipaddrSLocalIp, &ipaddrELocalIp);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_INetNatAliasDelete
** ��������: NAT ɾ��������� (���� IP)
** �䡡��  : pcAliasIp         ���� IP
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetNatAliasDelete (CPCHAR  pcAliasIp)
{
    ip4_addr_t  ipaddrAlias;
    INT        iRet;
    
    if (!pcAliasIp) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!ip4addr_aton(pcAliasIp, &ipaddrAlias)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iRet = __natAliasDelete(&ipaddrAlias);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __tshellNat
** ��������: ϵͳ���� "nat"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNat (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 3) {
        if (API_INetNatStart(ppcArgV[1], ppcArgV[2]) != ERROR_NONE) {
            fprintf(stderr, "can not start NAT network, errno: %s\n", lib_strerror(errno));
        
        } else {
            printf("NAT network started, [LAN: %s] [WAN: %s]\n", ppcArgV[1], ppcArgV[2]);
        }
    
    } else if (iArgC == 2) {
        if (lib_strcmp(ppcArgV[1], "stop") == 0) {
            API_INetNatStop();
            printf("NAT network stoped.\n");
        }

    } else {
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNatMap
** ��������: ϵͳ���� "natmap"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellNatMap (INT  iArgC, PCHAR  ppcArgV[])
{
    INT         iError;
    INT         iWanPort;
    INT         iLanPort;
    UINT8       ucProto;
    
    if (iArgC != 6) {
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (sscanf(ppcArgV[2], "%d", &iWanPort) != 1) {
        fprintf(stderr, "WAN port error option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    if (sscanf(ppcArgV[3], "%d", &iLanPort) != 1) {
        fprintf(stderr, "LAN port error option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcasecmp(ppcArgV[5], "tcp") == 0) {
        ucProto = IPPROTO_TCP;
        
    } else if (lib_strcasecmp(ppcArgV[5], "udp") == 0) {
        ucProto = IPPROTO_UDP;
    
    } else {
        fprintf(stderr, "protocol option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcmp(ppcArgV[1], "add") == 0) {
        iError = API_INetNatMapAdd(ppcArgV[4], (UINT16)iLanPort, (UINT16)iWanPort, ucProto);
        if (iError < 0) {
            fprintf(stderr, "add NAT map error: %s!\n", lib_strerror(errno));
            return  (-ERROR_TSHELL_EPARAM);
        }
        
    } else if (lib_strcmp(ppcArgV[1], "del") == 0) {
        iError = API_INetNatMapDelete(ppcArgV[4], (UINT16)iLanPort, (UINT16)iWanPort, ucProto);
        if (iError < 0) {
            fprintf(stderr, "delete NAT map error: %s!\n", lib_strerror(errno));
            return  (-ERROR_TSHELL_EPARAM);
        }
        
    } else {
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNatAlias
** ��������: ϵͳ���� "natalias"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellNatAlias (INT  iArgC, PCHAR  ppcArgV[])
{
    INT    iError;
    
    if (iArgC == 5) {
        if (lib_strcmp(ppcArgV[1], "add")) {
            goto    __error;
        }
        iError = API_INetNatAliasAdd(ppcArgV[2], ppcArgV[3], ppcArgV[4]);
        if (iError < 0) {
            fprintf(stderr, "add NAT alias error: %s!\n", lib_strerror(errno));
            return  (-ERROR_TSHELL_EPARAM);
        }
    
    } else if (iArgC == 3) {
        if (lib_strcmp(ppcArgV[1], "del")) {
            goto    __error;
        }
        iError = API_INetNatAliasDelete(ppcArgV[2]);
        if (iError < 0) {
            fprintf(stderr, "delete NAT alias error: %s!\n", lib_strerror(errno));
            return  (-ERROR_TSHELL_EPARAM);
        }
    
    } else {
__error:
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNatShow
** ��������: ϵͳ���� "nats"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellNatShow (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iFd;
    CHAR    cBuffer[512];
    ssize_t sstNum;
    
    iFd = open("/proc/net/nat/info", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/nat/info : %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
