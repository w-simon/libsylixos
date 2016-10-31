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
** ��   ��   ��: lwip_natlib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 03 �� 19 ��
**
** ��        ��: lwip NAT ֧�ְ��ڲ���.

** BUG:
2010.04.08  �����ǹ����³�Ⱥ��һ����������, ����ע��, ���������ʽ.
2010.04.12  ���� nat_ip_input_hook() ����, ϵͳʹ�õ� ip_input_hook().
2010.11.06  �������˻������ٿ�, ����ȴ����ֹͣ��صĻ�������, ת�����Ż������ݻ�������, ����������Ա
            �ǳ����������! ��������ԱΪ�������˵Ľ�����ש����, ȴ�õ���˴���.
            ip nat �������������.
2011.03.06  ���� gcc 4.5.1 ��� warning.
2011.06.23  �Ż�һ���ִ���. 
            �� NAT ת���������������ϸ������.
2011.07.01  lwip ֧�ֱ��������·�ɸĽ�����, ���� NAT ֧�ֵ�����ת��.
            https://savannah.nongnu.org/bugs/?33634
2011.07.30  �� ap ���벻�ڴ���˿ڷ�Χ����, ֱ���˳�.
2013.04.01  ���� GCC 4.7.3 �������� warning.
2015.12.16  ���� NAT ����֧������.
            ������������ӳ�书��.
2016.08.25  ������������ȱ�ٴ�����ƿ������.
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
#include "lwip/opt.h"
#include "lwip/inet.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/udp.h"
#include "lwip/icmp.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip_natlib.h"
/*********************************************************************************************************
  NAT ���ƿ黺���
*********************************************************************************************************/
static __NAT_CB             _G_natcbBuffer[LW_CFG_NET_NAT_MAX_SESSION];
static LW_OBJECT_HANDLE     _G_ulNatcbPool = 0;
/*********************************************************************************************************
  NAT ���������� AP ��������ӿ�
*********************************************************************************************************/
       struct netif        *_G_pnetifNatLocal = LW_NULL;
       struct netif        *_G_pnetifNatAp    = LW_NULL;
/*********************************************************************************************************
  NAT ������
*********************************************************************************************************/
       LW_OBJECT_HANDLE     _G_ulNatOpLock = 0;
/*********************************************************************************************************
  NAT ���ƿ�����
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatcbTcp  = LW_NULL;
static LW_LIST_LINE_HEADER  _G_plineNatcbUdp  = LW_NULL;
static LW_LIST_LINE_HEADER  _G_plineNatcbIcmp = LW_NULL;
/*********************************************************************************************************
  NAT ����
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatalias = LW_NULL;
/*********************************************************************************************************
  NAT ������������
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatmap = LW_NULL;
/*********************************************************************************************************
** ��������: __natChksumAdjust
** ��������: ���� NAT ���ݰ�У���.
** �䡡��  : ucChksum        points to the chksum in the packet
**           ucOptr          points to the old data in the packet
**           sOlen           old len
**           ucNptr          points to the new data in the packet
**           sNlen           new len
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __natChksumAdjust (u8_t  *ucChksum, u8_t  *ucOptr, s16_t  sOlen, u8_t  *ucNptr, s16_t  sNlen)
{
    s32_t     i32X, i32Old, i32New;
  
    i32X = (ucChksum[0] << 8) + ucChksum[1];
    i32X = ~i32X & 0xFFFF;
    
    while (sOlen) {
        i32Old  = (ucOptr[0] << 8) + ucOptr[1];
        ucOptr += 2;
        i32X   -= i32Old & 0xffff;
        if (i32X <= 0) {
            i32X--; 
            i32X &= 0xffff;
        }
        sOlen -= 2;
    }
    
    while (sNlen) {
        i32New  = (ucNptr[0] << 8) + ucNptr[1]; 
        ucNptr +=2;
        i32X   += i32New & 0xffff;
        if (i32X & 0x10000) {
            i32X++; 
            i32X &= 0xffff; 
        }
        sNlen -= 2;
    }
    
    i32X = ~i32X & 0xFFFF;
    ucChksum[0] = (u8_t)(i32X >> 8);
    ucChksum[1] = (u8_t)(i32X & 0xff);
}
/*********************************************************************************************************
** ��������: __natPoolCreate
** ��������: ���� NAT ���ƿ黺���.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __natPoolCreate (VOID)
{
    _G_ulNatcbPool = API_PartitionCreate("nat_pool", 
                                         (PVOID)_G_natcbBuffer,
                                         LW_CFG_NET_NAT_MAX_SESSION,
                                         sizeof(__NAT_CB),
                                         LW_OPTION_OBJECT_GLOBAL,
                                         LW_NULL);
}
/*********************************************************************************************************
** ��������: __natPoolAlloc
** ��������: ����һ�� NAT ���ƿ�.
** �䡡��  : NONE
** �䡡��  : NAT ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static __PNAT_CB   __natPoolAlloc (VOID)
{
    __PNAT_CB   pnatcb = (__PNAT_CB)API_PartitionGet(_G_ulNatcbPool);
    
    if (pnatcb) {
        lib_bzero(pnatcb, sizeof(__NAT_CB));
    }

    return  (pnatcb);
}
/*********************************************************************************************************
** ��������: __natPoolFree
** ��������: �ͷ�һ�� NAT ���ƿ�.
** �䡡��  : pnatcb            NAT ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID   __natPoolFree (__PNAT_CB  pnatcb)
{
    if (pnatcb) {
        API_PartitionPut(_G_ulNatcbPool, (PVOID)pnatcb);
    }
}
/*********************************************************************************************************
** ��������: __natNewmap
** ��������: ����һ�� NAT ����ӳ�����.
** �䡡��  : pipaddr       ���ػ��� IP
**           usPort        ���ػ��� �˿�
**           AssPort       ӳ��˿� 
**           ucProto       ʹ�õ�Э��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT   __natMapAdd (ip4_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto)
{
    __PNAT_MAP      pnatmap;
    
    pnatmap = (__PNAT_MAP)__SHEAP_ALLOC(sizeof(__NAT_MAP));
    if (pnatmap == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pnatmap->NAT_ucProto            = ucProto;
    pnatmap->NAT_ipaddrLocalIp.addr = pipaddr->addr;
    pnatmap->NAT_usLocalPort        = usPort;
    pnatmap->NAT_usAssPort          = AssPort;
    
    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    _List_Line_Add_Ahead(&pnatmap->NAT_lineManage, &_G_plineNatmap);
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natNewmap
** ��������: ɾ��һ�� NAT ����ӳ�����.
** �䡡��  : pipaddr       ���ػ��� IP
**           usPort        ���ػ��� �˿�
**           AssPort       ӳ��˿� 
**           ucProto       ʹ�õ�Э��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT   __natMapDelete (ip4_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto)
{
    PLW_LIST_LINE   plineTemp;
    __PNAT_MAP      pnatmap;
    PVOID           pvFree = LW_NULL;
    
    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    for (plineTemp  = _G_plineNatmap;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NAT_lineManage);
        if ((pnatmap->NAT_ucProto            == ucProto)       &&
            (pnatmap->NAT_ipaddrLocalIp.addr == pipaddr->addr) &&
            (pnatmap->NAT_usLocalPort        == usPort)        &&
            (pnatmap->NAT_usAssPort          == AssPort)) {
            break;
        }
    }
    if (plineTemp) {
        _List_Line_Del(&pnatmap->NAT_lineManage, &_G_plineNatmap);
        pvFree = pnatmap;
    }
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */
    
    if (pvFree) {
        __SHEAP_FREE(pvFree);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __natAliasAdd
** ��������: NAT ����������� (���� IP ӳ��)
** �䡡��  : pipaddrAlias      ����
**           ipaddrSLocalIp    ��Ӧ������ʼ IP
**           ipaddrELocalIp    ��Ӧ���ؽ��� IP
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natAliasAdd (const ip4_addr_t  *pipaddrAlias, 
                    const ip4_addr_t  *ipaddrSLocalIp,
                    const ip4_addr_t  *ipaddrELocalIp)
{
    __PNAT_ALIAS    pnatalias;
    
    pnatalias = (__PNAT_ALIAS)__SHEAP_ALLOC(sizeof(__NAT_ALIAS));
    if (pnatalias == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pnatalias->NAT_ipaddrAliasIp.addr  = pipaddrAlias->addr;
    pnatalias->NAT_ipaddrSLocalIp.addr = ipaddrSLocalIp->addr;
    pnatalias->NAT_ipaddrELocalIp.addr = ipaddrELocalIp->addr;
    
    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    _List_Line_Add_Ahead(&pnatalias->NAT_lineManage, &_G_plineNatalias);
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natAliasDelete
** ��������: NAT ɾ��������� (���� IP ӳ��)
** �䡡��  : pipaddrAlias      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natAliasDelete (const ip4_addr_t  *pipaddrAlias)
{
    PLW_LIST_LINE   plineTemp;
    __PNAT_ALIAS    pnatalias;
    PVOID           pvFree = LW_NULL;
    
    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    for (plineTemp  = _G_plineNatalias;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatalias = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NAT_lineManage);
        if (pnatalias->NAT_ipaddrAliasIp.addr == pipaddrAlias->addr) {
            break;
        }
    }
    if (plineTemp) {
        _List_Line_Del(&pnatalias->NAT_lineManage, &_G_plineNatalias);
        pvFree = pnatalias;
    }
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */
    
    if (pvFree) {
        __SHEAP_FREE(pvFree);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __natNewPort
** ��������: ����һ�� NAT ��ʱΨһ�˿�.
** �䡡��  : NONE
** �䡡��  : �˿�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static u16_t   __natNewPort (VOID)
{
    static u32_t    u32Port = LW_CFG_NET_NAT_MIN_PORT;
    __PNAT_CB       pnatcb;
    PLW_LIST_LINE   plineTemp;
    
__check_again:
    if (++u32Port > LW_CFG_NET_NAT_MAX_PORT) {
        u32Port = LW_CFG_NET_NAT_MIN_PORT;
    }
    
    for (plineTemp  = _G_plineNatcbTcp;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �����Ƿ����ظ��Ķ˿ں�      */
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        if (pnatcb->NAT_usAssPort == (u16_t)u32Port) {
            goto    __check_again;
        }
    }
    
    for (plineTemp  = _G_plineNatcbUdp;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �����Ƿ����ظ��Ķ˿ں�      */
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        if (pnatcb->NAT_usAssPort == (u16_t)u32Port) {
            goto    __check_again;
        }
    }
    
    for (plineTemp  = _G_plineNatcbIcmp;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �����Ƿ����ظ��Ķ˿ں�      */
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        if (pnatcb->NAT_usAssPort == (u16_t)u32Port) {
            goto    __check_again;
        }
    }
    
    return  ((u16_t)u32Port);
}
/*********************************************************************************************************
** ��������: __natNew
** ��������: ����һ�� NAT ����.
** �䡡��  : pipaddr       ���ػ��� IP
**           usPort        ���ػ��� �˿�
**           ucProto       ʹ�õ�Э��
** �䡡��  : NAT ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static __PNAT_CB   __natNew (ip4_addr_p_t  *pipaddr, u16_t  usPort, u8_t  ucProto)
{
    PLW_LIST_LINE   plineTemp;
    __PNAT_CB       pnatcb = __natPoolAlloc();
    __PNAT_CB       pnatcbOldest;
    u16_t           usNewPort;
    
    if (pnatcb == LW_NULL) {                                            /*  ��Ҫɾ������ϵ�            */
        /*
         *  û�п��ٳ���, ����Э����������������, ��Ȼ������һ�����ڽڵ�.
         */
        if (_G_plineNatcbTcp) {
            pnatcbOldest = _LIST_ENTRY(_G_plineNatcbTcp, __NAT_CB, NAT_lineManage);
        } else if (_G_plineNatcbUdp) {
            pnatcbOldest = _LIST_ENTRY(_G_plineNatcbUdp, __NAT_CB, NAT_lineManage);
        } else {
            pnatcbOldest = _LIST_ENTRY(_G_plineNatcbIcmp, __NAT_CB, NAT_lineManage);
        }
            
        for (plineTemp  = _G_plineNatcbTcp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            if (pnatcb->NAT_ulIdleTimer > pnatcbOldest->NAT_ulIdleTimer) {
                pnatcbOldest = pnatcb;
            }
        }
        for (plineTemp  = _G_plineNatcbUdp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            if (pnatcb->NAT_ulIdleTimer > pnatcbOldest->NAT_ulIdleTimer) {
                pnatcbOldest = pnatcb;
            }
        }
        for (plineTemp  = _G_plineNatcbIcmp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            if (pnatcb->NAT_ulIdleTimer > pnatcbOldest->NAT_ulIdleTimer) {
                pnatcbOldest = pnatcb;
            }
        }
        
        pnatcb = pnatcbOldest;                                          /*  ʹ�����ϵ�                  */
        switch (pnatcb->NAT_ucProto) {
        
        case IP_PROTO_TCP:                                              /*  TCP ����                    */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp);
            break;
            
        case IP_PROTO_UDP:                                              /*  UDP ����                    */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp);
            break;
            
        case IP_PROTO_ICMP:                                             /*  ICMP ����                   */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
            break;
            
        default:                                                        /*  ��Ӧ�����е�����            */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
            break;
        }
    }
        
    pnatcb->NAT_ucProto            = ucProto;
    pnatcb->NAT_ipaddrLocalIp.addr = pipaddr->addr;
    pnatcb->NAT_usLocalPort        = usPort;
    
    usNewPort = __natNewPort();
    pnatcb->NAT_usAssPort = htons(usNewPort);                           /*  ����һ���µ�Ψһ�˿�        */
    
    pnatcb->NAT_ulIdleTimer = 0;
    pnatcb->NAT_ulTermTimer = 0;
    pnatcb->NAT_iStatus     = __NAT_STATUS_OPEN;
    
    switch (ucProto) {
        
    case IP_PROTO_TCP:                                                  /*  TCP ����                    */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp);
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP ����                    */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp);
        break;
        
    case IP_PROTO_ICMP:                                                 /*  ICMP ����                   */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
        break;
        
    default:                                                            /*  ��Ӧ�����е�����            */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
        break;
    }
    
    return  (pnatcb);
}
/*********************************************************************************************************
** ��������: __natClose
** ��������: ɾ��һ�� NAT ����.
** �䡡��  : pnatcb            NAT ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __natClose (__PNAT_CB  pnatcb)
{
    if (pnatcb == LW_NULL) {
        return;
    }
    
    switch (pnatcb->NAT_ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP ����                    */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp);
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP ����                    */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp);
        break;
        
    case IP_PROTO_ICMP:                                                 /*  ICMP ����                   */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
        break;
        
    default:                                                            /*  ��Ӧ�����е�����            */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
        break;
    }

    __natPoolFree(pnatcb);
}
/*********************************************************************************************************
** ��������: __natTimer
** ��������: NAT ��ʱ��.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __natTimer (VOID)
{
    __PNAT_CB       pnatcb;
    PLW_LIST_LINE   plineTemp;
    
    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    plineTemp = _G_plineNatcbTcp;
    while (plineTemp) {
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        plineTemp = _list_line_get_next(plineTemp);                     /*  ָ����һ���ڵ�              */
        
        pnatcb->NAT_ulIdleTimer++;
        if (pnatcb->NAT_ulIdleTimer >= LW_CFG_NET_NAT_IDLE_TIMEOUT) {   /*  ����ʱ������ر�            */
            __natClose(pnatcb);
        
        } else {
            if (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) {
                pnatcb->NAT_ulTermTimer++;
                if (pnatcb->NAT_ulTermTimer >= 2) {                     /*  ������ʱ                    */
                    __natClose(pnatcb);
                }
            }
        }
    }
    
    plineTemp = _G_plineNatcbUdp;
    while (plineTemp) {
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        plineTemp = _list_line_get_next(plineTemp);                     /*  ָ����һ���ڵ�              */
        
        pnatcb->NAT_ulIdleTimer++;
        if (pnatcb->NAT_ulIdleTimer >= LW_CFG_NET_NAT_IDLE_TIMEOUT) {   /*  ����ʱ������ر�            */
            __natClose(pnatcb);
        }
    }
    
    plineTemp = _G_plineNatcbIcmp;
    while (plineTemp) {
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        plineTemp = _list_line_get_next(plineTemp);                     /*  ָ����һ���ڵ�              */
        
        pnatcb->NAT_ulIdleTimer++;
        if (pnatcb->NAT_ulIdleTimer >= LW_CFG_NET_NAT_IDLE_TIMEOUT) {   /*  ����ʱ������ر�            */
            __natClose(pnatcb);
        }
    }
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */
}
/*********************************************************************************************************
** ��������: __natLocalInput
** ��������: NAT ����ӿ����� (���ض�).
** �䡡��  : p             ���ݰ�
**           netif         ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static err_t  __natLocalInput (struct pbuf *p, struct netif *netif)
{
    struct ip_hdr           *iphdr   = (struct ip_hdr *)p->payload;
    struct tcp_hdr          *tcphdr  = LW_NULL;
    struct udp_hdr          *udphdr  = LW_NULL;
    struct icmp_echo_hdr    *icmphdr = LW_NULL;
    
    u32_t                    u32OldAddr;
    u16_t                    usDestPort, usSrcPort;
    u8_t                     ucProto;
    
    static u16_t             iphdrlen;
    
    __PNAT_CB                pnatcb    = LW_NULL;
    __PNAT_ALIAS             pnatalias = LW_NULL;
    PLW_LIST_LINE            plineTemp;
    LW_LIST_LINE_HEADER      plineHeader;
    
    
    if (ip4_addr_cmp(&iphdr->dest, netif_ip4_addr(netif))) {
        return  (ERR_OK);                                               /*  ָ�򱾻������ݰ�            */
    
    } else if (ip4_addr_cmp(&iphdr->dest, IP4_ADDR_BROADCAST)) {
        return  (ERR_OK);                                               /*  �������޹㲥                */
    }
    
    iphdrlen  = (u16_t)IPH_HL(iphdr);                                   /*  ��� IP ��ͷ����            */
    iphdrlen *= 4;
    
    /*
     *  ������뱣֤ p->payload �㹻��װ���� IP + UDP/TCP/ICMP �ı�ͷ
     */
    ucProto = IPH_PROTO(iphdr);
    switch (ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP ���ݱ�                  */
        tcphdr = (struct tcp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = tcphdr->dest;
        usSrcPort   = tcphdr->src;
        plineHeader = _G_plineNatcbTcp;
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP ���ݱ�                  */
        udphdr = (struct udp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = udphdr->dest;
        usSrcPort   = udphdr->src;
        plineHeader = _G_plineNatcbUdp;
        break;
        
    case IP_PROTO_ICMP:
        icmphdr = (struct icmp_echo_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usSrcPort   = usDestPort = icmphdr->id;
        plineHeader = _G_plineNatcbIcmp;
        break;
    
    default:
        plineHeader = _G_plineNatcbIcmp;
        break;
    }
    
    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    for (plineTemp  = plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        
        /*
         *  ���ƿ��ڵ�Դ�� IP �� Դ�� PORT ʹ��Э����ȫһ��.
         */
        if ((ip4_addr_cmp(&iphdr->src, &pnatcb->NAT_ipaddrLocalIp)) &&
            (usSrcPort == pnatcb->NAT_usLocalPort) &&
            (ucProto   == pnatcb->NAT_ucProto)) {
            break;                                                      /*  �ҵ��� NAT ���ƿ�           */
        }
    }
    if (plineTemp == LW_NULL) {
        pnatcb = LW_NULL;                                               /*  û���ҵ�                    */
    }
    
    if (pnatcb == LW_NULL) {
        INT  iSrcSameNet  = ip4_addr_netcmp(&iphdr->src, netif_ip4_addr(netif), netif_ip4_netmask(netif));
        INT  iDestSameNet = ip4_addr_netcmp(&iphdr->dest, netif_ip4_addr(netif), netif_ip4_netmask(netif));
        
        if (iSrcSameNet && !iDestSameNet) {
            /*  
             *  ���Դ��ַ�� local �ӿ�����ͬһ����, Ŀ�ĵ�ַ�� local ������ͬһ���� (����Ҫ NAT ת��)
             */
            pnatcb = __natNew(&iphdr->src, usSrcPort, ucProto);         /*  �½����ƿ�                  */
        }
    }
    
    if (pnatcb) {
        pnatcb->NAT_ulIdleTimer = 0;                                    /*  ˢ�¿���ʱ��                */
        
        /*
         *  �����ݰ�ת��Ϊ���� AP ��������ӿڷ��͵����ݰ�
         *  ���������Ч, ��ʹ�ñ�����ΪԴ��ַ.
         */
        u32OldAddr = iphdr->src.addr;
        for (plineTemp  = _G_plineNatalias;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  ���ұ�����                  */
        
            pnatalias = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NAT_lineManage);
            if ((pnatalias->NAT_ipaddrSLocalIp.addr <= ((ip4_addr_t *)&(iphdr->src))->addr) &&
                (pnatalias->NAT_ipaddrELocalIp.addr >= ((ip4_addr_t *)&(iphdr->src))->addr)) {
                break;
            }
        }
        if (plineTemp) {                                                /*  Դ IP ʹ�ñ��� IP           */
            ((ip4_addr_t *)&(iphdr->src))->addr = pnatalias->NAT_ipaddrAliasIp.addr;
            
        } else {                                                        /*  Դ IP ʹ�� AP �ӿ� IP       */
            ((ip4_addr_t *)&(iphdr->src))->addr = netif_ip4_addr(_G_pnetifNatAp)->addr;
        }
        __natChksumAdjust((u8_t *)&IPH_CHKSUM(iphdr),(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
        
        /*
         *  �������͵����������ݰ�ʹ�� NAT_usAssPort (Ψһ�ķ���˿�)
         */
        switch (IPH_PROTO(iphdr)) {
        
        case IP_PROTO_TCP:
            __natChksumAdjust((u8_t *)&tcphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
            tcphdr->src = pnatcb->NAT_usAssPort;
            __natChksumAdjust((u8_t *)&tcphdr->chksum,(u8_t *)&usSrcPort, 2, (u8_t *)&tcphdr->src, 2);
            break;
            
        case IP_PROTO_UDP:
            if (udphdr->chksum != 0) {
        	    __natChksumAdjust((u8_t *)&udphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
        	    udphdr->src = pnatcb->NAT_usAssPort;
        	    __natChksumAdjust((u8_t *)&udphdr->chksum,(u8_t *)&usSrcPort, 2, (u8_t *)&udphdr->src, 2);
        	}
            break;
            
        case IP_PROTO_ICMP:
            icmphdr->id = pnatcb->NAT_usAssPort;
            __natChksumAdjust((u8_t *)&icmphdr->chksum,(u8_t *)&usSrcPort, 2, (u8_t *)&icmphdr->id, 2);
            break;
            
        default:
            __NAT_OP_UNLOCK();                                          /*  ���� NAT ����               */
            return  (ERR_RTE);
            break;
        }
        
        /*
         *  NAT ���ƿ�״̬����
         */
        if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
            if (TCPH_FLAGS(tcphdr) & TCP_RST) {
	            pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
            
            } else if (TCPH_FLAGS(tcphdr) & TCP_FIN) {
	            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
	                pnatcb->NAT_iStatus = __NAT_STATUS_FIN;
	            
	            } else {
	                pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
	            }
            }
        }
    }
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */
    
    return  (ERR_OK);
}
/*********************************************************************************************************
** ��������: __natApInput
** ��������: NAT ����ӿ����� (��������).
** �䡡��  : p             ���ݰ�
**           netif         ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static err_t  __natApInput (struct pbuf *p, struct netif *netif)
{
    struct ip_hdr           *iphdr   = (struct ip_hdr *)p->payload;
    struct tcp_hdr          *tcphdr  = LW_NULL;
    struct udp_hdr          *udphdr  = LW_NULL;
    struct icmp_echo_hdr    *icmphdr = LW_NULL;
    
    u32_t                    u32OldAddr;
    u16_t                    usDestPort;
    u8_t                     ucProto;
    
    static u16_t             iphdrlen;
    
    __PNAT_CB                pnatcb  = LW_NULL;
    __PNAT_MAP               pnatmap = LW_NULL;
    PLW_LIST_LINE            plineTemp;
    LW_LIST_LINE_HEADER      plineHeader;
    
    
    iphdrlen  = (u16_t)IPH_HL(iphdr);                                   /*  ��� IP ��ͷ����            */
    iphdrlen *= 4;
    
    /*
     *  ������뱣֤ p->payload �㹻��װ���� IP + UDP/TCP/ICMP �ı�ͷ
     */
    ucProto = IPH_PROTO(iphdr);
    switch (ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP ���ݱ�                  */
        tcphdr = (struct tcp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = tcphdr->dest;
        plineHeader = _G_plineNatcbTcp;
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP ���ݱ�                  */
        udphdr = (struct udp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = udphdr->dest;
        plineHeader = _G_plineNatcbUdp;
        break;
        
    case IP_PROTO_ICMP:
        icmphdr = (struct icmp_echo_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = icmphdr->id;
        plineHeader = _G_plineNatcbIcmp;
        break;
    
    default:
        plineHeader = _G_plineNatcbIcmp;
        break;
    }

    __NAT_OP_LOCK();                                                    /*  ���� NAT ����               */
    if ((ntohs(usDestPort) < LW_CFG_NET_NAT_MIN_PORT) || 
        (ntohs(usDestPort) > LW_CFG_NET_NAT_MAX_PORT)) {                /*  Ŀ��˿ڲ��ڴ���˿�֮��    */
        for (plineTemp  = _G_plineNatmap;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NAT_lineManage);
            if ((usDestPort == pnatmap->NAT_usAssPort) &&
                (ucProto    == pnatmap->NAT_ucProto)) {
                break;                                                  /*  �ҵ��� NAT ӳ����ƿ�       */
            }
        }
        if (plineTemp == LW_NULL) {
            pnatmap = LW_NULL;                                          /*  û���ҵ�                    */
        }
        
        if (pnatmap) {
            for (plineTemp  = plineHeader;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                 
                pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
                if ((usDestPort == pnatcb->NAT_usAssPort) &&
                    (ucProto    == pnatcb->NAT_ucProto)) {
                    break;                                              /*  �ҵ��� NAT ���ƿ�           */
                }
            }
            if (plineTemp == LW_NULL) {
                ip4_addr_p_t  ipaddr;
            
                ipaddr.addr = pnatmap->NAT_ipaddrLocalIp.addr;
                pnatcb = __natNew(&ipaddr, 
                                  pnatmap->NAT_usLocalPort, ucProto);   /*  �½����ƿ�                  */
                pnatcb->NAT_usAssPort = pnatmap->NAT_usAssPort;
            }
        
            /*
             *  �����ݰ�Ŀ��תΪ��������Ŀ���ַ
             */
            u32OldAddr = iphdr->dest.addr;
            ((ip4_addr_t *)&(iphdr->dest))->addr = pnatmap->NAT_ipaddrLocalIp.addr;
            __natChksumAdjust((u8_t *)&IPH_CHKSUM(iphdr),(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
        
            /*
             *  �������͵����������ݰ�Ŀ��˿�Ϊ NAT_usLocalPort
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                __natChksumAdjust((u8_t *)&tcphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
                tcphdr->dest = pnatmap->NAT_usLocalPort;
                __natChksumAdjust((u8_t *)&tcphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&tcphdr->dest, 2);
            
            } else if (IPH_PROTO(iphdr) == IP_PROTO_UDP) {
                if (udphdr->chksum != 0) {
                    __natChksumAdjust((u8_t *)&udphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
    	            udphdr->dest = pnatmap->NAT_usLocalPort;
    	            __natChksumAdjust((u8_t *)&udphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&udphdr->dest, 2);
                }
                
            } else if ((IPH_PROTO(iphdr) == IP_PROTO_ICMP) && 
                       ((ICMPH_CODE(icmphdr) == ICMP_ECHO || ICMPH_CODE(icmphdr) == ICMP_ER))) {
                icmphdr->id = pnatmap->NAT_usLocalPort;
                __natChksumAdjust((u8_t *)&icmphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&icmphdr->id, 2);
            }
        }
    
    } else {
        for (plineTemp  = plineHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            if ((usDestPort == pnatcb->NAT_usAssPort) &&
                (ucProto    == pnatcb->NAT_ucProto)) {
                break;                                                  /*  �ҵ��� NAT ���ƿ�           */
            }
        }
        if (plineTemp == LW_NULL) {
            pnatcb = LW_NULL;                                           /*  û���ҵ�                    */
        }
    
        if (pnatcb) {                                                   /*  ����ҵ����ƿ�              */
            pnatcb->NAT_ulIdleTimer = 0;                                /*  ˢ�¿���ʱ��                */
            
            /*
             *  �����ݰ�Ŀ��תΪ��������Ŀ���ַ
             */
            u32OldAddr = iphdr->dest.addr;
            ((ip4_addr_t *)&(iphdr->dest))->addr = pnatcb->NAT_ipaddrLocalIp.addr;
            __natChksumAdjust((u8_t *)&IPH_CHKSUM(iphdr),(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
            
            /*
             *  �������͵����������ݰ�Ŀ��˿�Ϊ NAT_usLocalPort
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                __natChksumAdjust((u8_t *)&tcphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
                tcphdr->dest = pnatcb->NAT_usLocalPort;
                __natChksumAdjust((u8_t *)&tcphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&tcphdr->dest, 2);
            
            } else if (IPH_PROTO(iphdr) == IP_PROTO_UDP) {
                if (udphdr->chksum != 0) {
                    __natChksumAdjust((u8_t *)&udphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
    	            udphdr->dest = pnatcb->NAT_usLocalPort;
    	            __natChksumAdjust((u8_t *)&udphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&udphdr->dest, 2);
                }
                
            } else if ((IPH_PROTO(iphdr) == IP_PROTO_ICMP) && 
                       ((ICMPH_CODE(icmphdr) == ICMP_ECHO || ICMPH_CODE(icmphdr) == ICMP_ER))) {
                icmphdr->id = pnatcb->NAT_usLocalPort;
                __natChksumAdjust((u8_t *)&icmphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&icmphdr->id, 2);
            }
            
            /*
             *  NAT ���ƿ�״̬����
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                if (TCPH_FLAGS(tcphdr) & TCP_RST) {
    	            pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
                
                } else if (TCPH_FLAGS(tcphdr) & TCP_FIN) {
    	            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
    	                pnatcb->NAT_iStatus = __NAT_STATUS_FIN;
    	            
    	            } else {
    	                pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
    	            }
                }
            }
        }
    }
    __NAT_OP_UNLOCK();                                                  /*  ���� NAT ����               */

    return  (ERR_OK);
}
/*********************************************************************************************************
** ��������: nat_ip_input_hook
** ��������: NAT ip ����ص�
** �䡡��  : p         ���ݰ�
**           inp       ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  nat_ip_input_hook (struct pbuf *p, struct netif *inp)
{
    struct ip_hdr   *iphdr = (struct ip_hdr *)p->payload;
    
    if (IPH_V(iphdr) != 4) {
        return;                                                         /*  IPv4 ��Ч                   */
    }

    if (_G_pnetifNatLocal == _G_pnetifNatAp) {                          /*  �����ڴ���                  */
        if (inp == _G_pnetifNatLocal) {
            if (ip4_addr_cmp(&iphdr->dest, netif_ip4_addr(inp))) {
                __natApInput(p, inp);                                   /*  ָ�򱾻������ݰ�            */
            
            } else if (ip4_addr_isbroadcast(&iphdr->dest, inp) == 0) {
                __natLocalInput(p, inp);                                /*  ��ָ�򱾻��ķǹ㲥��        */
            }
        }
    
    } else {                                                            /*  ������                      */
        if (inp == _G_pnetifNatLocal) {
            __natLocalInput(p, inp);
        
        } else if (inp == _G_pnetifNatAp) {
            __natApInput(p, inp);
        }
    }
}
/*********************************************************************************************************
  NAT proc
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  NAT proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsNatSummaryRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
static ssize_t  __procFsNatAssNodeRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
/*********************************************************************************************************
  NAT proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoNatSummaryFuncs = {
    __procFsNatSummaryRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoNatAssNodeFuncs = {
    __procFsNatAssNodeRead, LW_NULL
};
/*********************************************************************************************************
  NAT proc �ļ�Ŀ¼��
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnNat[] = 
{
    LW_PROCFS_INIT_NODE("nat", 
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("info", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNatSummaryFuncs, 
                        "I",
                        0),
                        
    LW_PROCFS_INIT_NODE("assnode", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNatAssNodeFuncs, 
                        "A",
                        0),
};
/*********************************************************************************************************
** ��������: __procFsNatSummaryRead
** ��������: procfs ��һ���ں� nat/info proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNatSummaryRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    const CHAR   cAliasInfoHeader[] = "\n"
    "     ALIAS        LOCAL START     LOCAL END\n"
    "--------------- --------------- ---------------\n";

    const CHAR   cMapInfoHeader[] = "\n"
    " ASS PORT  LOCAL PORT    LOCAL IP      PROTO\n"
    "---------- ---------- --------------- --------\n";
    
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize;
        UINT            uiAssNum = 0;
        CHAR            cNetifAp[8];
        CHAR            cNetifLocal[8];
        PLW_LIST_LINE   plineTemp;
        PCHAR           pcProto;
        __PNAT_ALIAS    pnatalias;
        __PNAT_MAP      pnatmap;
        
        stNeedBufferSize = 512;                                         /*  ��ʼ��С                    */
        
        __NAT_OP_LOCK();                                                /*  ���� NAT ����               */
        for (plineTemp  = _G_plineNatalias;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  ������                      */
            stNeedBufferSize += 64;
        }
        for (plineTemp  = _G_plineNatmap;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        __NAT_OP_UNLOCK();                                              /*  ���� NAT ����               */
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        __NAT_OP_LOCK();                                                /*  ���� NAT ����               */
        if ((_G_pnetifNatAp    == LW_NULL) || 
            (_G_pnetifNatLocal == LW_NULL)) {
            __NAT_OP_UNLOCK();                                          /*  ���� NAT ����               */
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, 
                                  "NAT networking off!\n");
        
        } else {
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, 
                                  "NAT networking alias setting >>\n");
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%s", cAliasInfoHeader);
            
            for (plineTemp  = _G_plineNatalias;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {         /*  ������                      */
            
                CHAR    cIpBuffer1[INET_ADDRSTRLEN];
                CHAR    cIpBuffer2[INET_ADDRSTRLEN];
                CHAR    cIpBuffer3[INET_ADDRSTRLEN];
                
                pnatalias  = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NAT_lineManage);
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                      "%-15s %-15s %-15s\n", 
                                      ip4addr_ntoa_r(&pnatalias->NAT_ipaddrAliasIp,
                                                     cIpBuffer1, INET_ADDRSTRLEN),
                                      ip4addr_ntoa_r(&pnatalias->NAT_ipaddrSLocalIp, 
                                                     cIpBuffer2, INET_ADDRSTRLEN),
                                      ip4addr_ntoa_r(&pnatalias->NAT_ipaddrELocalIp, 
                                                     cIpBuffer3, INET_ADDRSTRLEN));
            }
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "\nNAT networking direct map setting >>\n");
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%s", cMapInfoHeader);
                                  
            for (plineTemp  = _G_plineNatmap;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                
                CHAR    cIpBuffer[INET_ADDRSTRLEN];
                
                pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NAT_lineManage);
                switch (pnatmap->NAT_ucProto) {
                
                case IP_PROTO_ICMP: pcProto = "ICMP"; break;
                case IP_PROTO_UDP:  pcProto = "UDP";  break;
                case IP_PROTO_TCP:  pcProto = "TCP";  break;
                default:            pcProto = "?";    break;
                }
                
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                      "%10d %10d %-15s %s\n",
                                      ntohs(pnatmap->NAT_usAssPort),
                                      ntohs(pnatmap->NAT_usLocalPort),
                                      ip4addr_ntoa_r(&pnatmap->NAT_ipaddrLocalIp, 
                                                     cIpBuffer, INET_ADDRSTRLEN),
                                       pcProto);
            }
            
            for (plineTemp  = _G_plineNatcbTcp;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                uiAssNum++;
            }
            for (plineTemp  = _G_plineNatcbUdp;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                uiAssNum++;
            }
            for (plineTemp  = _G_plineNatcbIcmp;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                uiAssNum++;
            }
            
            cNetifAp[0] = _G_pnetifNatAp->name[0];
            cNetifAp[1] = _G_pnetifNatAp->name[1];
            cNetifAp[2] = (CHAR)(_G_pnetifNatAp->num + '0');
            cNetifAp[3] = PX_EOS;
            
            cNetifLocal[0] = _G_pnetifNatLocal->name[0];
            cNetifLocal[1] = _G_pnetifNatLocal->name[1];
            cNetifLocal[2] = (CHAR)(_G_pnetifNatLocal->num + '0');
            cNetifLocal[3] = PX_EOS;
            __NAT_OP_UNLOCK();                                          /*  ���� NAT ����               */
    
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "\nNAT networking summary >>\n");
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "    LAN: %s WAN: %s\n", cNetifLocal, cNetifAp);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "    Total Ass-node: %d\n", LW_CFG_NET_NAT_MAX_SESSION);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "    Used  Ass-node: %d\n", uiAssNum);
        }
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʿ������ֽ���        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsNatAssNodeRead
** ��������: procfs ��һ���ں� nat/assnode proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNatAssNodeRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    const CHAR   cNatInfoHeader[] = "\n"
    "    LOCAL IP    LOCAL PORT ASS PORT PROTO IDLE(min)  STATUS\n"
    "--------------- ---------- -------- ----- --------- --------\n";
    
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize;
        PCHAR           pcProto;
        PCHAR           pcStatus;
        __PNAT_CB       pnatcb = LW_NULL;
        PLW_LIST_LINE   plineTemp;
        
        stNeedBufferSize = 256;                                         /*  ��ʼ��С                    */
        
        __NAT_OP_LOCK();                                                /*  ���� NAT ����               */
        for (plineTemp  = _G_plineNatcbTcp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        for (plineTemp  = _G_plineNatcbUdp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        for (plineTemp  = _G_plineNatcbIcmp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        __NAT_OP_UNLOCK();                                              /*  ���� NAT ����               */
    
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cNatInfoHeader); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        __NAT_OP_LOCK();                                                /*  ���� NAT ����               */
        for (plineTemp  = _G_plineNatcbTcp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            struct in_addr  inaddr;
            CHAR            cIpBuffer[INET_ADDRSTRLEN];
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            inaddr.s_addr = pnatcb->NAT_ipaddrLocalIp.addr;
            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                pcStatus = "OPEN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_FIN) {
                pcStatus = "FIN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) {
                pcStatus = "CLOSING";
            } else {
                pcStatus = "?";
            }
            
            inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                  ntohs(pnatcb->NAT_usLocalPort),
                                  ntohs(pnatcb->NAT_usAssPort),
                                  "TCP",
                                  pnatcb->NAT_ulIdleTimer,
                                  pcStatus);
        }
        
        for (plineTemp  = _G_plineNatcbUdp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            struct in_addr  inaddr;
            CHAR            cIpBuffer[INET_ADDRSTRLEN];

            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            inaddr.s_addr = pnatcb->NAT_ipaddrLocalIp.addr;
            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                pcStatus = "OPEN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_FIN) {
                pcStatus = "FIN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) {
                pcStatus = "CLOSING";
            } else {
                pcStatus = "?";
            }
            
            inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                  ntohs(pnatcb->NAT_usLocalPort),
                                  ntohs(pnatcb->NAT_usAssPort),
                                  "UDP",
                                  pnatcb->NAT_ulIdleTimer,
                                  pcStatus);
        }
        
        for (plineTemp  = _G_plineNatcbIcmp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            struct in_addr  inaddr;
            CHAR            cIpBuffer[INET_ADDRSTRLEN];
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            inaddr.s_addr = pnatcb->NAT_ipaddrLocalIp.addr;
            if (pnatcb->NAT_ucProto == IP_PROTO_ICMP) {
                pcProto = "ICMP";
            } else {
                pcProto = "?";
            }
            
            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                pcStatus = "OPEN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_FIN) {
                pcStatus = "FIN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) {
                pcStatus = "CLOSING";
            } else {
                pcStatus = "?";
            }
            
            inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                  ntohs(pnatcb->NAT_usLocalPort),
                                  ntohs(pnatcb->NAT_usAssPort),
                                  pcProto,
                                  pnatcb->NAT_ulIdleTimer,
                                  pcStatus);
        }
        __NAT_OP_UNLOCK();                                              /*  ���� NAT ����               */
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʿ������ֽ���        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsNatInit
** ��������: procfs ��ʼ���ں� NAT �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsNatInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnNat[0], "/net");
    API_ProcFsMakeNode(&_G_pfsnNat[1], "/net/nat");
    API_ProcFsMakeNode(&_G_pfsnNat[2], "/net/nat");
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
