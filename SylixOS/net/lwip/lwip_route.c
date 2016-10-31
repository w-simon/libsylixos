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
** ��   ��   ��: lwip_route.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 07 �� 01 ��
**
** ��        ��: lwip sylixos ·�ɱ�.
                 lwip ·�ɱ�ӿڵ�������, �����: http://savannah.nongnu.org/bugs/?33634

** BUG:
2011.07.07  �� net safe ״̬�²�������� printf ��ʹ�� IO �����.
2011.08.17  __rtSafeRun() �ڰ�װ lo0 ����ʱ, lwip core lock ��û�д���, ����������Ҫ�ж�һ��!
2012.03.29  __aodvEntryPrint() hcnt > 0 ������ת���ڵ�, Ӧ��ʾΪ G.
2013.01.15  �����·�ɲ����ӿ�.
2013.01.16  LW_RT_FLAG_G ��־Ϊ��̬��, ÿ�α���ʱ�Ż����, ��Ϊ���������ĸĶ����ܻ�����˱�־�ı仯.
2013.01.24  route ����ٴ�ӡ aodv ·�ɱ�, ���� aodvs �����ӡ aodv ·�ɱ�.
2013.06.21  adovs ��������Ŀ�������͵������ӵ���ʾ.
2013.08.22  route_msg ���� metric �ֶ�, ����ǰ����.
2014.07.02  �����ڽ�·�ɱ���� ppp ���ӵĴ�����ʾ.
2016.07.16  ÿһ��·����Ϣ������������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "sys/route.h"
#include "net/if.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/api.h"
#include "lwip/netif.h"
#include "src/netif/aodv/aodv_route.h"                                  /*  AODV ·�ɱ�                 */
/*********************************************************************************************************
  �������ӿ���
*********************************************************************************************************/
#define __LW_NETIF_MAX_NUM  10
/*********************************************************************************************************
  ·�ɱ�ڵ�
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE    RTE_lineManage;                                     /*  management list             */
    ip4_addr_t      RTE_ipaddrDest;                                     /*  dest address                */
    ip4_addr_t      RTE_ipaddrGw;                                       /*  gw IPADDR_ANY use netif->gw */
    struct netif   *RTE_pnetif;                                         /*  net device                  */
    CHAR            RTE_cNetifName[IF_NAMESIZE];                        /*  net device name             */
    UINT            RTE_uiFlag;                                         /*  route entry flag            */
} LW_RT_ENTRY;
typedef LW_RT_ENTRY        *PLW_RT_ENTRY;

#define LW_RT_FLAG_U        0x01                                        /*  valid                       */
#define LW_RT_FLAG_G        0x02                                        /*  to route                    */
#define LW_RT_FLAG_H        0x04                                        /*  to host                     */
#define LW_RT_FLAG_D        0x08                                        /*  icmp create                 */
#define LW_RT_FLAG_M        0x10                                        /*  icmp modify                 */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT             _G_uiTotalNum;
static UINT             _G_uiActiveNum;

#define LW_RT_TABLESIZE     32                                          /*  2 ^ 5 �����                */
#define LW_RT_TABLEMASK     (LW_RT_TABLESIZE - 1)
#define LW_RT_TABLESHIFT    (32 - 5)                                    /*  ʹ�� ip ��ַ�� 5 λΪ hash  */

#define LW_RT_HASHINDEX(pipaddr)    \
        (((pipaddr)->addr >> LW_RT_TABLESHIFT) & LW_RT_TABLEMASK)
        
static PLW_LIST_LINE    _G_plineRtHashHeader[LW_RT_TABLESIZE];
/*********************************************************************************************************
  CACHE
*********************************************************************************************************/
typedef struct {
    ip4_addr_t      RTCACHE_ipaddrDest;
    PLW_RT_ENTRY    RTCACHE_prteCache;
} LW_RT_CACHE;
static LW_RT_CACHE      _G_rtcache;

#define LW_RT_CACHE_INVAL(prte)                                 \
        if (_G_rtcache.RTCACHE_prteCache == prte) {             \
            _G_rtcache.RTCACHE_prteCache       = LW_NULL;       \
            _G_rtcache.RTCACHE_ipaddrDest.addr = IPADDR_ANY;    \
        }
        
typedef struct {
    ip4_addr_t      GWCACHE_ipaddrDest;
    PLW_RT_ENTRY    GWCACHE_prteCache;
} LW_GW_CACHE;
static LW_GW_CACHE      _G_gwcache[__LW_NETIF_MAX_NUM];

#define LW_GW_CACHE_INVAL(index, prte)                                  \
        if (_G_gwcache[index].GWCACHE_prteCache == prte) {              \
            _G_gwcache[index].GWCACHE_prteCache       = LW_NULL;        \
            _G_gwcache[index].GWCACHE_ipaddrDest.addr = IPADDR_ANY;     \
        }
/*********************************************************************************************************
** ��������: __rtSafeRun
** ��������: ��ȫ������һ���ص�����, �����ƻ�Э��ջ
** �䡡��  : pfuncHook     ִ�к���
**           pvArg0...5    ����
** �䡡��  : ִ�н��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtSafeRun (FUNCPTR  pfuncHook,
                        PVOID    pvArg0,
                        PVOID    pvArg1,
                        PVOID    pvArg2,
                        PVOID    pvArg3,
                        PVOID    pvArg4,
                        PVOID    pvArg5)
{
#if LWIP_TCPIP_CORE_LOCKING < 1
#error sylixos need LWIP_TCPIP_CORE_LOCKING > 0
#endif                                                                  /*  LWIP_TCPIP_CORE_LOCKING     */

    INT     iError;

    if (sys_mutex_valid(&lock_tcpip_core)) {
        LOCK_TCPIP_CORE();
        iError = pfuncHook(pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
        UNLOCK_TCPIP_CORE();
    } else {
        iError = pfuncHook(pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __rtMatch
** ��������: ƥ��һ�� sylixos ·����Ŀ (����ѡ������·��, Ȼ��ѡ������·��)
** �䡡��  : pipaddrDest    Ŀ���ַ
** �䡡��  : �����ѯ��, �򷵻�·�ɱ�ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_RT_ENTRY __rtMatch (const ip4_addr_t *pipaddrDest)
{
    INT             iHash = LW_RT_HASHINDEX(pipaddrDest);
    PLW_LIST_LINE   plineTemp;
    PLW_RT_ENTRY    prte;
    PLW_RT_ENTRY    prteNet = LW_NULL;                                  /*  ��������ǲ����������ַ    */
    
    for (plineTemp  = _G_plineRtHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        prte = (PLW_RT_ENTRY)plineTemp;
        if (prte->RTE_uiFlag & LW_RT_FLAG_U) {                          /*  ·�ɽڵ���Ч                */
            if ((prte->RTE_uiFlag & LW_RT_FLAG_H) &&
                (prte->RTE_ipaddrDest.addr == pipaddrDest->addr)) {     /*  ����ƥ����                */
                return  (prte);
            
            } else if ((prteNet == LW_NULL) &&                          /*  ����ƥ����                */
                       (ip4_addr_netcmp(pipaddrDest, 
                                        netif_ip4_addr(prte->RTE_pnetif),   
                                        netif_ip4_netmask(prte->RTE_pnetif)))) {
                prteNet = prte;
            }
        }
    }
    
    return  (prteNet);
}
/*********************************************************************************************************
** ��������: __rtFind
** ��������: ����һ�� sylixos ·����Ŀ
** �䡡��  : pipaddrDest   Ŀ���ַ
**           ulFlag        ��Ҫ����ı�־
** �䡡��  : �����ѯ��, �򷵻�·�ɱ�ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_RT_ENTRY __rtFind (ip4_addr_t *pipaddrDest, UINT  ulFlag)
{
    INT             iHash = LW_RT_HASHINDEX(pipaddrDest);
    PLW_LIST_LINE   plineTemp;
    PLW_RT_ENTRY    prte;
    
    for (plineTemp  = _G_plineRtHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        prte = (PLW_RT_ENTRY)plineTemp;
        if ((prte->RTE_ipaddrDest.addr == pipaddrDest->addr) &&
            ((prte->RTE_uiFlag & ulFlag) == ulFlag)) {                  /*  ��������ֱ�ӷ���            */
            return  (prte);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __rtFind
** ��������: ���� sylixos ·����Ŀ
** �䡡��  : pfuncHook     �����ص�����
**           pvArg0...4    ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __rtTraversal (VOIDFUNCPTR  pfuncHook,
                           PVOID        pvArg0,
                           PVOID        pvArg1,
                           PVOID        pvArg2,
                           PVOID        pvArg3,
                           PVOID        pvArg4)
{
    INT             i;
    PLW_LIST_LINE   plineTemp;
    PLW_RT_ENTRY    prte;
    
    for (i = 0; i < LW_RT_TABLESIZE; i++) {
        for (plineTemp  = _G_plineRtHashHeader[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            prte = (PLW_RT_ENTRY)plineTemp;
            if (prte->RTE_pnetif) {
                prte->RTE_uiFlag |= LW_RT_FLAG_U;                       /*  ·����Ч                    */
                if ((prte->RTE_ipaddrDest.addr != IPADDR_BROADCAST) &&
                    !ip4_addr_netcmp(&prte->RTE_ipaddrDest,
                                     netif_ip4_addr(prte->RTE_pnetif),
                                     netif_ip4_netmask(prte->RTE_pnetif))) {
                    prte->RTE_uiFlag |= LW_RT_FLAG_G;                   /*  ���·��                    */
                } else {
                    prte->RTE_uiFlag &= ~LW_RT_FLAG_G;
                }
            }
            pfuncHook(prte, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4);
        }
    }
}
/*********************************************************************************************************
** ��������: __rtFind
** ��������: ���� sylixos ·����Ŀ
** �䡡��  : pfuncHook     �����ص�����
**           pvArg0...4    ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __rtBuildinTraversal (VOIDFUNCPTR  pfuncHook,
                                  PVOID        pvArg0,
                                  PVOID        pvArg1,
                                  PVOID        pvArg2,
                                  PVOID        pvArg3,
                                  PVOID        pvArg4)
{
    struct netif  *netif;
    LW_RT_ENTRY    rte;
    
    for (netif = netif_list; netif != NULL; netif = netif->next) {
        rte.RTE_ipaddrDest.addr = netif_ip4_addr(netif)->addr;
        rte.RTE_ipaddrGw.addr   = IPADDR_ANY;
        rte.RTE_pnetif          = netif;
        rte.RTE_cNetifName[0]   = netif->name[0];
        rte.RTE_cNetifName[1]   = netif->name[1];
        rte.RTE_cNetifName[2]   = (char)(netif->num + '0');
        rte.RTE_cNetifName[3]   = PX_EOS;
        
        if (netif_is_up(netif) && netif_is_link_up(netif)) {
            rte.RTE_uiFlag = LW_RT_FLAG_U | LW_RT_FLAG_H;
        
        } else {
            rte.RTE_uiFlag = LW_RT_FLAG_H;
        }
        
        pfuncHook(&rte, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4);
        
        if ((netif->flags & NETIF_FLAG_BROADCAST) == 0) {               /*  PPP / SLIP ����             */
            rte.RTE_ipaddrDest.addr = netif_ip4_gw(netif)->addr;
            
        } else {                                                        /*  ��ͨ����                    */
            rte.RTE_ipaddrDest.addr = netif_ip4_addr(netif)->addr & netif_ip4_netmask(netif)->addr;
            rte.RTE_uiFlag &= ~LW_RT_FLAG_H;
        }
        
        pfuncHook(&rte, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4);
    }
    
    if (netif_default) {                                                /*  Ĭ�ϳ���                    */
        rte.RTE_ipaddrDest.addr = IPADDR_ANY;                           /*  Ŀ�ĵ�ַΪ 0                */
        rte.RTE_ipaddrGw.addr   = IPADDR_ANY;
        rte.RTE_pnetif          = netif_default;
        rte.RTE_cNetifName[0]   = netif_default->name[0];
        rte.RTE_cNetifName[1]   = netif_default->name[1];
        rte.RTE_cNetifName[2]   = (char)(netif_default->num + '0');
        rte.RTE_cNetifName[3]   = PX_EOS;
        
        if (netif_is_up(netif_default) && netif_is_link_up(netif_default)) {
            rte.RTE_uiFlag = LW_RT_FLAG_U | LW_RT_FLAG_G;
        
        } else {
            rte.RTE_uiFlag = LW_RT_FLAG_G;
        }
        
        pfuncHook(&rte, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4);
    }
}
/*********************************************************************************************************
** ��������: __rtAddCallback
** ��������: ���һ�� sylixos ·����Ŀ(��������, �� TCPIP ��������ִ��)
** �䡡��  : ipaddrDest    Ŀ�ĵ�ַ
**           pipaddrGw     ����
**           uiFlag        flag
**           pcNetifName   ·���豸�ӿ���
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtAddCallback (ip4_addr_t *pipaddrDest, 
                            ip4_addr_t *pipaddrGw, 
                            UINT        uiFlag, 
                            CPCHAR      pcNetifName)
{
    INT             iHash = LW_RT_HASHINDEX(pipaddrDest);
    PLW_RT_ENTRY    prte;
    
    prte = (PLW_RT_ENTRY)__SHEAP_ALLOC(sizeof(LW_RT_ENTRY));
    if (prte == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    prte->RTE_ipaddrDest = *pipaddrDest;
    prte->RTE_ipaddrGw   = *pipaddrGw;
    prte->RTE_pnetif     = netif_find((PCHAR)pcNetifName);
    prte->RTE_uiFlag     = uiFlag;
    lib_strlcpy(prte->RTE_cNetifName, pcNetifName, IF_NAMESIZE);
    if (prte->RTE_pnetif) {
        prte->RTE_uiFlag |= LW_RT_FLAG_U;                               /*  ·����Ч                    */
        _G_uiActiveNum++;
    }
    _G_uiTotalNum++;
    
    _List_Line_Add_Ahead(&prte->RTE_lineManage, &_G_plineRtHashHeader[iHash]);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtDelCallback
** ��������: ɾ��һ�� sylixos ·����Ŀ(���������Ƴ�, �� TCPIP ��������ִ��)
** �䡡��  : pipaddrDest    Ŀ���ַ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtDelCallback (ip4_addr_t *pipaddrDest)
{
    PLW_RT_ENTRY    prte;
    
    prte = __rtFind(pipaddrDest, 0);
    if (prte) {
        INT     iHash = LW_RT_HASHINDEX(&prte->RTE_ipaddrDest);
        
        if (prte->RTE_uiFlag & LW_RT_FLAG_U) {
            _G_uiActiveNum--;
        }
        _G_uiTotalNum--;
        
        LW_RT_CACHE_INVAL(prte);
        if (prte->RTE_pnetif) {
            LW_GW_CACHE_INVAL(prte->RTE_pnetif->num, prte);
        }
        
        _List_Line_Del(&prte->RTE_lineManage, &_G_plineRtHashHeader[iHash]);
        
        __SHEAP_FREE(prte);
        
        return  (ERROR_NONE);
    }
    
    _ErrorHandle(EINVAL);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __rtChangeCallback
** ��������: �޸�һ�� sylixos ·����Ŀ(�� TCPIP ��������ִ��)
** �䡡��  : ipaddrDest    Ŀ�ĵ�ַ
**           pipaddrGw     ����
**           uiFlag        flag
**           pcNetifName   ·���豸�ӿ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtChangeCallback (ip4_addr_t *pipaddrDest, 
                               ip4_addr_t *pipaddrGw, 
                               UINT        uiFlag, 
                               CPCHAR      pcNetifName)
{
    PLW_RT_ENTRY    prte;
    
    prte = __rtFind(pipaddrDest, 0);
    if (prte) {
        INT     iHash = LW_RT_HASHINDEX(&prte->RTE_ipaddrDest);
        
        if (prte->RTE_uiFlag & LW_RT_FLAG_U) {
            _G_uiActiveNum--;
        }
        _G_uiTotalNum--;
        
        LW_RT_CACHE_INVAL(prte);
        if (prte->RTE_pnetif) {
            LW_GW_CACHE_INVAL(prte->RTE_pnetif->num, prte);
        }
        
        _List_Line_Del(&prte->RTE_lineManage, &_G_plineRtHashHeader[iHash]);
        
        prte->RTE_ipaddrDest = *pipaddrDest;
        prte->RTE_ipaddrGw   = *pipaddrGw;
        prte->RTE_pnetif     = netif_find((PCHAR)pcNetifName);
        prte->RTE_uiFlag     = uiFlag;
        lib_strlcpy(prte->RTE_cNetifName, pcNetifName, IF_NAMESIZE);
        if (prte->RTE_pnetif) {
            prte->RTE_uiFlag |= LW_RT_FLAG_U;                           /*  ·����Ч                    */
            _G_uiActiveNum++;
        }
        _G_uiTotalNum++;
        
        iHash = LW_RT_HASHINDEX(&prte->RTE_ipaddrDest);
        
        _List_Line_Add_Ahead(&prte->RTE_lineManage, &_G_plineRtHashHeader[iHash]);
        
        return  (ERROR_NONE);
    }
    
    _ErrorHandle(EINVAL);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __rtSetGwCallback
** ��������: ��������ӿڵ�����(�� TCPIP ��������ִ��)
** �䡡��  : pipaddrGw     ���ص�ַ
**           uiGwFlags     ������������
**           pcNetifName   ����ӿ���
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtSetGwCallback (ip4_addr_t *pipaddrGw, UINT  uiGwFlags, CPCHAR  pcNetifName)
{
#define LW_RT_GW_FLAG_SET       0x1
#define LW_RT_GW_FLAG_DEFAULT   0x2

    struct netif  *pnetif = netif_find((PCHAR)pcNetifName);
    
    if (pnetif) {
        if (uiGwFlags & LW_RT_GW_FLAG_SET) {
            netif_set_gw(pnetif, pipaddrGw);
        }
        if (uiGwFlags & LW_RT_GW_FLAG_DEFAULT) {
            netif_set_default(pnetif);
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __rtGetCallback
** ��������: ��ȡ·�ɱ�ӿ�(�� TCPIP ��������ִ��)
** �䡡��  : prte          ·����Ŀ
**           flag          ��ȡ���������� 
**           msgbuf        �����
**           num           ���������
**           piIndex       ��ǰ�±�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __rtGetCallback (PLW_RT_ENTRY       prte, 
                             u_char             flag, 
                             struct route_msg  *msgbuf, 
                             size_t             num, 
                             int               *piIndex)
{
    struct  netif  *pnetif = prte->RTE_pnetif;
    u_char  ucGetFlag = 0;

    if (*piIndex >= num) {
        return;
    }
    
    if (prte->RTE_uiFlag & LW_RT_FLAG_U) {                              /*  ��Ч·��                    */
        ucGetFlag |= ROUTE_RTF_UP;
    }
    if (prte->RTE_uiFlag & LW_RT_FLAG_H) {
        ucGetFlag |= ROUTE_RTF_HOST;                                    /*  ����·��                    */
    }
    if (prte->RTE_uiFlag & LW_RT_FLAG_G) {
        ucGetFlag |= ROUTE_RTF_GATEWAY;                                 /*  Ŀ����һ�����ص�·��        */
    }
    
    if ((ucGetFlag & flag) == flag) {                                   /*  �����ȡ����                */
        msgbuf[*piIndex].rm_flag        = ucGetFlag;
        msgbuf[*piIndex].rm_metric      = 1;
        msgbuf[*piIndex].rm_dst.s_addr  = prte->RTE_ipaddrDest.addr;    /*  Ŀ�ĵ�ַ                    */
        if (pnetif) {
            if (prte->RTE_ipaddrGw.addr != IPADDR_ANY) {
                msgbuf[*piIndex].rm_gw.s_addr = prte->RTE_ipaddrGw.addr;
            } else {                                                    /*  ���ص�ַ                    */
                msgbuf[*piIndex].rm_gw.s_addr = netif_ip4_gw(pnetif)->addr;
            }                                                           /*  ��������                    */
            msgbuf[*piIndex].rm_mask.s_addr  = netif_ip4_netmask(pnetif)->addr;
            msgbuf[*piIndex].rm_if.s_addr    = netif_ip4_addr(pnetif)->addr;
                                                                        /*  ����ӿ� IP                 */
        } else {
            msgbuf[*piIndex].rm_gw.s_addr   = IPADDR_ANY;
            msgbuf[*piIndex].rm_mask.s_addr = IPADDR_ANY;
            msgbuf[*piIndex].rm_if.s_addr   = IPADDR_ANY;
        }
        lib_memcpy(msgbuf[*piIndex].rm_ifname, 
                   prte->RTE_cNetifName, IF_NAMESIZE);                  /*  ��������                    */
        (*piIndex)++;                                                   /*  ���������ƶ�                */
    }
}
/*********************************************************************************************************
** ��������: __rtAdd
** ��������: ���һ�� sylixos ·����Ŀ
** �䡡��  : ipaddrDest    Ŀ�ĵ�ַ
**           pipaddrGw     ���ص�ַ
**           uiFlag        flag
**           pcNetifName   ·���豸�ӿ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtAdd (ip4_addr_t *pipaddrDest, ip4_addr_t *pipaddrGw, UINT  uiFlag, CPCHAR  pcNetifName)
{
    INT     iError;
    
    if (pipaddrDest == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pipaddrDest->addr == IPADDR_ANY) {                              /*  ��������Ĭ������            */
        iError = __rtSafeRun(__rtSetGwCallback, pipaddrGw, (PVOID)LW_RT_GW_FLAG_SET, 
                             (PVOID)pcNetifName, 0, 0, 0);
    
    } else {                                                            /*  ����·����Ϣ                */
        iError = __rtSafeRun(__rtAddCallback, pipaddrDest, pipaddrGw, 
                             (PVOID)uiFlag, (PVOID)pcNetifName, 0, 0);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __rtDelete
** ��������: �Ƴ�һ�� sylixos ·����Ŀ
** �䡡��  : ipaddrDest    Ŀ���ַ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtDel (ip4_addr_t *pipaddrDest)
{
    INT     iError;

    if (pipaddrDest == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iError = __rtSafeRun(__rtDelCallback, pipaddrDest, 0, 0, 0, 0, 0);

    return  (iError);
}
/*********************************************************************************************************
** ��������: __rtChange
** ��������: �ı�һ�� sylixos ·����Ŀ
** �䡡��  : ipaddrDest    Ŀ�ĵ�ַ
**           pipaddrGw     ���ص�ַ
**           uiFlag        flag
**           pcNetifName   ·���豸�ӿ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtChange (ip4_addr_t *pipaddrDest, ip4_addr_t *pipaddrGw, UINT  uiFlag, CPCHAR  pcNetifName)
{
    INT     iError;
    
    if (pipaddrDest == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pipaddrDest->addr == IPADDR_ANY) {                              /*  ��������Ĭ������            */
        iError = __rtSafeRun(__rtSetGwCallback, pipaddrGw, (PVOID)LW_RT_GW_FLAG_SET, 
                             (PVOID)pcNetifName, 0, 0, 0);
    
    } else {                                                            /*  ����·����Ϣ                */
        iError = __rtSafeRun(__rtChangeCallback, pipaddrDest, pipaddrGw, 
                             (PVOID)uiFlag, (PVOID)pcNetifName, 0, 0);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __rtSetDefautNetif
** ��������: ����һ��Ĭ���������
** �䡡��  : pcNetifName   �豸�ӿ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtSetDefautNetif (CPCHAR  pcNetifName)
{
    INT     iError;
    
    if (pcNetifName == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iError = __rtSafeRun(__rtSetGwCallback, LW_NULL, (PVOID)LW_RT_GW_FLAG_DEFAULT, 
                         (PVOID)pcNetifName, 0, 0, 0);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __rtGet
** ��������: ��ȡ·�ɱ�
** �䡡��  : flag          ��ȡ���������� 
**           msgbuf        ·�ɱ�����
**           num           ��������
** �䡡��  : ��ȡ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __rtGet (u_char  flag, struct route_msg  *msgbuf, size_t  num)
{
    INT     iNum = 0;
    
    __rtSafeRun((FUNCPTR)__rtTraversal, (void *)__rtGetCallback,
                (PVOID)(ULONG)flag, (PVOID)msgbuf, (PVOID)num, (PVOID)&iNum, 0);
                
    __rtSafeRun((FUNCPTR)__rtBuildinTraversal, (void *)__rtGetCallback,
                (PVOID)(ULONG)flag, (PVOID)msgbuf, (PVOID)num, (PVOID)&iNum, 0);

    return  (iNum);
}
/*********************************************************************************************************
** ��������: __rtNeifAddCallback
** ��������: �������ӿڻص�(���簲ȫ)
** �䡡��  : netif     ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __rtNeifAddCallback (struct netif *netif)
{
    INT             i;
    PLW_LIST_LINE   plineTemp;
    PLW_RT_ENTRY    prte;
    CHAR            cNetifName[IF_NAMESIZE];

    if_indextoname(netif->num, cNetifName);

    for (i = 0; i < LW_RT_TABLESIZE; i++) {
        for (plineTemp  = _G_plineRtHashHeader[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            prte = (PLW_RT_ENTRY)plineTemp;
            if (lib_strncmp(cNetifName, prte->RTE_cNetifName, IF_NAMESIZE) == 0) {
                prte->RTE_pnetif  = netif;                              /*  ��Ӧ��·����Ŀ��Ч          */
                prte->RTE_uiFlag |= LW_RT_FLAG_U;
                _G_uiActiveNum++;
            }
        }
    }
}
/*********************************************************************************************************
** ��������: __rtNeifRemoveCallback
** ��������: �Ƴ�����ӿڻص�(���簲ȫ)
** �䡡��  : netif     ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __rtNeifRemoveCallback (struct netif *netif)
{
    INT             i;
    PLW_LIST_LINE   plineTemp;
    PLW_RT_ENTRY    prte;

    for (i = 0; i < LW_RT_TABLESIZE; i++) {
        for (plineTemp  = _G_plineRtHashHeader[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            prte = (PLW_RT_ENTRY)plineTemp;
            if (prte->RTE_pnetif == netif) {                            /*  ��Ӧ��·����ĿӦ����Ч      */
                prte->RTE_pnetif  = LW_NULL;
                prte->RTE_uiFlag &= ~LW_RT_FLAG_U;

                LW_RT_CACHE_INVAL(prte);                                /*  ��Ч��Ӧ cache              */
                LW_GW_CACHE_INVAL(netif->num, prte);
                _G_uiActiveNum--;
            }
        }
    }
}
/*********************************************************************************************************
** ��������: __rtNeifAddHook
** ��������: �������ӿڻص�
** �䡡��  : netif     ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID rt_netif_add_hook (struct netif *netif)
{
    __rtSafeRun((FUNCPTR)__rtNeifAddCallback, (PVOID)netif, 0, 0, 0, 0, 0);
}
/*********************************************************************************************************
** ��������: __rtNeifRemoveHook
** ��������: �Ƴ�����ӿڻص�
** �䡡��  : netif     ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID rt_netif_remove_hook (struct netif *netif)
{
    __rtSafeRun((FUNCPTR)__rtNeifRemoveCallback, (PVOID)netif, 0, 0, 0, 0, 0);
}
/*********************************************************************************************************
** ��������: sys_ip_route_hook
** ��������: sylixos ·�ɲ��ҽӿ�
** �䡡��  : ipaddrDest    Ŀ���ַ
** �䡡��  : �����ѯ��, �򷵻�·�ɽӿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
struct netif *sys_ip_route_hook (const ip4_addr_t *pipaddrDest)
{
    PLW_RT_ENTRY    prte;

    if (_G_uiActiveNum == 0) {
        return  (LW_NULL);
    }
    
    if ((_G_rtcache.RTCACHE_prteCache) &&
        (_G_rtcache.RTCACHE_prteCache->RTE_uiFlag & LW_RT_FLAG_U) &&
        (_G_rtcache.RTCACHE_ipaddrDest.addr == pipaddrDest->addr)) {    /*  �����ж� CACHE �Ƿ�����     */
        return  (_G_rtcache.RTCACHE_prteCache->RTE_pnetif);
    }
    
    prte = __rtMatch(pipaddrDest);                                      /*  Ѱ����ЧĿ��·��            */
    if (prte) {
        _G_rtcache.RTCACHE_prteCache = prte;
        _G_rtcache.RTCACHE_ipaddrDest.addr = pipaddrDest->addr;
        return  (prte->RTE_pnetif);
    }
    
    return  (LW_NULL);                                                  /*  ʹ�� lwip �ڽ�·��          */
}
/*********************************************************************************************************
** ��������: sys_ip_gw_hook
** ��������: sylixos ·�ɲ��ҽӿ� (��������)
** �䡡��  : netif         ����ӿ�
**           ipaddrDest    Ŀ���ַ
** �䡡��  : ���ؽӿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ip4_addr_t *sys_ip_gw_hook (struct netif *netif, const ip4_addr_t *pipaddrDest)
{
    PLW_RT_ENTRY    prte;

    if (_G_uiActiveNum == 0) {
        return  (LW_NULL);
    }
                                                                        /*  �����ж� CACHE �Ƿ�����     */
    if ((_G_gwcache[netif->num].GWCACHE_prteCache) &&
        (_G_gwcache[netif->num].GWCACHE_prteCache->RTE_uiFlag & LW_RT_FLAG_U) &&
        (_G_gwcache[netif->num].GWCACHE_ipaddrDest.addr == pipaddrDest->addr)) {    
        if ((_G_gwcache[netif->num].GWCACHE_prteCache)->RTE_ipaddrGw.addr != IPADDR_ANY) {
            return  (&(_G_gwcache[netif->num].GWCACHE_prteCache)->RTE_ipaddrGw);
        
        } else {
            return  (LW_NULL);
        }
    }
    
    prte = __rtMatch(pipaddrDest);                                      /*  Ѱ����ЧĿ��·��            */
    if (prte) {
        _G_gwcache[netif->num].GWCACHE_prteCache = prte;
        _G_gwcache[netif->num].GWCACHE_ipaddrDest.addr = pipaddrDest->addr;
        if (prte->RTE_ipaddrGw.addr != IPADDR_ANY) {
            return  (&prte->RTE_ipaddrGw);
        
        } else {
            return  (LW_NULL);
        }
    }
    
    return  (LW_NULL);
}

/*********************************************************************************************************
** ��������: __rtEntryPrint
** ��������: ��ӡһ��·����Ϣ
** �䡡��  : prte      ·�ɱ�ڵ�
**           pcBuffer  ������
**           stSize    ��������С
**           pstOffset ƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static VOID __rtEntryPrint (PLW_RT_ENTRY prte, PCHAR  pcBuffer, size_t  stSize, size_t *pstOffset)
{
    CHAR    cIpDest[INET_ADDRSTRLEN];
    CHAR    cGateway[INET_ADDRSTRLEN] = "*";
    CHAR    cMask[INET_ADDRSTRLEN]    = "*";
    CHAR    cFlag[6] = "\0";
    
    ip4addr_ntoa_r(&prte->RTE_ipaddrDest, cIpDest, INET_ADDRSTRLEN);
    if (prte->RTE_pnetif) {
        if (prte->RTE_ipaddrGw.addr != IPADDR_ANY) {
            ip4addr_ntoa_r(&prte->RTE_ipaddrGw, cGateway, INET_ADDRSTRLEN);
        } else {
            ip4addr_ntoa_r(netif_ip4_gw(prte->RTE_pnetif), cGateway, INET_ADDRSTRLEN);
        }
        ip4addr_ntoa_r(netif_ip4_netmask(prte->RTE_pnetif), cMask, INET_ADDRSTRLEN);
    }
    
    if (prte->RTE_uiFlag & LW_RT_FLAG_U) {
        lib_strcat(cFlag, "U");
    }
    if (prte->RTE_uiFlag & LW_RT_FLAG_G) {
        lib_strcat(cFlag, "G");
    } else {
        lib_strcpy(cGateway, "*");
    }
    if (prte->RTE_uiFlag & LW_RT_FLAG_H) {
        lib_strcat(cFlag, "H");
    }
    if (prte->RTE_uiFlag & LW_RT_FLAG_D) {
        lib_strcat(cFlag, "D");
    }
    if (prte->RTE_uiFlag & LW_RT_FLAG_M) {
        lib_strcat(cFlag, "M");
    }
    
    *pstOffset = bnprintf(pcBuffer, stSize, *pstOffset,
                          "%-18s %-18s %-18s %-8s %-3s\n",
                          cIpDest, cGateway, cMask, cFlag, prte->RTE_cNetifName);
}
/*********************************************************************************************************
** ��������: __aodvEntryPrint
** ��������: ��ӡ aodv ·����Ϣ
** �䡡��  : rt        aodv ·����Ŀ
**           pcBuffer  ������
**           stSize    ��������С
**           pstOffset ƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __aodvEntryPrint (struct aodv_rtnode *rt, PCHAR  pcBuffer, size_t  stSize, size_t *pstOffset)
{
    CHAR    cIpDest[INET_ADDRSTRLEN];
    CHAR    cNextHop[INET_ADDRSTRLEN];
    CHAR    cMask[INET_ADDRSTRLEN];
    CHAR    cFlag[16] = "\0";
    CHAR    cIfName[IF_NAMESIZE] = "\0";
    
    inet_ntoa_r(rt->dest_addr, cIpDest, INET_ADDRSTRLEN);
    inet_ntoa_r(rt->next_hop, cNextHop, INET_ADDRSTRLEN);
    
    if (rt->state & AODV_VALID) {
        lib_strcat(cFlag, "U");
    }
    if (rt->hcnt > 0) {
        lib_strcat(cFlag, "G");
    }
    if ((rt->flags & AODV_RT_GATEWAY) == 0) {
        lib_strcat(cFlag, "H");
    }
    if ((rt->flags & AODV_RT_UNIDIR) == 0) {                            /*  ��������                    */
        lib_strcat(cFlag, "-ud");
    }
    
    /*
     *  aodv ·�ɽڵ�����ӿ�һ����Ч
     */
    ip4addr_ntoa_r(netif_ip4_netmask(rt->netif), cMask, INET_ADDRSTRLEN);
    if_indextoname(rt->netif->num, cIfName);
    
    *pstOffset = bnprintf(pcBuffer, stSize, *pstOffset,
                          "%-18s %-18s %-18s %-8s %8d %-3s\n",
                          cIpDest, cNextHop, cMask, cFlag, rt->hcnt, cIfName);
}
/*********************************************************************************************************
** ��������: __buildinRtPrint
** ��������: ��ӡ lwip �ڽ�·����Ϣ
** �䡡��  : rt            aodv ·����Ŀ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __buildinRtPrint (PCHAR  pcBuffer, size_t  stSize, size_t *pstOffset)
{
    struct netif *netif;
    ip4_addr_t    ipaddr;
    
    CHAR    cIpDest[INET_ADDRSTRLEN];
    CHAR    cGateway[INET_ADDRSTRLEN] = "*";
    CHAR    cMask[INET_ADDRSTRLEN];
    CHAR    cFlag[6];
    CHAR    cIfName[IF_NAMESIZE] = "\0";
    
    for (netif = netif_list; netif != NULL; netif = netif->next) {
        if ((netif->flags & NETIF_FLAG_BROADCAST) == 0) {               /*  PPP / SLIP ����             */
            ipaddr.addr = netif_ip4_gw(netif)->addr;
            ip4addr_ntoa_r(&ipaddr, cIpDest, INET_ADDRSTRLEN);
            ip4addr_ntoa_r(netif_ip4_netmask(netif), cMask, INET_ADDRSTRLEN);
            if_indextoname(netif->num, cIfName);
            
            if (netif_is_up(netif) && netif_is_link_up(netif)) {
                lib_strcpy(cFlag, "UH");
            
            } else {
                lib_strcpy(cFlag, "H");
            }
            
        } else {                                                        /*  ��ͨ����                    */
            ipaddr.addr = netif_ip4_addr(netif)->addr & netif_ip4_netmask(netif)->addr;
            ip4addr_ntoa_r(&ipaddr, cIpDest, INET_ADDRSTRLEN);
            ip4addr_ntoa_r(netif_ip4_netmask(netif), cMask, INET_ADDRSTRLEN);
            if_indextoname(netif->num, cIfName);
            
            if (netif_is_up(netif) && netif_is_link_up(netif)) {
                lib_strcpy(cFlag, "U");
            
            } else {
                cFlag[0] = PX_EOS;
            }
        }
        
        *pstOffset = bnprintf(pcBuffer, stSize, *pstOffset,
                              "%-18s %-18s %-18s %-8s %-3s\n",
                              cIpDest, cGateway, cMask, cFlag, cIfName);

        ip4addr_ntoa_r(netif_ip4_addr(netif), cIpDest, INET_ADDRSTRLEN);
        
        if (netif_is_up(netif) && netif_is_link_up(netif)) {
            lib_strcpy(cFlag, "UH");
        
        } else {
            lib_strcpy(cFlag, "H");
        }
        
        *pstOffset = bnprintf(pcBuffer, stSize, *pstOffset,
                              "%-18s %-18s %-18s %-8s %-3s\n",
                              cIpDest, cGateway, cMask, cFlag, cIfName);
    }
    
    if (netif_default) {
        ip4addr_ntoa_r(netif_ip4_gw(netif_default), cGateway, INET_ADDRSTRLEN);
        ip4addr_ntoa_r(netif_ip4_netmask(netif_default), cMask, INET_ADDRSTRLEN);
        if_indextoname(netif_default->num, cIfName);
        
        if (netif_is_up(netif_default) && netif_is_link_up(netif_default)) {
            lib_strcpy(cFlag, "UG");
            
        } else {
            lib_strcpy(cFlag, "G");
        }

        *pstOffset = bnprintf(pcBuffer, stSize, *pstOffset,
                              "%-18s %-18s %-18s %-8s %-3s\n",
                              "default", cGateway, cMask, cFlag, cIfName);
    }
}
/*********************************************************************************************************
** ��������: __rtTotalEntry
** ��������: �������·��(���� kernel, aodv, build-in)���, ����һ�����
** �䡡��  : NONE
** �䡡��  : ����һ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT __rtEntryMaxNum (VOID)
{
    UINT    uiMax;

    /*
     *  total entry include : kernel + aodv + (netifnum * 2) + default
     */
    uiMax = _G_uiTotalNum;                                              /*  kernel route entry          */

    if (uiMax < AODV_RT_NUM_ENTRIES()) {
        uiMax = AODV_RT_NUM_ENTRIES();                                  /*  aodv route entry            */
    }

    if (uiMax < (netif_get_num() * 2) + 1) {                            /*  lwip build-in route entry   */
        uiMax = (netif_get_num() * 2) + 1;
    }

    return  (uiMax);
}
/*********************************************************************************************************
** ��������: __tshellRoute
** ��������: ϵͳ���� "route"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellRoute (INT  iArgC, PCHAR  *ppcArgV)
{
#define LW_RT_PRINT_SIZE        74

    INT         iError;
    FUNCPTR     pfuncAddOrChange;
    PCHAR       pcOpAddorChange;

    UINT        uiFlag = 0;
    ip4_addr_t  ipaddr;
    ip4_addr_t  ipaddrGw;
    CHAR        cNetifName[IF_NAMESIZE];

    if (iArgC == 1) {
        PCHAR   pcBuffer;
        size_t  stSize;
        size_t  stOffset;
        UINT    uiNumEntries;

        uiNumEntries = (UINT)__rtSafeRun((FUNCPTR)__rtEntryMaxNum, 0, 0, 0, 0, 0, 0);
        if (uiNumEntries == 0) {
            printf("no route entry.\n");
            return  (ERROR_NONE);
        }

        /*
         *  �� net safe ״̬�²�������� printf ��ʹ�� IO �����. ����ֻ�ܴ�ӡ����������,
         *  Ȼ��ͳһʹ�� IO ��ӡ.
         */
        stSize   = LW_RT_PRINT_SIZE * uiNumEntries;
        pcBuffer = (PCHAR)__SHEAP_ALLOC(stSize);
        if (pcBuffer == LW_NULL) {
            fprintf(stderr, "system low memory.\n");
            return  (PX_ERROR);
        }

        /*
         *  ��ӡ kernel route entry
         */
        stOffset = 0;
        printf("kernel routing tables\n");
        printf("Destination        Gateway            Mask               Flag     Interface\n");
        __rtSafeRun((FUNCPTR)__rtTraversal, (void *)__rtEntryPrint,
                    pcBuffer, (PVOID)stSize, &stOffset, 0, 0);
        if (stOffset > 0) {
            fwrite(pcBuffer, stOffset, 1, stdout);
            fflush(stdout);                                             /*  �������ȷ��������        */
        }
        
        /*
         *  ��ӡ lwip build-in route entry
         */
        stOffset = 0;
        printf("\nbuild-in routing tables\n");
        printf("Destination        Gateway            Mask               Flag     Interface\n");
        __rtSafeRun((FUNCPTR)__buildinRtPrint,
                    pcBuffer, (PVOID)stSize, &stOffset, 0, 0, 0);
        if (stOffset > 0) {
            fwrite(pcBuffer, stOffset, 1, stdout);
            fflush(stdout);                                             /*  �������ȷ��������        */
        }

        __SHEAP_FREE(pcBuffer);

        return  (ERROR_NONE);
        
    } else {                                                            /*  ����·�ɱ�                  */
        if (lib_strcmp(ppcArgV[1], "add") == 0) {
            pfuncAddOrChange = __rtAdd;
            pcOpAddorChange  = "add";

        } else if (lib_strcmp(ppcArgV[1], "change") == 0) {
            pfuncAddOrChange = __rtChange;
            pcOpAddorChange  = "change";

        } else {
            pfuncAddOrChange = LW_NULL;
        }

        if (pfuncAddOrChange && (iArgC == 7)) {                         /*  ��ӻ����޸�·�ɱ�          */
            if (!ip4addr_aton(ppcArgV[3], &ipaddr)) {
                fprintf(stderr, "inet address format error.\n");
                goto    __error_handle;
            }
            if (!ip4addr_aton(ppcArgV[4], &ipaddrGw)) {
                fprintf(stderr, "gw address format error.\n");
                goto    __error_handle;
            }

            if (lib_strcmp(ppcArgV[5], "if") == 0) {                    /*  ʹ�� ifindex ��ѯ����       */
                INT   iIndex = lib_atoi(ppcArgV[6]);
                if (if_indextoname(iIndex, cNetifName) == LW_NULL) {
                    fprintf(stderr, "can not find net interface with ifindex %d.\n", iIndex);
                    goto    __error_handle;
                }

            } else if (lib_strcmp(ppcArgV[5], "dev") == 0) {
                lib_strlcpy(cNetifName, ppcArgV[6], IF_NAMESIZE);

            } else {
                fprintf(stderr, "net interface argument error.\n");
                goto    __error_handle;
            }

            if (lib_strcmp(ppcArgV[2], "-host") == 0) {                 /*  ����·��                    */
                uiFlag |= LW_RT_FLAG_H;

            } else if (lib_strcmp(ppcArgV[2], "-net") != 0) {           /*  ������·��                  */
                fprintf(stderr, "route add must determine -host or -net.\n");
                goto    __error_handle;
            }

            iError = pfuncAddOrChange(&ipaddr, &ipaddrGw, uiFlag, cNetifName);
            if (iError >= 0) {                                          /*  ����·�ɱ�                  */
                printf("route %s %s successful.\n", ppcArgV[3], pcOpAddorChange);
                return  (ERROR_NONE);
            }
        
        } else if (pfuncAddOrChange && (iArgC == 5)) {                  /*  ����Ĭ������ӿ�            */
            if (lib_strcmp(ppcArgV[2], "default") != 0) {
                goto    __error_handle;
            }
            
            if (lib_strcmp(ppcArgV[3], "if") == 0) {                    /*  ʹ�� ifindex ��ѯ����       */
                INT   iIndex = lib_atoi(ppcArgV[4]);
                if (if_indextoname(iIndex, cNetifName) == LW_NULL) {
                    fprintf(stderr, "can not find net interface with ifindex %d.\n", iIndex);
                    goto    __error_handle;
                }

            } else if (lib_strcmp(ppcArgV[3], "dev") == 0) {
                lib_strlcpy(cNetifName, ppcArgV[4], IF_NAMESIZE);

            } else {
                fprintf(stderr, "net interface argument error.\n");
                goto    __error_handle;
            }
            
            uiFlag |= LW_RT_GW_FLAG_DEFAULT;
            pfuncAddOrChange = __rtSetDefautNetif;
            
            iError = pfuncAddOrChange(cNetifName);                      /*  ����Ĭ���������            */
            if (iError >= 0) {
                printf("default device set successful.\n");
                return  (ERROR_NONE);
            }

        } else if ((lib_strcmp(ppcArgV[1], "del") == 0) && (iArgC == 3)) {
            if (!ip4addr_aton(ppcArgV[2], &ipaddr)) {                   /*  ɾ��һ��·�ɱ���            */
                fprintf(stderr, "inet address format error.\n");
                goto    __error_handle;
            }

            iError = __rtDel(&ipaddr);
            if (iError >= 0) {
                printf("route %s delete successful.\n", ppcArgV[2]);
                return  (ERROR_NONE);
            }
        }
    }
    
__error_handle:
    fprintf(stderr, "argments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** ��������: __tshellAodvs
** ��������: ϵͳ���� "aodvs"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellAodvs (INT  iArgC, PCHAR  *ppcArgV)
{
    PCHAR   pcBuffer;
    size_t  stSize;
    size_t  stOffset;
    UINT    uiNumEntries;

    uiNumEntries = (UINT)__rtSafeRun((FUNCPTR)__rtEntryMaxNum, 0, 0, 0, 0, 0, 0);
    if (uiNumEntries == 0) {
        printf("no route entry.\n");
        return  (ERROR_NONE);
    }
    
    /*
     *  �� net safe ״̬�²�������� printf ��ʹ�� IO �����. ����ֻ�ܴ�ӡ����������,
     *  Ȼ��ͳһʹ�� IO ��ӡ.
     */
    stSize   = LW_RT_PRINT_SIZE * uiNumEntries;
    pcBuffer = (PCHAR)__SHEAP_ALLOC(stSize);
    if (pcBuffer == LW_NULL) {
        fprintf(stderr, "system low memory.\n");
        return  (PX_ERROR);
    }

    /*
     *  ��ӡ aodv route entry
     */
    stOffset = 0;
    printf("aodv routing tables\n");
    printf("Destination        Gateway            Mask               Flag     Hops     Interface\n");
    __rtSafeRun((FUNCPTR)aodv_rt_traversal, (void *)__aodvEntryPrint,
                pcBuffer, (PVOID)stSize, &stOffset, 0, 0);
    if (stOffset > 0) {
        fwrite(pcBuffer, stOffset, 1, stdout);
        fflush(stdout);                                                 /*  �������ȷ��������        */
    }

    __SHEAP_FREE(pcBuffer);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellRouteInit
** ��������: ע��·��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID __tshellRouteInit (VOID)
{
    API_TShellKeywordAdd("route", __tshellRoute);
    API_TShellFormatAdd("route", " [add | del | change] {-host | -net} [addr] [gateway] [[dev] | if]");
    API_TShellHelpAdd("route",   "show, add, del, change route table\n"
                                 "eg. route\n"
                                 "    route add -host(-net) 123.123.123.123 0.0.0.0 dev en1       (add a route and use netif default gatewat set)\n"
                                 "    route add -host(-net) 123.123.123.123 123.0.0.1 dev en1     (add a route and use specified gatewat set)\n"
                                 "    route add -host(-net) 0.0.0.0 123.0.0.1 dev en1             (set netif default gatewat: 123.0.0.1)\n"
                                 "    route change -host(-net) 123.123.123.123 0.0.0.0 dev en2    (change a route and use netif default gatewat set)\n"
                                 "    route change -host(-net) 123.123.123.123 123.0.0.1 dev en1  (change a route and use specified gatewat set)\n"
                                 "    route change -host(-net) 0.0.0.0 123.0.0.1 dev en1          (set netif default gatewat: 123.0.0.1)\n"
                                 "    route change default dev en2                                (set default netif)\n"
                                 "    route del 145.26.122.35                                     (delete a route)\n");
                                 
    API_TShellKeywordAdd("aodvs", __tshellAodvs);
    API_TShellHelpAdd("aodvs",   "show AODV route table\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  ���´���Ϊ 2013.01.16 ���
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: route_add
** ��������: ����һ��·����Ϣ
** �䡡��  : pinaddr       ·�ɵ�ַ
**           pinaddrGw     ���ص�ַ
**           type          HOST / NET
**           ifname        Ŀ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_add (struct in_addr *pinaddr, struct in_addr *pinaddrGw, int  type, const char *ifname)
{
    ip4_addr_t  ipaddr;
    ip4_addr_t  ipaddrGw;
    INT         iError;

    if (!pinaddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ipaddr.addr   = pinaddr->s_addr;
    ipaddrGw.addr = pinaddrGw->s_addr;
    
    switch (type) {
    
    case ROUTE_TYPE_HOST:                                               /*  ���һ������·��            */
        iError = __rtAdd(&ipaddr, &ipaddrGw, LW_RT_FLAG_H, ifname);
        break;
        
    case ROUTE_TYPE_NET:                                                /*  ���һ������·��            */
        iError = __rtAdd(&ipaddr, &ipaddrGw, 0ul, ifname);
        break;
        
    case ROUTE_TYPE_DEFAULT:                                            /*  ����Ĭ������                */
        iError = __rtSetDefautNetif(ifname);
        break;
        
    default:
        iError = PX_ERROR;
        _ErrorHandle(EINVAL);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: route_delete
** ��������: ɾ��һ��·����Ϣ
** �䡡��  : pinaddr       ·�ɵ�ַ (HOST or NET)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_delete (struct in_addr *pinaddr)
{
    ip4_addr_t   ipaddr;

    if (!pinaddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ipaddr.addr = pinaddr->s_addr;
    
    return  (__rtDel(&ipaddr));
}
/*********************************************************************************************************
** ��������: route_change
** ��������: �޸�һ��·����Ϣ
** �䡡��  : pinaddr       ·�ɵ�ַ
**           pinaddrGw     ���ص�ַ
**           type          HOST / NET
**           ifname        Ŀ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_change (struct in_addr *pinaddr, struct in_addr *pinaddrGw, int  type, const char *ifname)
{
    ip4_addr_t  ipaddr;
    ip4_addr_t  ipaddrGw;
    INT         iError;

    if (!pinaddr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ipaddr.addr   = pinaddr->s_addr;
    ipaddrGw.addr = pinaddrGw->s_addr;
    
    switch (type) {
    
    case ROUTE_TYPE_HOST:                                               /*  �޸�һ������·��            */
        iError = __rtChange(&ipaddr, &ipaddrGw, LW_RT_FLAG_H, ifname);
        break;
        
    case ROUTE_TYPE_NET:                                                /*  �޸�һ������·��            */
        iError = __rtChange(&ipaddr, &ipaddrGw, 0ul, ifname);
        break;
        
    case ROUTE_TYPE_DEFAULT:                                            /*  ����Ĭ������                */
        iError = __rtSetDefautNetif(ifname);
        break;
        
    default:
        iError = PX_ERROR;
        _ErrorHandle(EINVAL);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: route_getnum
** ��������: ��ȡ·�ɱ���Ϣ�������� (������ aodv)
** �䡡��  : NONE
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_getnum (void)
{
    INT     iNum = 1;                                                   /*  1 default gw                */
    
    iNum += (netif_get_num() * 2);                                      /*  ÿһ���������� buildin ��Ŀ */
    iNum += _G_uiTotalNum;                                              /*  ·������                    */
    
    return  (iNum);
}
/*********************************************************************************************************
** ��������: route_get
** ��������: ��ȡ·�ɱ���Ϣ
** �䡡��  : flag          ��ȡ���������� 
**           msgbuf        ·�ɱ�����
**           num           ��������
** �䡡��  : ��ȡ������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_get (u_char flag, struct route_msg  *msgbuf, size_t  num)
{
    return  (__rtGet(flag, msgbuf, num));
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
