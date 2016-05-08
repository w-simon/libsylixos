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
** ��   ��   ��: lwip_proc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 06 �� 21 ��
**
** ��        ��: procfs ��Ӧ���粿�ֽӿ�.

** BUG:
2013.08.22  ���� tcp tcp6 udp udp6 udplite udplite6 raw raw6 igmp igmp6 �ļ�.
            ���� route �ļ�.
2013.09.11  ���� dev �ļ�.
            ���� arp �ļ�.
2013.09.12  ʹ�� bnprintf �򻺳�����ӡ����. 
2013.09.24  ���� if_inet6 �ļ�.
2013.10.14  ���� aodv_rt ��ȡ aodv ��ǰ·�ɱ�.
2014.04.03  ���� packet.
2014.05.06  tcp listen ��ӡ, ����� IPv6 ���ӡ�Ƿ�ֻ���� IPv6 ��������.
2014.06.25  ���� wireless �ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_PROCFS_EN > 0)
#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include "lwip/stats.h"
/*********************************************************************************************************
  unix
*********************************************************************************************************/
#include "../unix/af_unix.h"
/*********************************************************************************************************
  packet
*********************************************************************************************************/
#include "../packet/af_packet.h"
/*********************************************************************************************************
  aodv
*********************************************************************************************************/
#include "../src/netif/aodv/aodv_route.h"
/*********************************************************************************************************
  TCP
*********************************************************************************************************/
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
extern struct tcp_pcb           *tcp_active_pcbs;
extern union  tcp_listen_pcbs_t  tcp_listen_pcbs;
extern struct tcp_pcb           *tcp_tw_pcbs;
/*********************************************************************************************************
  UDP
*********************************************************************************************************/
#include "lwip/udp.h"
extern struct udp_pcb *udp_pcbs;
/*********************************************************************************************************
  RAW
*********************************************************************************************************/
#include "lwip/raw.h"
extern struct raw_pcb *raw_pcbs;
/*********************************************************************************************************
  IGMP
*********************************************************************************************************/
#include "lwip/igmp.h"
#include "lwip/mld6.h"
extern struct igmp_group *igmp_group_list;
extern struct mld_group  *mld_group_list;
/*********************************************************************************************************
  ROUTE
*********************************************************************************************************/
#include "sys/route.h"
/*********************************************************************************************************
  ARP
*********************************************************************************************************/
#include "netif/etharp.h"
/*********************************************************************************************************
  PPP ����
*********************************************************************************************************/
#if LW_CFG_LWIP_PPP > 0
#include "lwip/pppapi.h"
#endif                                                                  /*  LW_CFG_LWIP_PPP > 0         */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#if LW_CFG_NET_WIRELESS_EN > 0
#include "net/if_wireless.h"
#include "net/if_whandler.h"
#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
  ���� proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsNetTcpRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
static ssize_t  __procFsNetTcp6Read(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
static ssize_t  __procFsNetUdpRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
static ssize_t  __procFsNetUdp6Read(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
static ssize_t  __procFsNetRawRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
static ssize_t  __procFsNetRaw6Read(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
static ssize_t  __procFsNetIgmpRead(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
static ssize_t  __procFsNetIgmp6Read(PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft);
static ssize_t  __procFsNetRouteRead(PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft);
static ssize_t  __procFsNetTcpipStatRead(PLW_PROCFS_NODE  p_pfsn, 
                                         PCHAR            pcBuffer, 
                                         size_t           stMaxBytes,
                                         off_t            oft);
static ssize_t  __procFsNetUnixRead(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
static ssize_t  __procFsNetDevRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
static ssize_t  __procFsNetIfInet6Read(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
static ssize_t  __procFsNetArpRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
static ssize_t  __procFsNetAodvRead(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
static ssize_t  __procFsNetAodvRtRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
static ssize_t  __procFsNetPacketRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
static ssize_t  __procFsNetPppRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
static ssize_t  __procFsNetWlRead(PLW_PROCFS_NODE  p_pfsn, 
                                  PCHAR            pcBuffer, 
                                  size_t           stMaxBytes,
                                  off_t            oft);
/*********************************************************************************************************
  ���� proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP    _G_pfsnoNetTcpFuncs = {
    __procFsNetTcpRead,         LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetTcp6Funcs = {
    __procFsNetTcp6Read,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetUdpFuncs = {
    __procFsNetUdpRead,         LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetUdp6Funcs = {
    __procFsNetUdp6Read,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetUdpliteFuncs = {
    __procFsNetUdpRead,         LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetUdplite6Funcs = {
    __procFsNetUdp6Read,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetRawFuncs = {
    __procFsNetRawRead,         LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetRaw6Funcs = {
    __procFsNetRaw6Read,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetIgmpFuncs = {
    __procFsNetIgmpRead,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetIgmp6Funcs = {
    __procFsNetIgmp6Read,       LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetRouteFuncs = {
    __procFsNetRouteRead,       LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetTcpipStatFuncs = {
    __procFsNetTcpipStatRead,   LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetUnixFuncs = {
    __procFsNetUnixRead,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetDevFuncs = {
    __procFsNetDevRead,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetIfInet6Funcs = {
    __procFsNetIfInet6Read,    LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetArpFuncs = {
    __procFsNetArpRead,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetAodvFuncs = {
    __procFsNetAodvRead,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetAodvRtFuncs = {
    __procFsNetAodvRtRead,      LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetPacketFuncs = {
    __procFsNetPacketRead,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetPppFuncs = {
    __procFsNetPppRead,        LW_NULL
};
static LW_PROCFS_NODE_OP    _G_pfsnoNetWlFuncs = {
    __procFsNetWlRead,        LW_NULL
};
/*********************************************************************************************************
  ���� proc �ļ�Ŀ¼��
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_TCPIP_STAT     8192
#define __PROCFS_BUFFER_SIZE_AODV           2048
#define __PROCFS_BUFFER_SIZE_ARP            ((LW_CFG_LWIP_ARP_TABLE_SIZE * 64) + 64)

static LW_PROCFS_NODE       _G_pfsnNet[] = 
{
    LW_PROCFS_INIT_NODE("net", 
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
    
    LW_PROCFS_INIT_NODE("mesh-adhoc", 
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("aodv_para", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetAodvFuncs, 
                        "A",
                        __PROCFS_BUFFER_SIZE_AODV),
                        
    LW_PROCFS_INIT_NODE("aodv_rt", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetAodvRtFuncs, 
                        "R",
                        0),
    
    LW_PROCFS_INIT_NODE("tcp", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetTcpFuncs, 
                        "t",
                        0),
    LW_PROCFS_INIT_NODE("tcp6", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetTcp6Funcs, 
                        "t",
                        0),
    
    LW_PROCFS_INIT_NODE("udp", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetUdpFuncs, 
                        "u",
                        0),
    LW_PROCFS_INIT_NODE("udp6", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetUdp6Funcs, 
                        "u",
                        0),
    
    LW_PROCFS_INIT_NODE("udplite", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetUdpliteFuncs, 
                        "u",
                        0),
    LW_PROCFS_INIT_NODE("udplite6", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetUdplite6Funcs, 
                        "u",
                        0),
                        
    LW_PROCFS_INIT_NODE("raw", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetRawFuncs, 
                        "r",
                        0),
    LW_PROCFS_INIT_NODE("raw6", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetRaw6Funcs, 
                        "r",
                        0),
                        
    LW_PROCFS_INIT_NODE("igmp", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetIgmpFuncs, 
                        "r",
                        0),
    LW_PROCFS_INIT_NODE("igmp6", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetIgmp6Funcs, 
                        "r",
                        0),
                        
    LW_PROCFS_INIT_NODE("route", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetRouteFuncs, 
                        "r",
                        0),
                        
    LW_PROCFS_INIT_NODE("tcpip_stat", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetTcpipStatFuncs, 
                        "r",
                        __PROCFS_BUFFER_SIZE_TCPIP_STAT),
    
    LW_PROCFS_INIT_NODE("unix", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetUnixFuncs, 
                        "A",
                        0),
                        
    LW_PROCFS_INIT_NODE("dev", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetDevFuncs, 
                        "D",
                        0),
    LW_PROCFS_INIT_NODE("if_inet6", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetIfInet6Funcs, 
                        "I",
                        0),
                        
    LW_PROCFS_INIT_NODE("arp", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetArpFuncs, 
                        "A",
                        __PROCFS_BUFFER_SIZE_ARP),
                        
    LW_PROCFS_INIT_NODE("packet", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetPacketFuncs, 
                        "P",
                        0),
                        
    LW_PROCFS_INIT_NODE("ppp", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetPppFuncs, 
                        "p",
                        0),
                        
    LW_PROCFS_INIT_NODE("wireless", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetWlFuncs, 
                        "w",
                        0),
};
/*********************************************************************************************************
** ��������: __procFsNetTcpGetStat
** ��������: ��� tcp ״̬��Ϣ
** �䡡��  : state     ״̬
** �䡡��  : ״̬�ִ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static CPCHAR  __procFsNetTcpGetStat (u8_t  state)
{
    static const PCHAR cTcpState[] = {
        "close",
        "listen",
        "syn_send",
        "syn_rcvd",
        "estab",
        "fin_w_1",
        "fin_w_2",
        "close_w",
        "closeing",
        "last_ack",
        "time_w",
        "unknown"
    };
    
    if (state > 11) {
        state = 11;
    }
    
    return  (cTcpState[state]);
}
/*********************************************************************************************************
** ��������: __procFsNetTcpPrint
** ��������: ��ӡ���� tcp �ļ�
** �䡡��  : pcb           tcp ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetTcpPrint (struct tcp_pcb *pcb, PCHAR  pcBuffer, 
                                  size_t  stTotalSize, size_t *pstOft)
{
    if (pcb->isipv6 == 0) {
        if (pcb->state == LISTEN) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%08X:%04X 00000000:0000 %-8s %7d %7d %7d\n",
                               pcb->local_ip.ip4.addr, htons(pcb->local_port),
                               __procFsNetTcpGetStat((u8_t)pcb->state),
                               0, 0, 0);
        } else {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%08X:%04X %08X:%04X %-8s %7d %7d %7d\n",
                               pcb->local_ip.ip4.addr, htons(pcb->local_port),
                               pcb->remote_ip.ip4.addr, htons(pcb->remote_port),
                               __procFsNetTcpGetStat((u8_t)pcb->state),
                               (u32_t)pcb->nrtx, (u32_t)pcb->rcv_wnd, (u32_t)pcb->snd_wnd);
        }
    } else {
        if (pcb->state == LISTEN) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%08X%08X%08X%08X:%04X %08X%08X%08X%08X:%04X %-8s %-9s %7d %7d %7d\n",
                               pcb->local_ip.ip6.addr[0],
                               pcb->local_ip.ip6.addr[1],
                               pcb->local_ip.ip6.addr[2],
                               pcb->local_ip.ip6.addr[3],
                               htons(pcb->local_port),
                               IPADDR_ANY, IPADDR_ANY, IPADDR_ANY, IPADDR_ANY, 0,
                               __procFsNetTcpGetStat((u8_t)pcb->state),
                               (((struct tcp_pcb_listen *)pcb)->accept_any_ip_version) ? "NO" : "YES",
                               0, 0, 0);
        } else {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%08X%08X%08X%08X:%04X %08X%08X%08X%08X:%04X %-8s %-9s %7d %7d %7d\n",
                               pcb->local_ip.ip6.addr[0],
                               pcb->local_ip.ip6.addr[1],
                               pcb->local_ip.ip6.addr[2],
                               pcb->local_ip.ip6.addr[3],
                               htons(pcb->local_port),
                               pcb->remote_ip.ip6.addr[0],
                               pcb->remote_ip.ip6.addr[1],
                               pcb->remote_ip.ip6.addr[2],
                               pcb->remote_ip.ip6.addr[3],
                               htons(pcb->remote_port),
                               __procFsNetTcpGetStat((u8_t)pcb->state),
                               "",
                               (u32_t)pcb->nrtx, (u32_t)pcb->rcv_wnd, (u32_t)pcb->snd_wnd);
        }
    }
}
/*********************************************************************************************************
** ��������: __procFsNetTcpRead
** ��������: procfs ��һ����ȡ���� tcp �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetTcpRead (PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft)
{
    const CHAR      cTcpInfoHdr[] = 
    "LOCAL         REMOTE        STATUS   RETRANS RCV_WND SND_WND\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t  stNeedBufferSize = 0;
        struct tcp_pcb *pcb;
        
        LOCK_TCPIP_CORE();
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0) {
                stNeedBufferSize += 128;
            }
        }
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0) {
                stNeedBufferSize += 128;
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0) {
                stNeedBufferSize += 128;
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cTcpInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cTcpInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0) {
                __procFsNetTcpPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0) {
                __procFsNetTcpPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0) {
                __procFsNetTcpPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        UNLOCK_TCPIP_CORE();
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
** ��������: __procFsNetTcp6Read
** ��������: procfs ��һ����ȡ���� tcp6 �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetTcp6Read (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
    const CHAR      cTcpInfoHdr[] = 
    "LOCAL                                 REMOTE                                "
    "STATUS   IPv6-ONLY RETRANS RCV_WND SND_WND\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t  stNeedBufferSize = 0;
        struct tcp_pcb *pcb;
        
        LOCK_TCPIP_CORE();
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6) {
                stNeedBufferSize += 192;
            }
        }
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6) {
                stNeedBufferSize += 192;
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6) {
                stNeedBufferSize += 192;
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cTcpInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cTcpInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6) {
                __procFsNetTcpPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6) {
                __procFsNetTcpPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6) {
                __procFsNetTcpPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        UNLOCK_TCPIP_CORE();
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
** ��������: __procFsNetUdpPrint
** ��������: ��ӡ���� udp �ļ�
** �䡡��  : pcb           udp ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetUdpPrint (struct udp_pcb *pcb, PCHAR  pcBuffer, 
                                  size_t  stTotalSize, size_t *pstOft)
{
    if (pcb->isipv6 == 0) {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%08X:%04X %08X:%04X\n",
                           pcb->local_ip.ip4.addr, htons(pcb->local_port),
                           pcb->remote_ip.ip4.addr, htons(pcb->remote_port));
    } else {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%08X%08X%08X%08X:%04X %08X%08X%08X%08X:%04X\n",
                           pcb->local_ip.ip6.addr[0],
                           pcb->local_ip.ip6.addr[1],
                           pcb->local_ip.ip6.addr[2],
                           pcb->local_ip.ip6.addr[3],
                           htons(pcb->local_port),
                           pcb->remote_ip.ip6.addr[0],
                           pcb->remote_ip.ip6.addr[1],
                           pcb->remote_ip.ip6.addr[2],
                           pcb->remote_ip.ip6.addr[3],
                           htons(pcb->remote_port));
    }
}
/*********************************************************************************************************
** ��������: __procFsNetUdpRead
** ��������: procfs ��һ����ȡ���� udp �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetUdpRead (PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft)
{
    const CHAR      cUdpInfoHdr[] = 
    "LOCAL         REMOTE\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize = 0;
        struct udp_pcb *pcb;
        int             udplite;
        
        if (p_pfsn->PFSN_p_pfsnoFuncs == &_G_pfsnoNetUdpFuncs) {
            udplite = 0;
        } else {
            udplite = UDP_FLAGS_UDPLITE;
        }
        
        LOCK_TCPIP_CORE();
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0 && ((pcb->flags & UDP_FLAGS_UDPLITE) == udplite)) {
                stNeedBufferSize += 64;
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cUdpInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cUdpInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0 && ((pcb->flags & UDP_FLAGS_UDPLITE) == udplite)) {
                __procFsNetUdpPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        UNLOCK_TCPIP_CORE();
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
** ��������: __procFsNetUdp6Read
** ��������: procfs ��һ����ȡ���� udp6 �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetUdp6Read (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
    const CHAR      cUdpInfoHdr[] = 
    "LOCAL                                 REMOTE\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize = 0;
        struct udp_pcb *pcb;
        int             udplite;
        
        if (p_pfsn->PFSN_p_pfsnoFuncs == &_G_pfsnoNetUdp6Funcs) {
            udplite = 0;
        } else {
            udplite = UDP_FLAGS_UDPLITE;
        }
        
        LOCK_TCPIP_CORE();
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 && ((pcb->flags & UDP_FLAGS_UDPLITE) == udplite)) {
                stNeedBufferSize += 128;
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cUdpInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cUdpInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 && ((pcb->flags & UDP_FLAGS_UDPLITE) == udplite)) {
                __procFsNetUdpPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        UNLOCK_TCPIP_CORE();
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
** ��������: __procFsNetRawGetProto
** ��������: ���Э����Ϣ
** �䡡��  : state     ״̬
** �䡡��  : ״̬�ִ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static CPCHAR  __procFsNetRawGetProto (u8_t  proto)
{
    switch (proto) {
    
    case IPPROTO_IP:
        return  ("ip");
        
    case IPPROTO_IPV6:
        return  ("ipv6");
        
    case IPPROTO_ICMP:
        return  ("icmp");
        
    case IPPROTO_TCP:
        return  ("tcp");
        
    case IPPROTO_UDP:
        return  ("udp");
        
    case IPPROTO_UDPLITE:
        return  ("udplite");
        
    default:
        return  ("unknown");
    }
}
/*********************************************************************************************************
** ��������: __procFsNetRawPrint
** ��������: ��ӡ���� raw �ļ�
** �䡡��  : pcb           raw ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetRawPrint (struct raw_pcb *pcb, PCHAR  pcBuffer, 
                                  size_t  stTotalSize, size_t *pstOft)
{
    if (pcb->isipv6 == 0) {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%08X %08X %s\n",
                           pcb->local_ip.ip4.addr,
                           pcb->remote_ip.ip4.addr,
                           __procFsNetRawGetProto(pcb->protocol));
    } else {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%08X%08X%08X%08X %08X%08X%08X%08X %s\n",
                           pcb->local_ip.ip6.addr[0],
                           pcb->local_ip.ip6.addr[1],
                           pcb->local_ip.ip6.addr[2],
                           pcb->local_ip.ip6.addr[3],
                           pcb->remote_ip.ip6.addr[0],
                           pcb->remote_ip.ip6.addr[1],
                           pcb->remote_ip.ip6.addr[2],
                           pcb->remote_ip.ip6.addr[3],
                           __procFsNetRawGetProto(pcb->protocol));
    }
}
/*********************************************************************************************************
** ��������: __procFsNetRawRead
** ��������: procfs ��һ����ȡ���� raw �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetRawRead (PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft)
{
    const CHAR      cRawInfoHdr[] = 
    "LOCAL    REMOTE   PROTO\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t  stNeedBufferSize = 0;
        struct raw_pcb *pcb;
        
        LOCK_TCPIP_CORE();
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0) {
                stNeedBufferSize += 64;
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cRawInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cRawInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6 == 0) {
                __procFsNetRawPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        UNLOCK_TCPIP_CORE();
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
** ��������: __procFsNetRaw6Read
** ��������: procfs ��һ����ȡ���� raw �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetRaw6Read (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
    const CHAR      cRawInfoHdr[] = 
    "LOCAL                            REMOTE                           PROTO\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t  stNeedBufferSize = 0;
        struct raw_pcb *pcb;
        
        LOCK_TCPIP_CORE();
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6) {
                stNeedBufferSize += 128;
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cRawInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cRawInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (pcb->isipv6) {
                __procFsNetRawPrint(pcb, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        UNLOCK_TCPIP_CORE();
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
** ��������: __procFsNetIgmpPrint
** ��������: ��ӡ���� igmp �ļ�
** �䡡��  : group         group ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_LWIP_IGMP > 0

static VOID  __procFsNetIgmpPrint (struct igmp_group *group, PCHAR  pcBuffer, 
                                  size_t  stTotalSize, size_t *pstOft)
{
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%c%c%d: %08X %d\n",
                       group->netif->name[0],
                       group->netif->name[1],
                       group->netif->num,
                       group->group_address.addr,
                       (u32_t)group->use);
}

#endif                                                                  /*  LW_CFG_LWIP_IGMP > 0        */
/*********************************************************************************************************
** ��������: __procFsNetIgmpRead
** ��������: procfs ��һ����ȡ���� igmp �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetIgmpRead (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
#if LW_CFG_LWIP_IGMP > 0
    const CHAR      cIgmpInfoHdr[] = 
    "DEV  GROUP    COUNT\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t  stNeedBufferSize = 0;
        struct igmp_group *group;
        
        LOCK_TCPIP_CORE();
        for (group = igmp_group_list; group != NULL; group = group->next) {
            stNeedBufferSize += 64;
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cIgmpInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cIgmpInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (group = igmp_group_list; group != NULL; group = group->next) {
            __procFsNetIgmpPrint(group, pcFileBuffer, stNeedBufferSize, &stRealSize);
        }
        UNLOCK_TCPIP_CORE();
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
#else
    return  (0);
#endif                                                                  /*  LW_CFG_LWIP_IGMP > 0        */
}
/*********************************************************************************************************
** ��������: __procFsNetIgmp6Print
** ��������: ��ӡ���� igmp6 �ļ�
** �䡡��  : group         mld_group ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_LWIP_IGMP > 0

static VOID  __procFsNetIgmp6Print (struct mld_group *group, PCHAR  pcBuffer, 
                                    size_t  stTotalSize, size_t *pstOft)
{
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%c%c%d: %08X%08X%08X%08X %d\n",
                       group->netif->name[0],
                       group->netif->name[1],
                       group->netif->num,
                       group->group_address.addr[0],
                       group->group_address.addr[1],
                       group->group_address.addr[2],
                       group->group_address.addr[3],
                       (u32_t)group->use);
}

#endif                                                                  /*  LW_CFG_LWIP_IGMP > 0        */
/*********************************************************************************************************
** ��������: __procFsNetIgmp6Read
** ��������: procfs ��һ����ȡ���� igmp6 �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetIgmp6Read (PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft)
{
#if LW_CFG_LWIP_IGMP > 0
    const CHAR      cIgmp6InfoHdr[] = 
    "DEV  GROUP                            COUNT\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t  stNeedBufferSize = 0;
        struct mld_group *group;
        
        LOCK_TCPIP_CORE();
        for (group = mld_group_list; group != NULL; group = group->next) {
            stNeedBufferSize += 128;
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cIgmp6InfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cIgmp6InfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (group = mld_group_list; group != NULL; group = group->next) {
            __procFsNetIgmp6Print(group, pcFileBuffer, stNeedBufferSize, &stRealSize);
        }
        UNLOCK_TCPIP_CORE();
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
#else
    return  (0);
#endif                                                                  /*  LW_CFG_LWIP_IGMP > 0        */
}
/*********************************************************************************************************
** ��������: __procFsNetTcpipStatPrintProto
** ��������: ��ӡ���� tcpip_stat �ļ�Э�鲿��
** �䡡��  : proto         Э��
**           name          ����
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetTcpipStatPrintProto (struct stats_proto *proto, const char *name,
                                             PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{      
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-9s %-8u %-8u %-8u %-8u %-8u %-8u %-8u %-8u %-8u %-8u %-8u %-8u\n",
                       name, proto->xmit, proto->recv, proto->fw, proto->drop, proto->chkerr, 
                       proto->lenerr, proto->memerr, proto->rterr, proto->proterr, proto->opterr,
                       proto->err, proto->cachehit);
}
/*********************************************************************************************************
** ��������: __procFsNetTcpipStatPrintIgmp
** ��������: ��ӡ���� tcpip_stat �ļ� igmp ����
** �䡡��  : igmp          �鲥��Ϣ
**           name          ����
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_LWIP_IGMP > 0

static VOID  __procFsNetTcpipStatPrintIgmp (struct stats_igmp *igmp, const char *name,
                                            PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{                 
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-9s %-8u %-8u %-8u %-8u %-8u %-8u %-8u %-8u %-8u %-10u %-9u %-8u %-8u %-9u\n",
                       name, igmp->xmit, igmp->recv, igmp->drop, igmp->chkerr, igmp->lenerr, 
                       igmp->memerr, igmp->proterr, igmp->rx_v1, igmp->rx_group, igmp->rx_general, 
                       igmp->rx_report, igmp->tx_join, igmp->tx_leave, igmp->tx_report);
}

#endif                                                                  /*  LW_CFG_LWIP_IGMP > 0        */
/*********************************************************************************************************
** ��������: __procFsNetTcpipStatPrintMem
** ��������: ��ӡ���� tcpip_stat �ļ� mem ����
** �䡡��  : mem           �ڴ���Ϣ
**           name          ����
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetTcpipStatPrintMem (struct stats_mem *mem, const char *name,
                                           PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{                 
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-15s %-8u %-8u %-8u %-8u\n",
                       name, (u32_t)mem->avail, (u32_t)mem->used, (u32_t)mem->max, (u32_t)mem->err);
}
/*********************************************************************************************************
** ��������: __procFsNetTcpipStatPrintSys
** ��������: ��ӡ���� tcpip_stat �ļ� mem ����
** �䡡��  : sys           ϵͳ��Ϣ
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetTcpipStatPrintSys (struct stats_sys *sys,
                                           PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "\n%-9s %-8s %-8s %-8s\n",
                       "semaphore", "used", "max", "error");
                        
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-9s %-8u %-8u %-8u\n",
                       "", (u32_t)sys->sem.used, (u32_t)sys->sem.max, (u32_t)sys->sem.err);
                        
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-9s %-8s %-8s %-8s\n",
                       "mutex", "used", "max", "error");
                        
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-9s %-8u %-8u %-8u\n",
                       "", (u32_t)sys->mutex.used, (u32_t)sys->mutex.max, (u32_t)sys->mutex.err);
                        
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-9s %-8s %-8s %-8s\n",
                       "mbox", "used", "max", "error");
                        
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-9s %-8u %-8u %-8u\n",
                       "", (u32_t)sys->mbox.used, (u32_t)sys->mbox.max, (u32_t)sys->mbox.err);
}
/*********************************************************************************************************
** ��������: __procFsNetTcpipStatPrint
** ��������: ��ӡ���� tcpip_stat �ļ�
** �䡡��  : pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetTcpipStatPrint (PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    INT  i;
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-9s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s\n",
                       "proto", "xmit", "recv", "fw", "drop", "chkerr", "lenerr", "memerr", "rterr",
                       "proterr", "opterr", "err", "cachehit");

    __procFsNetTcpipStatPrintProto(&lwip_stats.link,     "LINK",      pcBuffer, stTotalSize, pstOft);
    __procFsNetTcpipStatPrintProto(&lwip_stats.etharp,   "ETHARP",    pcBuffer, stTotalSize, pstOft);
    
#if LW_CFG_LWIP_IPFRAG > 0
    __procFsNetTcpipStatPrintProto(&lwip_stats.ip_frag,  "IP_FRAG",   pcBuffer, stTotalSize, pstOft);
    __procFsNetTcpipStatPrintProto(&lwip_stats.ip6_frag, "IPv6_FRAG", pcBuffer, stTotalSize, pstOft);
#endif                                                                  /*  LW_CFG_LWIP_IPFRAG > 0      */

    __procFsNetTcpipStatPrintProto(&lwip_stats.ip,       "IP",        pcBuffer, stTotalSize, pstOft);
    __procFsNetTcpipStatPrintProto(&lwip_stats.nd6,      "ND",        pcBuffer, stTotalSize, pstOft);
    __procFsNetTcpipStatPrintProto(&lwip_stats.ip6,      "IPv6",      pcBuffer, stTotalSize, pstOft);
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "\n%-9s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-10s %-9s %-8s %-8s %-9s\n",
                       "name", "xmit", "recv", "drop", "chkerr", "lenerr", "memerr", "proterr", 
                       "rx_v1", "rx_group", "rx_general", "rx_report", 
                       "tx_join", "tx_leave", "tx_report");

#if LW_CFG_LWIP_IGMP > 0
    __procFsNetTcpipStatPrintIgmp(&lwip_stats.igmp,      "IGMP",      pcBuffer, stTotalSize, pstOft);
    __procFsNetTcpipStatPrintIgmp(&lwip_stats.mld6,      "MLDv1",     pcBuffer, stTotalSize, pstOft);
#endif
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "\n%-9s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s\n",
                       "name", "xmit", "recv", "fw", "drop", "chkerr", "lenerr", "memerr", "rterr",
                       "proterr", "opterr", "err", "cachehit");
                        
    __procFsNetTcpipStatPrintProto(&lwip_stats.icmp,     "ICMP",      pcBuffer, stTotalSize, pstOft);
    __procFsNetTcpipStatPrintProto(&lwip_stats.icmp6,    "ICMPv6",    pcBuffer, stTotalSize, pstOft);
    
    __procFsNetTcpipStatPrintProto(&lwip_stats.udp,      "UDP",       pcBuffer, stTotalSize, pstOft);
    __procFsNetTcpipStatPrintProto(&lwip_stats.tcp,      "TCP",       pcBuffer, stTotalSize, pstOft);
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "\n%-15s %-8s %-8s %-8s %-8s\n",
                       "name", "avail", "used", "max", "err");
                        
    __procFsNetTcpipStatPrintMem(&lwip_stats.mem, "HEAP", pcBuffer, stTotalSize, pstOft);
    
    for (i = 0; i < MEMP_MAX; i++) {
        char *memp_names[] = {
#define LWIP_MEMPOOL(name,num,size,desc) desc,
#include "lwip/memp_std.h"
        };
        __procFsNetTcpipStatPrintMem(&lwip_stats.memp[i], memp_names[i], pcBuffer, stTotalSize, pstOft);
    }
    
    __procFsNetTcpipStatPrintSys(&lwip_stats.sys, pcBuffer, stTotalSize, pstOft);
}
/*********************************************************************************************************
** ��������: __procFsNetTcpipStatRead
** ��������: procfs ��һ����ȡ���� tcpip_stat �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetTcpipStatRead (PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft)
{
    PCHAR     pcFileBuffer;
    size_t    stRealSize;                                               /*  ʵ�ʵ��ļ����ݴ�С          */
    size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        LOCK_TCPIP_CORE();
        __procFsNetTcpipStatPrint(pcFileBuffer, 
                                  p_pfsn->PFSN_pfsnmMessage.PFSNM_stBufferSize, 
                                  &stRealSize);
        UNLOCK_TCPIP_CORE();
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** ��������: __procFsNetRoutePrint
** ��������: ��ӡ���� route �ļ�
** �䡡��  : msgbuf        ·�ɱ���Ϣ
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetRoutePrint (struct route_msg *msgbuf,
                                    PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cIpDest[INET_ADDRSTRLEN];
    CHAR    cGateway[INET_ADDRSTRLEN] = "*";
    CHAR    cMask[INET_ADDRSTRLEN]    = "*";
    CHAR    cFlag[6] = "\0";
    
    inet_ntoa_r(msgbuf->rm_dst, cIpDest, INET_ADDRSTRLEN);
    if (msgbuf->rm_if.s_addr != INADDR_ANY) {
        inet_ntoa_r(msgbuf->rm_gw, cGateway, INET_ADDRSTRLEN);
        inet_ntoa_r(msgbuf->rm_mask, cMask, INET_ADDRSTRLEN);
    }
    
    if (msgbuf->rm_flag & ROUTE_RTF_UP) {
        lib_strcat(cFlag, "U");
    }
    if (msgbuf->rm_flag & ROUTE_RTF_GATEWAY) {
        lib_strcat(cFlag, "G");
    } else {
        lib_strcpy(cGateway, "*");                                      /*  ֱ��·�� (����Ҫ��ӡ����)   */
    }
    if (msgbuf->rm_flag & ROUTE_RTF_HOST) {
        lib_strcat(cFlag, "H");
    }

    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-18s %-18s %-18s %-8s %6d %-3s\n",
                       cIpDest, cGateway, cMask, cFlag, msgbuf->rm_metric, msgbuf->rm_ifname);
}
/*********************************************************************************************************
** ��������: __procFsNetRouteRead
** ��������: procfs ��һ����ȡ���� igmp6 �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetRouteRead (PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft)
{
    const CHAR      cRouteInfoHdr[] = 
    "destination        gateway            mask               flag     metric interface\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t              stNeedBufferSize = 0;
        INT                 i;
        INT                 iRouteNum;
        struct route_msg   *msgbuf;
        
        iRouteNum = route_getnum();
        stNeedBufferSize = (iRouteNum * 128) + sizeof(cRouteInfoHdr);
        
        msgbuf = (struct route_msg *)__SHEAP_ALLOC(sizeof(struct route_msg) * (size_t)iRouteNum);
        if (!msgbuf) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        
        iRouteNum = route_get(0, msgbuf, (size_t)iRouteNum);
        if (iRouteNum <= 0) {
            __SHEAP_FREE(msgbuf);
            return  (0);
        }
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            __SHEAP_FREE(msgbuf);
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cRouteInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        for (i = 0; i < iRouteNum; i++) {
            __procFsNetRoutePrint(msgbuf + i, pcFileBuffer, stNeedBufferSize, &stRealSize);
        }
                                                                        
        __SHEAP_FREE(msgbuf);
        
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
** ��������: __procFsNetUnixGetCnt
** ��������: ��ȡ���� unix �ļ�����
** �䡡��  : pafunix       unix �ļ�
**           pstBufferSize ��������С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_NET_UNIX_EN > 0

static VOID  __procFsNetUnixGetCnt (AF_UNIX_T  *pafunix, size_t  *pstNeedBufferSize)
{
    *pstNeedBufferSize += lib_strlen(pafunix->UNIX_cFile) + 70;
}
/*********************************************************************************************************
** ��������: __procFsNetUnixPrint
** ��������: ��ӡ���� unix �ļ�
** �䡡��  : pafunix       unix �ļ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetUnixPrint (AF_UNIX_T  *pafunix, PCHAR  pcBuffer, 
                                   size_t  stTotalSize, size_t *pstOft)
{
    PCHAR   pcType;
    PCHAR   pcStat;
    PCHAR   pcShut;
    
    if (stTotalSize > *pstOft) {
        if (pafunix->UNIX_iType == SOCK_STREAM) {
            pcType = "stream";
        } else if (pafunix->UNIX_iType == SOCK_SEQPACKET) {
            pcType = "seqpacket";
        } else {
            pcType = "dgram";
        }
        
        switch (pafunix->UNIX_iStatus) {
        case __AF_UNIX_STATUS_NONE:    pcStat = "none";    break;
        case __AF_UNIX_STATUS_LISTEN:  pcStat = "listen";  break;
        case __AF_UNIX_STATUS_CONNECT: pcStat = "connect"; break;
        case __AF_UNIX_STATUS_ESTAB:   pcStat = "estab";   break;
        default:                       pcStat = "unknown"; break;
        }
        
        if ((pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_R) && 
            (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_W)) {
            pcShut = "rw";
        } else if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_R) {
            pcShut = "r";
        } else if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_W) {
            pcShut = "w";
        } else {
            pcShut = "no";
        }
        
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, "%-9s %4x %-7s %-5s %10zu %10zu %s\n",
                           pcType, pafunix->UNIX_iFlag, pcStat, pcShut, 
                           pafunix->UNIX_unixq.UNIQ_stTotal,
                           pafunix->UNIX_stMaxBufSize,
                           pafunix->UNIX_cFile);
    }
}

#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
/*********************************************************************************************************
** ��������: __procFsNetUnixRead
** ��������: procfs ��һ����ȡ���� unix �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetUnixRead (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
#if LW_CFG_NET_UNIX_EN > 0
    const CHAR      cUnixInfoHdr[] = 
    "TYPE      FLAG STATUS  SHUTD      NREAD MAX_BUFFER PATH\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t      stNeedBufferSize = 0;
        
        unix_traversal(__procFsNetUnixGetCnt, (PVOID)&stNeedBufferSize, 
                       LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);    /*  ������Ҫ�Ļ�������С        */
        stNeedBufferSize += sizeof(cUnixInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cUnixInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        unix_traversal(__procFsNetUnixPrint, pcFileBuffer, 
                       (PVOID)stNeedBufferSize, (PVOID)&stRealSize, 
                       LW_NULL, LW_NULL, LW_NULL);
        
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
    
#else
    return  (0);
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
}
/*********************************************************************************************************
** ��������: __procFsNetGetIfFlag
** ��������: �������ӿ� flag
** �䡡��  : netif         �����豸
**           pcFlag        ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetGetIfFlag (struct netif *netif, PCHAR  pcFlag)
{
    pcFlag[0] = PX_EOS;

    if (netif->flags & NETIF_FLAG_UP) {
        lib_strcat(pcFlag, "U");
    }
    if (netif->flags & NETIF_FLAG_BROADCAST) {
        lib_strcat(pcFlag, "B");
    }
    if (netif->flags & NETIF_FLAG_POINTTOPOINT) {
        lib_strcat(pcFlag, "P");
    }
    if (netif->flags2 & NETIF_FLAG2_DHCP) {
        lib_strcat(pcFlag, "D");
    }
    if (netif->flags & NETIF_FLAG_LINK_UP) {
        lib_strcat(pcFlag, "L");
    }
    if (netif->flags & NETIF_FLAG_ETHARP) {
        lib_strcat(pcFlag, "Eth");
    }
    if (netif->flags & NETIF_FLAG_IGMP) {
        lib_strcat(pcFlag, "G");
    }
}
/*********************************************************************************************************
** ��������: __procFsNetDevPrint
** ��������: ��ӡ���� dev �ļ�
** �䡡��  : netif         �����豸
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetDevPrint (struct netif *netif, PCHAR  pcBuffer, 
                                  size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cFlag[10];
    
    __procFsNetGetIfFlag(netif, cFlag);
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%c%c%d: %-6u %-10u %-5u %-6u %-6u %-6u    %-10u %-5u %-6u %-6u %-6u %s\n",
                       netif->name[0], netif->name[1], netif->num,
                       netif->mtu,
                       netif->ifinoctets,
                       netif->ifinucastpkts + netif->ifinnucastpkts,
                       0, netif->ifindiscards, 0,
                       netif->ifoutoctets,
                       netif->ifoutucastpkts + netif->ifoutnucastpkts,
                       0, netif->ifoutdiscards, 0, 
                       cFlag);
}
/*********************************************************************************************************
** ��������: __procFsNetDevRead
** ��������: procfs ��һ����ȡ���� dev �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetDevRead (PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft)
{
    const CHAR      cDevInfoHdr[] = 
    "           |RECEIVE                                 |TRANSMIT\n"
    "FACE MTU    RX-BYTES   RX-OK RX-ERR RX-DRP RX-OVR    TX-BYTES   TX-OK TX-ERR TX-DRP TX-OVR FLAG\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t        stNeedBufferSize = 0;
        struct netif *netif;
        
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            stNeedBufferSize += 128;
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cDevInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cDevInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            __procFsNetDevPrint(netif, pcFileBuffer, stNeedBufferSize, &stRealSize);
        }
        UNLOCK_TCPIP_CORE();
        
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
** ��������: __procFsNetDevPrint
** ��������: ��ӡ���� if_inet6 �ļ�
** �䡡��  : netif         �����豸
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetIfInet6Print (struct netif *netif, PCHAR  pcBuffer, 
                                      size_t  stTotalSize, size_t *pstOft)
{
    INT     i;
    CHAR    cFlag[10];
    
    __procFsNetGetIfFlag(netif, cFlag);
    
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif->ip6_addr_state[i])) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%08X%08X%08X%08X %-5d %-9s %-5s %-9s %c%c%d\n",
                               netif->ip6_addr[i].addr[0],
                               netif->ip6_addr[i].addr[1],
                               netif->ip6_addr[i].addr[2],
                               netif->ip6_addr[i].addr[3],
                               netif->num,
                               "--",
                               "--",
                               cFlag,
                               netif->name[0], netif->name[1], netif->num);
        }
    }
}
/*********************************************************************************************************
** ��������: __procFsNetIfInet6Read
** ��������: procfs ��һ����ȡ���� if_inet6 �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetIfInet6Read (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    const CHAR      cIfInet6InfoHdr[] = 
    "INET6                            INDEX REFIX-LEN SCOPE FLAG      FACE\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t        stNeedBufferSize = 0;
        struct netif *netif;
        INT           i;
        
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                if (ip6_addr_isvalid(netif->ip6_addr_state[i])) {
                    stNeedBufferSize += 128;
                }
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cIfInet6InfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cIfInet6InfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
                                                                        
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            __procFsNetIfInet6Print(netif, pcFileBuffer, stNeedBufferSize, &stRealSize);
        }
        UNLOCK_TCPIP_CORE();
        
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
** ��������: __procFsNetArpPrint
** ��������: ��ӡ���� arp �ļ�
** �䡡��  : netif         ����ӿ�
**           ipaddr        arp ip ��ַ
**           ethaddr       arp mac ��ַ
**           iIsStatic     �Ƿ�Ϊ��̬ת����ϵ
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetArpPrint (struct netif      *netif, 
                                  ip_addr_t         *ipaddr, 
                                  struct eth_addr   *ethaddr,
                                  INT                iIsStatic, 
                                  PCHAR              pcBuffer, 
                                  size_t             stTotalSize, 
                                  size_t            *pstOft)
{
    CHAR    cBuffer[INET_ADDRSTRLEN];

    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%c%c%d  %-16s %02x:%02x:%02x:%02x:%02x:%02x %s\n",
                       netif->name[0], netif->name[1], netif->num,
                       ipaddr_ntoa_r(ipaddr, cBuffer, INET_ADDRSTRLEN),
                       ethaddr->addr[0],
                       ethaddr->addr[1],
                       ethaddr->addr[2],
                       ethaddr->addr[3],
                       ethaddr->addr[4],
                       ethaddr->addr[5],
                       (iIsStatic) ? "static" : "dynamic");
}
/*********************************************************************************************************
** ��������: __procFsNetArpRead
** ��������: procfs ��һ����ȡ���� arp �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetArpRead (PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft)
{
    extern void etharp_traversal(struct netif *netif, 
                                 void (*callback)(),
                                 void *arg0,
                                 void *arg1,
                                 void *arg2,
                                 void *arg3,
                                 void *arg4,
                                 void *arg5);
    
    const CHAR      cArpInfoHdr[] = 
    "FACE INET ADDRESS     PHYSICAL ADDRESS  TYPE\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С(�����ڵ�ʱԤ�ô�СΪ 64 �ֽ�).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        struct netif *netif;
        
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_ARP, 0, cArpInfoHdr);
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            if (netif->flags & NETIF_FLAG_ETHARP) {
                etharp_traversal(netif, __procFsNetArpPrint, 
                                 (PVOID)pcFileBuffer, 
                                 (PVOID)__PROCFS_BUFFER_SIZE_ARP, 
                                 (PVOID)&stRealSize,
                                 LW_NULL, LW_NULL, LW_NULL);
            }
        }
        UNLOCK_TCPIP_CORE();
    
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** ��������: __procFsNetAodvRead
** ��������: procfs ��һ����ȡ���� adhoc-aodv �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetAodvRead (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С(�����ڵ�ʱԤ�ô�СΪ 64 �ֽ�).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_AODV, 0,
                              "ACTIVE_ROUTE_TIMEOUT : %d\n"
                              "ALLOWED_HELLO_LOSS   : %d\n"
                              "BLACKLIST_TIMEOUT    : %d\n"
                              "DELETE_PERIOD        : %d\n"
                              "HELLO_INTERVAL       : %d\n"
                              "LOCAL_ADD_TTL        : %d\n"
                              "MAX_REPAIR_TTL       : %d\n"
                              "MIN_REPAIR_TTL       : %d\n"
                              "MY_ROUTE_TIMEOUT     : %d\n"
                              "NET_DIAMETER         : %d\n"
                              "NET_TRAVERSAL_TIME   : %d\n"
                              "NEXT_HOP_WAIT        : %d\n"
                              "NODE_TRAVERSAL_TIME  : %d\n"
                              "PATH_DISCOVERY_TIME  : %d\n"
                              "RERR_RATELIMIT       : %d\n"
                              "RREQ_RETRIES         : %d\n"
                              "RREQ_RATELIMIT       : %d\n"
                              "TIMEOUT_BUFFER       : %d\n"
                              "TTL_START            : %d\n"
                              "TTL_INCREMENT        : %d\n"
                              "TTL_THRESHOLD        : %d\n"
                              "AODV_MCAST           : %d\n"
                              "GROUP_HELLO_INTERVAL : %d\n"
                              "RREP_WAIT_TIME       : %d\n"
                              "PRUNE_TIMEOUT        : %d\n"
                              "AODV_TIMER_PRECISION : %d\n"
                              "AODV_MAX_NETIF       : %d\n"
                              "AODV_RECV_N_HELLOS   : %d\n"
                              "AODV_HELLO_NEIGHBOR  : %d\n",
                              ACTIVE_ROUTE_TIMEOUT,
                              ALLOWED_HELLO_LOSS,
                              BLACKLIST_TIMEOUT,
                              DELETE_PERIOD,
                              HELLO_INTERVAL,
                              LOCAL_ADD_TTL,
                              MAX_REPAIR_TTL,
                              MIN_REPAIR_TTL,
                              MY_ROUTE_TIMEOUT,
                              NET_DIAMETER,
                              NET_TRAVERSAL_TIME,
                              NEXT_HOP_WAIT,
                              NODE_TRAVERSAL_TIME,
                              PATH_DISCOVERY_TIME,
                              RERR_RATELIMIT,
                              RREQ_RETRIES,
                              RREQ_RATELIMIT,
                              TIMEOUT_BUFFER,
                              TTL_START,
                              TTL_INCREMENT,
                              TTL_THRESHOLD,
                              AODV_MCAST,
#if LW_CFG_LWIP_IGMP > 0
                              GROUP_HELLO_INTERVAL,
                              RREP_WAIT_TIME,
                              PRUNE_TIMEOUT,
#else
                              0,
                              0,
                              0,
#endif
                              AODV_TIMER_PRECISION,
                              AODV_MAX_NETIF,
                              AODV_RECV_N_HELLOS,
                              AODV_HELLO_NEIGHBOR);
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
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
** ��������: __procFsNetAodvRtGetCnt
** ��������: ��ȡ���� aodv_rt �ļ�����
** �䡡��  : rt            aodv ·�ɽڵ�
**           pstBufferSize ��������С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetAodvRtGetCnt (struct aodv_rtnode *rt, size_t  *pstNeedBufferSize)
{
    (VOID)rt;

    *pstNeedBufferSize += 80;
}
/*********************************************************************************************************
** ��������: __procFsNetAodvRtPrint
** ��������: ��ӡ���� aodv_rt �ļ�
** �䡡��  : rt            aodv ·�ɽڵ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetAodvRtPrint (struct aodv_rtnode *rt, PCHAR  pcBuffer, 
                                     size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cIpDest[INET_ADDRSTRLEN];
    CHAR    cNextHop[INET_ADDRSTRLEN];
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
    if_indextoname(rt->netif->num, cIfName);
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-18s %-18s %-8s %6d %-3s\n",
                       cIpDest, cNextHop, cFlag, rt->hcnt, cIfName);
}
/*********************************************************************************************************
** ��������: __procFsNetAodvRtRead
** ��������: procfs ��һ����ȡ���� aodv_rt �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetAodvRtRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    const CHAR      cAodvRtInfoHdr[] = 
    "destination        nexthop            flag     metric interface\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t      stNeedBufferSize = 0;
        
        LOCK_TCPIP_CORE();
        aodv_rt_traversal((aodv_rt_hook_handler)__procFsNetAodvRtGetCnt, 
                          (PVOID)&stNeedBufferSize, 
                          LW_NULL, LW_NULL, LW_NULL, LW_NULL);          /*  ������Ҫ�Ļ�������С        */
        UNLOCK_TCPIP_CORE();
                       
        stNeedBufferSize += sizeof(cAodvRtInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cAodvRtInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        
        LOCK_TCPIP_CORE();
        aodv_rt_traversal((aodv_rt_hook_handler)__procFsNetAodvRtPrint, pcFileBuffer, 
                       (PVOID)stNeedBufferSize, (PVOID)&stRealSize, 
                       LW_NULL, LW_NULL);
        UNLOCK_TCPIP_CORE();
        
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
** ��������: __procFsNetPacketGetCnt
** ��������: ��ȡ���� unix �ļ�����
** �䡡��  : pafpacket     packet �ļ�
**           pstBufferSize ��������С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetPacketGetCnt (AF_PACKET_T *pafpacket, size_t  *pstNeedBufferSize)
{
    *pstNeedBufferSize += 64;
}
/*********************************************************************************************************
** ��������: __procFsNetPacketPrint
** ��������: ��ӡ���� packet �ļ�
** �䡡��  : pafpacket     packet �ļ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetPacketPrint (AF_PACKET_T *pafpacket, PCHAR  pcBuffer, 
                                     size_t  stTotalSize, size_t *pstOft)
{
    PCHAR   pcType;


    if (stTotalSize > *pstOft) {
        if (pafpacket->PACKET_iType == SOCK_RAW) {
            pcType = "raw";
        } else {
            pcType = "dgram";
        }
    
#if LW_CFG_NET_PACKET_MMAP > 0
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, "%-9s %4x %8x %5d %-4s %9x %-8d %-4d\n",
                           pcType, pafpacket->PACKET_iFlag, pafpacket->PACKET_iProtocol,
                           pafpacket->PACKET_iIfIndex, 
                           (pafpacket->PACKET_bMmap) ? "yes" : "no",
                           pafpacket->PACKET_mmapRx.PKTB_stSize,
                           pafpacket->PACKET_stats.tp_packets,
                           pafpacket->PACKET_stats.tp_drops);
#else
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, "%-9s %4x %8x %5d %-4s %9x %-8d %-4d\n",
                           pcType, pafpacket->PACKET_iFlag, pafpacket->PACKET_iProtocol,
                           pafpacket->PACKET_iIfIndex, 
                           "no", 0,
                           pafpacket->PACKET_stats.tp_packets,
                           pafpacket->PACKET_stats.tp_drops);
#endif
    }
}
/*********************************************************************************************************
** ��������: __procFsNetPacketRead
** ��������: procfs ��һ����ȡ���� packet �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetPacketRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    const CHAR      cPacketInfoHdr[] = 
    "TYPE      FLAG PROTOCOL INDEX MMAP MMAP_SIZE TOTAL    DROP\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t      stNeedBufferSize = 0;
        
        packet_traversal(__procFsNetPacketGetCnt, (PVOID)&stNeedBufferSize, 
                         LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);  /*  ������Ҫ�Ļ�������С        */
        stNeedBufferSize += sizeof(cPacketInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cPacketInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        packet_traversal(__procFsNetPacketPrint, pcFileBuffer, 
                         (PVOID)stNeedBufferSize, (PVOID)&stRealSize, 
                         LW_NULL, LW_NULL, LW_NULL);
        
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
** ��������: __procFsNetPppPrint
** ��������: ��ӡ���� ppp �ļ�
** �䡡��  : netif         �����豸
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_LWIP_PPP > 0

typedef struct {
#define PPP_OS          0
#define PPP_OE          1
#define PPP_OL2TP       2
    UINT                CTXP_uiType;
} PPP_CTX_PRIV;

static VOID  __procFsNetPppPrint (struct netif *netif, PCHAR  pcBuffer, 
                                  size_t  stTotalSize, size_t *pstOft)
{
    PPP_CTX_PRIV *pctxp;
    ppp_pcb      *pcb;
    
    PCHAR     pcType;
    PCHAR     pcPhase;
    
    pcb   = _LIST_ENTRY(netif, ppp_pcb, netif);
    pctxp = (PPP_CTX_PRIV *)pcb->ctx_cb;
    if (pctxp->CTXP_uiType == PPP_OS) {
        pcType = "PPPoS";
    } else if (pctxp->CTXP_uiType == PPP_OE) {
        pcType = "PPPoE";
    } else {
        pcType = "PPPoL2TP";
    }
    
    switch (pcb->phase) {
    
    case PPP_PHASE_DEAD:            pcPhase = "dead";           break;
    case PPP_PHASE_INITIALIZE:      pcPhase = "initialize";     break;
    case PPP_PHASE_SERIALCONN:      pcPhase = "serialconn";     break;
    case PPP_PHASE_DORMANT:         pcPhase = "dormant";        break;
    case PPP_PHASE_ESTABLISH:       pcPhase = "establish";      break;
    case PPP_PHASE_AUTHENTICATE:    pcPhase = "authenticate";   break;
    case PPP_PHASE_CALLBACK:        pcPhase = "callback";       break;
    case PPP_PHASE_NETWORK:         pcPhase = "network";        break;
    case PPP_PHASE_RUNNING:         pcPhase = "running";        break;
    case PPP_PHASE_TERMINATE:       pcPhase = "terminate";      break;
    case PPP_PHASE_DISCONNECT:      pcPhase = "disconnect";     break;
    case PPP_PHASE_HOLDOFF:         pcPhase = "holdoff";        break;
    case PPP_PHASE_MASTER:          pcPhase = "master";         break;
    default:                        pcPhase = "<unknown>";      break;
    }
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%c%c%d  %-8s %-6u %-12s %-12u %-12u\n",
                       netif->name[0], netif->name[1], netif->num,
                       pcType,
                       netif->mtu,
                       pcPhase,
                       netif->ifinoctets,
                       netif->ifoutoctets);
}

#endif                                                                  /*  LW_CFG_LWIP_PPP > 0         */
/*********************************************************************************************************
** ��������: __procFsNetPppRead
** ��������: procfs ��һ����ȡ���� ppp �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetPppRead (PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft)
{
#if LW_CFG_LWIP_PPP > 0
    const CHAR      cPppInfoHdr[] = 
    "FACE TYPE     MTU    PHASE        RX-BYTES     TX-BYTES\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t        stNeedBufferSize = 0;
        struct netif *netif;
        
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            if (netif->flags & NETIF_FLAG_POINTTOPOINT) {
                stNeedBufferSize += 64;
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cPppInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cPppInfoHdr); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            if (netif->flags & NETIF_FLAG_POINTTOPOINT) {
                __procFsNetPppPrint(netif, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        UNLOCK_TCPIP_CORE();
        
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
#else
    return  (0);
#endif                                                                  /*  LW_CFG_LWIP_PPP > 0         */
}
/*********************************************************************************************************
** ��������: __procFsNetWlPrint
** ��������: ��ӡ���� wireless �ļ�
** �䡡��  : netif         �����豸
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_NET_WIRELESS_EN > 0

static VOID  __procFsNetWlPrint (struct netif *netif, PCHAR  pcBuffer, 
                                 size_t  stTotalSize, size_t *pstOft)
{
extern struct iw_statistics *get_wireless_stats(struct netif *);

    struct iw_statistics *stats = get_wireless_stats(netif);
    
    if (!stats) {
        return;
    }
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%c%c%d: %04x  %3d%c  %3d%c  %3d%c  %6d %6d %6d "
                       "%6d %6d   %6d\n",
                       netif->name[0], netif->name[1], netif->num,
                       stats->status, stats->qual.qual,
                       stats->qual.updated & IW_QUAL_QUAL_UPDATED
        			   ? '.' : ' ',
        			   ((s32) stats->qual.level) -
        			   ((stats->qual.updated & IW_QUAL_DBM) ? 0x100 : 0),
        			   stats->qual.updated & IW_QUAL_LEVEL_UPDATED
        			   ? '.' : ' ',
        			   ((s32) stats->qual.noise) -
        			   ((stats->qual.updated & IW_QUAL_DBM) ? 0x100 : 0),
        			   stats->qual.updated & IW_QUAL_NOISE_UPDATED
        			   ? '.' : ' ',
        			   stats->discard.nwid, stats->discard.code,
        			   stats->discard.fragment, stats->discard.retries,
        			   stats->discard.misc, stats->miss.beacon);
}

#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
** ��������: __procFsNetWlRead
** ��������: procfs ��һ����ȡ���� wireless �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetWlRead (PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft)
{
#if LW_CFG_NET_WIRELESS_EN > 0
    const CHAR      cWlInfoHdr[] = 
    "Inter-| sta-|   Quality        |   Discarded "
	"packets               | Missed | WE\n"
	" face | tus | link level noise |  nwid  "
	"crypt   frag  retry   misc | beacon | %d\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t        stNeedBufferSize = 0;
        struct netif *netif;
        
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            if (netif->wireless_handlers) {
                stNeedBufferSize += 180;
            }
        }
        UNLOCK_TCPIP_CORE();
        
        stNeedBufferSize += sizeof(cWlInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cWlInfoHdr, WIRELESS_EXT);
                                                                        /*  ��ӡͷ��Ϣ                  */
        LOCK_TCPIP_CORE();
        for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
            if (netif->wireless_handlers) {
                __procFsNetWlPrint(netif, pcFileBuffer, stNeedBufferSize, &stRealSize);
            }
        }
        UNLOCK_TCPIP_CORE();
        
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
#else
    return  (0);
#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
}
/*********************************************************************************************************
** ��������: __procFsNetInit
** ��������: procfs ��ʼ������ proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsNetInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnNet[0],  "/");
    API_ProcFsMakeNode(&_G_pfsnNet[1],  "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[2],  "/net/mesh-adhoc");
    API_ProcFsMakeNode(&_G_pfsnNet[3],  "/net/mesh-adhoc");
    API_ProcFsMakeNode(&_G_pfsnNet[4],  "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[5],  "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[6],  "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[7],  "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[8],  "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[9],  "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[10], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[11], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[12], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[13], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[14], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[15], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[16], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[17], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[18], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[19], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[20], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[21], "/net");
    API_ProcFsMakeNode(&_G_pfsnNet[22], "/net");
}
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
