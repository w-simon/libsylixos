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
** ��   ��   ��: lwip_ping6.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 06 �� 07 ��
**
** ��        ��: lwip ipv6 ping ����. 

** BUG:
2011.06.07  2011��6��8���������׸�IPv6��, SylixOS��6��8��ǰ��ʼ֧��IPv6.
2014.02.24  �����ָ�� IPv6 ����ӿڵĲ���֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_PING_EN > 0)
#include "lwip/icmp.h"
#include "lwip/raw.h"
#include "lwip/tcpip.h"
#include "lwip/inet6.h"
#include "lwip/inet_chksum.h"
#include "lwip/netdb.h"
#include "sys/socket.h"
/*********************************************************************************************************
** ��������: __inetPing6FindSrc
** ��������: ȷ��Դ�˵�ַ
** �䡡��  : netif         ����ӿ� (NULL ��ʾ�Զ�ȷ���ӿ�)
**           pip6addrDest  Ŀ�ĵ�ַ
**           pip6addrSrc   �����ж�, �������õ� IPv6 Դ��ַ
** �䡡��  : -1: �޷�ȷ������ӿ�
**           -2: ·�ɴ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if CHECKSUM_GEN_ICMP6

static INT  __inetPing6FindSrc (struct netif    *netif, 
                                struct ip6_addr *pip6addrDest,
                                struct ip6_addr *pip6addrSrc)
{
    static struct ip6_addr  ip6addrAny = {{0, 0, 0, 0}};
           struct ip6_addr *pip6addr;

    if (netif == LW_NULL) {
        netif =  ip6_route(&ip6addrAny, pip6addrDest);
        if (netif == LW_NULL) {
            return  (-1);
        }
    }
    
    pip6addr = ip6_select_source_address(netif, pip6addrDest);
    if (pip6addr == NULL) {
        return  (-2);
    }
    
    *pip6addrSrc = *pip6addr;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  CHECKSUM_GEN_ICMP6          */
/*********************************************************************************************************
** ��������: __inetPing6Prepare
** ��������: ���� ping ��
** �䡡��  : icmp6hdrEcho  ����
**           pip6addrDest  Ŀ�� IP
**           iDataSize     ���ݴ�С
**           pcNetif       ����ӿ��� (NULL ��ʾ�Զ�ȷ���ӿ�)
**           pusSeqRecv    ��Ҫ�жϵ� seq
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetPing6Prepare (struct icmp6_echo_hdr   *icmp6hdrEcho, 
                                struct ip6_addr         *pip6addrDest,
                                INT                      iDataSize, 
                                CPCHAR                   pcNetif,
                                UINT16                  *pusSeqRecv)
{
    static u16_t    usSeqNum = 1;
    REGISTER INT    i;
    
#if CHECKSUM_GEN_ICMP6
           INT      iError;
    struct ip6_addr ip6addrSrc;
    struct pbuf     pbuf;
    struct netif   *netif;
#endif                                                                  /*  CHECKSUM_GEN_ICMP6          */
    
    SYS_ARCH_DECL_PROTECT(x);

    icmp6hdrEcho->type = ICMP6_TYPE_EREQ;
    icmp6hdrEcho->code = 0;
    
    *pusSeqRecv = usSeqNum;
    
    icmp6hdrEcho->chksum = 0;
    icmp6hdrEcho->id     = 0xAFAF;                                      /*  ID                          */
    icmp6hdrEcho->seqno  = htons(usSeqNum);
    
    /*
     *  �������
     */
    for(i = 0; i < iDataSize; i++) {
        ((PCHAR)icmp6hdrEcho)[sizeof(struct icmp6_echo_hdr) + i] = (CHAR)(i % 256);
    }
    
#if CHECKSUM_GEN_ICMP6
    LOCK_TCPIP_CORE();
    if (pcNetif) {
        netif = netif_find((PCHAR)pcNetif);
        if (netif == LW_NULL) {
            UNLOCK_TCPIP_CORE();
            fprintf(stderr, "Invalid interface.\n");
            return  (PX_ERROR);
        }
    } else {
        netif = LW_NULL;
    }
    iError = __inetPing6FindSrc(netif, pip6addrDest, &ip6addrSrc);
    UNLOCK_TCPIP_CORE();
    
    if (iError == -1) {
        fprintf(stderr, "You must determine net interface.\n");
        return  (PX_ERROR);
    
    } else if (iError == -2) {
        fprintf(stderr, "Unreachable destination.\n");
        return  (PX_ERROR);
    }
    
    pbuf.next    = LW_NULL;
    pbuf.payload = (void *)icmp6hdrEcho;
    pbuf.tot_len = (u16_t)(iDataSize + sizeof(struct icmp6_echo_hdr));
    pbuf.len     = pbuf.tot_len;
    pbuf.type    = PBUF_ROM;
    pbuf.flags   = 0;
    pbuf.ref     = 1;
    
    icmp6hdrEcho->chksum = ip6_chksum_pseudo(&pbuf, IP6_NEXTH_ICMP6, pbuf.tot_len,
                                             &ip6addrSrc, pip6addrDest);
#endif                                                                  /*  CHECKSUM_GEN_ICMP6          */
    
    SYS_ARCH_PROTECT(x);
    usSeqNum++;
    SYS_ARCH_UNPROTECT(x);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __inetPing6Send
** ��������: ���� ping ��
** �䡡��  : iSock         �׽���
**           pin6addr      Ŀ�� ip ��ַ.
**           iDataSize     ���ݴ�С
**           pcNetif       ����ӿ��� (NULL ��ʾ�Զ�ȷ���ӿ�)
**           pusSeqRecv    ��Ҫ�жϵ� seq
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetPing6Send (INT              iSock, 
                             struct in6_addr *pin6addr, 
                             INT              iDataSize, 
                             CPCHAR           pcNetif,
                             UINT16          *pusSeqRecv)
{
    REGISTER size_t                 stPingSize = sizeof(struct icmp6_echo_hdr) + iDataSize;
             struct icmp6_echo_hdr *icmp6hdrEcho;
             ssize_t                sstError;
             struct sockaddr_in6    sockaddrin6;
             struct ip6_addr        ip6addrDest;
             
    icmp6hdrEcho = (struct icmp6_echo_hdr *)__SHEAP_ALLOC(stPingSize);
    if (icmp6hdrEcho == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory\r\n");
        return  (PX_ERROR);
    }
    
    ip6addrDest.addr[0] = pin6addr->un.u32_addr[0];
    ip6addrDest.addr[1] = pin6addr->un.u32_addr[1];
    ip6addrDest.addr[2] = pin6addr->un.u32_addr[2];
    ip6addrDest.addr[3] = pin6addr->un.u32_addr[3];
    
    if (__inetPing6Prepare(icmp6hdrEcho, &ip6addrDest, iDataSize, pcNetif, pusSeqRecv) < ERROR_NONE) {
        return  (ERR_VAL);
    }
    
    sockaddrin6.sin6_len    = sizeof(struct sockaddr_in);
    sockaddrin6.sin6_family = AF_INET6;
    sockaddrin6.sin6_port   = 0;
    sockaddrin6.sin6_addr   = *pin6addr;
    
    sstError = sendto(iSock, icmp6hdrEcho, stPingSize, 0, 
                      (const struct sockaddr *)&sockaddrin6, 
                      sizeof(struct sockaddr_in6));
                         
    __SHEAP_FREE(icmp6hdrEcho);
    
    return ((sstError > 0) ? ERR_OK : ERR_VAL);
}
/*********************************************************************************************************
** ��������: __inetPingRecv
** ��������: ���� ping ��
** �䡡��  : iSock         socket
**           usSeqRecv     ��Ҫ�жϵ� seq
**           piHL          ���յ� hop limit
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetPing6Recv (INT  iSock, UINT16  usSeqRecv, INT  *piHL)
{
             CHAR                   cBuffer[512];
             
             INT                    iCnt = 20;                          /*  Ĭ�������յ����ݰ���      */
    REGISTER ssize_t                sstLen;
             INT                    iAddLen = sizeof(struct sockaddr_in6);
             struct sockaddr_in6    sockaddrin6From;
             
             struct ip6_hdr        *ip6hdrFrom;
             struct icmp6_echo_hdr *icmp6hdrFrom;
             
             u8_t                   nexth;
             INT                    hlen;
             INT                    totalhlen = 0;
             u8_t                  *pucData;

    *piHL = 0;

    while ((sstLen = recvfrom(iSock, cBuffer, sizeof(cBuffer), 0, 
                              (struct sockaddr *)&sockaddrin6From, (socklen_t *)&iAddLen)) > 0) {
        
        if (sstLen >= (sizeof(struct ip6_hdr) + sizeof(struct icmp6_echo_hdr))) {
            ip6hdrFrom = (struct ip6_hdr *)cBuffer;
            hlen       = IP6_HLEN;
            totalhlen  = IP6_HLEN;
            nexth      = IP6H_NEXTH(ip6hdrFrom);
            *piHL      = IP6H_HOPLIM(ip6hdrFrom);
            
            pucData    = (u8_t *)cBuffer;
            pucData   += IP6_HLEN;
            
            while (nexth != IP6_NEXTH_NONE) {
                switch (nexth) {
                
                case IP6_NEXTH_HOPBYHOP:
                case IP6_NEXTH_DESTOPTS:
                case IP6_NEXTH_ROUTING:
                    nexth      = *pucData;
                    hlen       = 8 * (1 + *(pucData + 1));
                    totalhlen += hlen;
                    pucData   += hlen;
                    break;
                    
                case IP6_NEXTH_FRAGMENT:
                    nexth      = *pucData;
                    hlen       = 8;
                    totalhlen += hlen;
                    pucData   += hlen;
                    break;
                    
                default:
                    goto    __hdrlen_cal_ok;
                    break;
                }
            }
            
__hdrlen_cal_ok:
            icmp6hdrFrom = (struct icmp6_echo_hdr *)(cBuffer + totalhlen);
            if ((icmp6hdrFrom->id == 0xAFAF) && (icmp6hdrFrom->seqno == htons(usSeqRecv))) {
                return  (ERROR_NONE);
            }
        }
        
        iCnt--;                                                         /*  ���յ���������ݰ�̫��      */
        if (iCnt < 0) {
            break;                                                      /*  �˳�                        */
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __inetPing6Cleanup
** ��������: ���� ping ��Դ.
** �䡡��  : iSock         �׽���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetPing6Cleanup (INT  iSock)
{
    close(iSock);
}
/*********************************************************************************************************
** ��������: API_INetPing6
** ��������: internet ipv6 ping
** �䡡��  : pinaddr       Ŀ�� ip ��ַ.
**           iTimes        ����
**           iDataSize     ���ݴ�С
**           iTimeout      ��ʱʱ��
**           pcNetif       ����ӿ��� (NULL ��ʾ�Զ�ȷ���ӿ�)
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetPing6 (struct in6_addr  *pin6addr, 
                    INT               iTimes, 
                    INT               iDataSize, 
                    INT               iTimeout, 
                    CPCHAR            pcNetif)
{
             CHAR       cInetAddr[INET6_ADDRSTRLEN];                    /*  IP ��ַ����                 */

    REGISTER INT        iSock;
             INT        i;
             UINT16     usSeqRecv = 0;
             ULONG      ulTime1;
             ULONG      ulTime2;
             
             INT        iSuc = 0;
             INT        iHLRecv;

    struct  sockaddr_in6    sockaddrin6To;
    
    REGISTER ULONG      ulTimeMax = 0;
    REGISTER ULONG      ulTimeMin = 0xFFFFFFFF;
    REGISTER ULONG      ulTimeAvr = 0;
    
    
    if ((iDataSize >= (64 * LW_CFG_KB_SIZE)) || (iDataSize < 0)) {      /*  0 - 64KB                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    sockaddrin6To.sin6_len    = sizeof(struct sockaddr_in6);
    sockaddrin6To.sin6_family = AF_INET6;
    sockaddrin6To.sin6_addr   = *pin6addr;
    
    iSock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);                 /*  ����ԭ�����е� push         */
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    API_ThreadCleanupPush(__inetPing6Cleanup, (PVOID)iSock);            /*  �����������                */
    
    setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, &iTimeout, sizeof(INT));
    
    connect(iSock, (struct sockaddr *)&sockaddrin6To, 
            sizeof(struct sockaddr_in6));                               /*  �趨Ŀ��                    */
    
    printf("Pinging %s\n\n", inet6_ntoa_r(*pin6addr, cInetAddr, sizeof(cInetAddr)));
    
    for (i = 0; ;) {
        if (__inetPing6Send(iSock, pin6addr, iDataSize, pcNetif, &usSeqRecv) < 0) {
                                                                        /*  ���� icmp ���ݰ�            */
            fprintf(stderr, "error : %s.\n", lib_strerror(errno));
        
            i++;
            if (i >= iTimes) {
                break;
            }
            API_TimeSSleep(1);                                          /*  �ȴ� 1 S                    */
            continue;
        
        } else {
            i++;                                                        /*  ���ʹ��� ++                 */
        }
        
        ulTime1 = API_TimeGet();
        if (__inetPing6Recv(iSock, usSeqRecv, &iHLRecv) < 0) {          /*  ���� icmp ���ݰ�            */
            printf("Request time out.\n");                              /*  timeout                     */
            if (i >= iTimes) {
                break;                                                  /*  ping ����                   */
            }
        
        } else {
            ulTime2 = API_TimeGet();
            ulTime2 = (ulTime2 >= ulTime1) ? (ulTime2 - ulTime1) 
                    : ((__ARCH_ULONG_MAX - ulTime1) + ulTime2 + 1);
            ulTime2 = ((ulTime2 * 1000) / LW_TICK_HZ);                  /*  תΪ������                  */
                    
            printf("Reply from %s: bytes=%d time=%ldms hoplim=%d\n", 
                   inet6_ntoa_r(*pin6addr, cInetAddr, sizeof(cInetAddr)),
                   iDataSize, ulTime2, iHLRecv);
        
            iSuc++;
            
            ulTimeAvr += ulTime2;
            ulTimeMax = (ulTimeMax > ulTime2) ? ulTimeMax : ulTime2;
            ulTimeMin = (ulTimeMin < ulTime2) ? ulTimeMin : ulTime2;
            
            if (i >= iTimes) {
                break;                                                  /*  ping ����                   */
            } else {
                API_TimeSSleep(1);                                      /*  �ȴ� 1 S                    */
            }
        }
    }
    API_ThreadCleanupPop(LW_TRUE);                                      /*  ping ���                   */
    
    /*
     *  ��ӡ�ܽ���Ϣ
     */
    printf("\nPing statistics for %s:\n", inet6_ntoa_r(*pin6addr, cInetAddr, sizeof(cInetAddr)));
    printf("    Packets: Send = %d, Received = %d, Lost = %d(%d%% loss),\n", 
                  iTimes, iSuc, (iTimes - iSuc), (((iTimes - iSuc) * 100) / iTimes));
    
    if (iSuc == 0) {                                                    /*  û��һ�γɹ�                */
        return  (PX_ERROR);
    }
    
    ulTimeAvr = ulTimeAvr / iSuc;
    
    printf("Approximate round trip times in milli-seconds:\n");
    printf("    Minimum = %ldms, Maximum = %ldms, Average = %ldms\r\n\r\n",
                  ulTimeMin, ulTimeMax, ulTimeAvr);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellPing6
** ��������: ping6 ����
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellPing6 (INT  iArgC, PCHAR  *ppcArgV)
{
    struct addrinfo      hints;
    struct addrinfo     *phints = LW_NULL;

    struct in6_addr in6addr;
           CHAR     cInetAddr[INET6_ADDRSTRLEN];                        /*  IP ��ַ����                 */
           PCHAR    pcNetif = LW_NULL;

           INT      iTimes    = 4;
           INT      iDataSize = 32;
           INT      iTimeout  = 3000;

    if (iArgC <= 1) {
        fprintf(stderr, "argments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    /*
     *  ��������
     */
    if (iArgC >= 4) {
        REGISTER INT    i;
        
        for (i = 2; i < iArgC; i += 2) {
            if (lib_strcmp(ppcArgV[i], "-n") == 0) {
                sscanf(ppcArgV[i + 1], "%i", &iTimes);                  /*  ��ô���                    */
            
            } else if (lib_strcmp(ppcArgV[i], "-l") == 0) {
                sscanf(ppcArgV[i + 1], "%i", &iDataSize);               /*  ������ݴ�С                */
                if ((iDataSize > (65000 - sizeof(struct icmp_echo_hdr))) ||
                    (iDataSize < 1)) {
                    fprintf(stderr, "data size error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
            
            } else if (lib_strcmp(ppcArgV[i], "-w") == 0) {             /*  ��� timeout ��ֵ           */
                sscanf(ppcArgV[i + 1], "%i", &iTimeout);
                if ((iTimeout < 1) || (iTimeout > 60000)) {
                    fprintf(stderr, "timeout error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                
            } else if (lib_strcmp(ppcArgV[i], "-I") == 0) {             /*  ����ӿ���                  */
                pcNetif = ppcArgV[i + 1];
                
            } else {
                fprintf(stderr, "argments error!\n");                   /*  ��������                    */
                return  (-ERROR_TSHELL_EPARAM);
            }
        }
    }
    
    /*
     *  ������ַ
     */
    if (!inet6_aton(ppcArgV[1], &in6addr)) {
        printf("Execute a DNS query...\n");
        
        {
            ULONG  iOptionNoAbort;
            ULONG  iOption;
            ioctl(STD_IN, FIOGETOPTIONS, &iOption);
            iOptionNoAbort = (iOption & ~OPT_ABORT);
            ioctl(STD_IN, FIOSETOPTIONS, iOptionNoAbort);               /*  ������ control-C ����       */
            
            hints.ai_family = AF_INET6;                                 /*  ���� IPv6 ��ַ              */
            getaddrinfo(ppcArgV[1], LW_NULL, &hints, &phints);          /*  ��������                    */
        
            ioctl(STD_IN, FIOSETOPTIONS, iOption);                      /*  �ظ�ԭ��״̬                */
        }
        
        if (phints == LW_NULL) {
            printf("Pinging request could not find host %s ."
                   "Please check the name and try again.\n\n", ppcArgV[1]);
            return  (-ERROR_TSHELL_EPARAM);
        
        } else {
            if (phints->ai_addr->sa_family == AF_INET6) {               /*  ��������ַ                */
                in6addr = ((struct sockaddr_in6 *)(phints->ai_addr))->sin6_addr;
                freeaddrinfo(phints);
            
            } else {
                freeaddrinfo(phints);
                printf("Ping6 only support AF_INET6 domain!\n");
                return  (-EAFNOSUPPORT);
            }
            printf("Pinging %s [%s]\n\n", ppcArgV[1], 
                   inet6_ntoa_r(in6addr, cInetAddr, sizeof(cInetAddr)));
        }
    }
    
    return  (API_INetPing6(&in6addr, iTimes, iDataSize, iTimeout, pcNetif));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: API_INetPing6Init
** ��������: ��ʼ�� IPv6 ping ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_INetPing6Init (VOID)
{
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("ping6", __tshellPing6);
    API_TShellFormatAdd("ping6", " ip(v6)/hostname [-l datalen] [-n times] [-w timeout] [-I interface]");
    API_TShellHelpAdd("ping6",   "ipv6 ping tool\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_PING_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
