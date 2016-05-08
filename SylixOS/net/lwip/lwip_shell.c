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
** ��   ��   ��: lwip_shell.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip shell ����.

** BUG:
2009.05.22  ��������Ĭ��·���� shell ����.
2009.05.27  ��������ӿ� linkup ����ʾ��Ϣ.
2009.06.02  ������ tftp �� put �� get ��Դ�ļ���Ŀ���˳��.
2009.06.03  ����������ʱ����� DHCP ��ȡ IP, ������Ҫ�������ַ����.
2009.06.08  ifup ������ -nodhcp ѡ��, ����ǿ�Ʋ�ʹ�� DHCP ��ȡ IP ��ַ.
2009.06.26  ���� shell ��������.
2009.07.29  ���� ifconfig ����, �����ӽ� linux bash.
2009.09.14  ���� ifrouter ��Ĭ��·�ɽӿڵ���ʾ.
2009.11.09  ��Щ�������� api ��Ҫʹ�� netifapi_... ���.
2009.11.21  tftp ������е���, ���� tftp������.
2009.12.11  ifconfig �м��� metric ����ʾ.
2010.11.04  ���� arp ����֧��.
2011.06.08  ifconfig ��ʾ inet6 ��ַ�����Ϣ.
2011.07.02  ���� route ����.
2012.08.21  ���ڳ�ʼ�� ppp ��� shell.
2013.05.14  ��ӡ ipv6 ��ַʱ��Ҫ��ӡ��ַ״̬.
2013.09.12  ���� ifconfig ��������ӿ�ʱ�İ�ȫ��.
            ���� arp ����İ�ȫ��.
2014.12.23  ���� ifconfig ��ʾ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/inet6.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip_route.h"
#include "lwip_if.h"
/*********************************************************************************************************
  ARP ��Э����غ���
*********************************************************************************************************/
#include "netif/etharp.h"
/*********************************************************************************************************
  netstat ������Ϣ
*********************************************************************************************************/
static const CHAR   _G_cNetstatHelp[] = {
    "show net status\n\n"
    "-h, --help             display this message\n\n"
    "-r, --route            display route table\n"
    "-i, --interface        display interface table\n"
    "-g, --groups           display multicast group memberships\n"
    "-s, --statistics       display networking statistics (like SNMP)\n\n"
    "-w, --raw              display raw socket information\n"
    "-t, --tcp              display tcp socket information\n"
    "-u, --udp              display udp socket information\n"
    "-p, --packet           display packet socket information\n"
    "-x, --unix             display unix socket information\n\n"
    "-l, --listening        display listening server sockets\n"
    "-a, --all              display all sockets\n\n"
    "-A <net type>, --<net type>    select <net type>, <net type>=inet, inet6 or unix\n"
};
extern VOID  __tshellNetstatIf(VOID);
#if LW_CFG_LWIP_IGMP > 0
extern VOID  __tshellNetstatGroup(INT  iNetType);
#endif
extern VOID  __tshellNetstatStat(VOID);
extern VOID  __tshellNetstatRaw(INT  iNetType);
extern VOID  __tshellNetstatTcp(INT  iNetType);
extern VOID  __tshellNetstatTcpListen(INT  iNetType);
extern VOID  __tshellNetstatUdp(INT  iNetType);
extern VOID  __tshellNetstatUnix(INT  iNetType);
extern VOID  __tshellNetstatPacket(INT  iNetType);
/*********************************************************************************************************
** ��������: __tshellNetstat
** ��������: ϵͳ���� "netstat"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNetstat (INT  iArgC, PCHAR  *ppcArgV)
{
    int             iC;
    const  char     cShortOpt[] = "hrigswtpuxlaA:";
    struct option   optionNetstat[] = {
        {"help",       0, LW_NULL, 'h'},
        {"route",      0, LW_NULL, 'r'},
        {"interface",  0, LW_NULL, 'i'},
        {"groups",     0, LW_NULL, 'g'},
        {"statistics", 0, LW_NULL, 's'},
        {"raw",        0, LW_NULL, 'r'},
        {"tcp",        0, LW_NULL, 't'},
        {"udp",        0, LW_NULL, 'u'},
        {"packet",     0, LW_NULL, 'p'},
        {"unix",       0, LW_NULL, 'x'},
        {"listening",  0, LW_NULL, 'l'},
        {"all",        0, LW_NULL, 'a'},
        {"unix",       0, LW_NULL, 1},
        {"inet",       0, LW_NULL, 2},
        {"inet6",      0, LW_NULL, 3},
        {LW_NULL,      0, LW_NULL, 0},
    };
    
    BOOL    bPacket  = LW_FALSE;
    BOOL    bRaw     = LW_FALSE;
    BOOL    bUdp     = LW_FALSE;
    BOOL    bTcp     = LW_FALSE;
    BOOL    bUnix    = LW_FALSE;
    BOOL    bListen  = LW_FALSE;
    INT     iNetType = 0;                                               /* 0:all 1:unix 2:inet 3:inet6  */
    CHAR    cNettype[10];
    
    while ((iC = getopt_long(iArgC, ppcArgV, cShortOpt, optionNetstat, LW_NULL)) != -1) {
        switch (iC) {
        
        case 'h':                                                       /*  ��ʾ����                    */
            printf(_G_cNetstatHelp);
            return  (ERROR_NONE);
            
        case 'r':                                                       /*  ��ʾ·�ɱ�                  */
            ppcArgV[1] = LW_NULL;
            __tshellRoute(1, ppcArgV);
            return  (ERROR_NONE);
            
        case 'i':                                                       /*  ��ʾ����ӿ���Ϣ            */
            __tshellNetstatIf();
            return  (ERROR_NONE);
            
        case 'g':                                                       /*  ��ʾ�鲥�����              */
#if LW_CFG_LWIP_IGMP > 0
            __tshellNetstatGroup(iNetType);
#endif
            return  (ERROR_NONE);
        
        case 's':                                                       /*  ��ʾͳ����Ϣ                */
            __tshellNetstatStat();
            return  (ERROR_NONE);
            
        case 'p':                                                       /*  ��ʾ packet socket          */
            bPacket = LW_TRUE;
            break;
            
        case 'w':                                                       /*  ��ʾ raw socket             */
            bRaw = LW_TRUE;
            break;
            
        case 't':                                                       /*  ��ʾ tcp socket             */
            bTcp = LW_TRUE;
            break;
            
        case 'u':                                                       /*  ��ʾ udp socket             */
            bUdp = LW_TRUE;
            break;
        
        case 'x':                                                       /*  ��ʾ unix socket            */
            bUnix = LW_TRUE;
            break;
        
        case 'l':                                                       /*  ��ʾ listen socket          */
            bListen = LW_TRUE;
            break;
            
        case 'a':                                                       /*  ��ʾ�б�                    */
            goto    __show;
            
        case 'A':                                                       /*  ��������                    */
            lib_strlcpy(cNettype, optarg, 10);
            if (lib_strcmp(cNettype, "unix") == 0) {
                iNetType = 1;
            } else if (lib_strcmp(cNettype, "inet") == 0) {
                iNetType = 2;
            } else if (lib_strcmp(cNettype, "inet6") == 0) {
                iNetType = 3;
            }
            break;
            
        case 1:
            iNetType = 1;
            break;
            
        case 2:
            iNetType = 2;
            break;
            
        case 3:
            iNetType = 3;
            break;
        }
    }
    getopt_free();
    
__show:
    if ((bRaw    == LW_FALSE) && (bUdp    == LW_FALSE) &&
        (bTcp    == LW_FALSE) && (bUnix   == LW_FALSE) &&
        (bListen == LW_FALSE) && (bPacket == LW_FALSE)) {
        bRaw    = LW_TRUE;
        bUdp    = LW_TRUE;
        bTcp    = LW_TRUE;
        bUnix   = LW_TRUE;
        bPacket = LW_TRUE;
    }
    if (bUnix) {
        __tshellNetstatUnix(iNetType);
    }
    if (bPacket) {
        __tshellNetstatPacket(iNetType);
    }
    if (bTcp || bListen) {
        __tshellNetstatTcpListen(iNetType);
        if (bTcp) {
            __tshellNetstatTcp(iNetType);
        }
    }
    if (bUdp) {
        __tshellNetstatUdp(iNetType);
    }
    if (bRaw) {
        __tshellNetstatRaw(iNetType);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __netIfSpeed
** ��������: ��ʾָ��������ӿ���Ϣ (ip v4)
** �䡡��  : pcIfName      ����ӿ���
**           netifShow     ����ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __netIfSpeed (struct netif  *netif, PCHAR  pcSpeedStr, size_t  stSize)
{
    if (netif->link_speed == 0) {
        lib_strlcpy(pcSpeedStr, "AUTO", stSize);
        
    } else if (netif->link_speed < 1000) {
        snprintf(pcSpeedStr, stSize, "%d(bps)", netif->link_speed);
    
    } else if (netif->link_speed < 5000000) {
        snprintf(pcSpeedStr, stSize, "%d(Kbps)", netif->link_speed / 1000);
    
    } else {
        snprintf(pcSpeedStr, stSize, "%d(Mbps)", netif->link_speed / 1000000);
    }
}
/*********************************************************************************************************
** ��������: __netIfShow
** ��������: ��ʾָ��������ӿ���Ϣ (ip v4)
** �䡡��  : pcIfName      ����ӿ���
**           netifShow     ����ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __netIfShow (CPCHAR  pcIfName, const struct netif  *netifShow)
{
    struct netif    *netif;
    CHAR             cSpeed[32];
    ip_addr_t        ipaddrBroadcast;
    INT              i;

    if ((pcIfName == LW_NULL) && (netifShow == LW_NULL)) {
        return;
    }

    if (netifShow) {
        netif = (struct netif *)netifShow;
    } else {
        netif = netif_find((PCHAR)pcIfName);
    }

    if (netif == LW_NULL) {
        return;
    }

    /*
     *  ��ӡ���ڻ�����Ϣ
     */
    printf("%c%c%d       ",   netif->name[0], netif->name[1], netif->num);
    printf("enable: %s ",     (netif_is_up(netif) > 0) ? "true" : "false");
    printf("linkup: %s ",     (netif_is_link_up(netif) > 0) ? "true" : "false");
    printf("MTU: %d ",        netif->mtu);
    printf("multicast: %s\n", (netif->flags & NETIF_FLAG_IGMP) ? "true" : "false");

    /*
     *  ��ӡ·����Ϣ
     */
    if (netif == netif_default) {                                       /*  route interface             */
        printf("          metric: 1 ");
    } else {
        printf("          metric: 0 ");
    }
    /*
     *  ��ӡ����Ӳ����ַ��Ϣ
     */
    if (netif->flags & NETIF_FLAG_ETHARP) {
        printf("type: Ethernet-Cap HWaddr: ");                          /*  ��̫����                    */
        for (i = 0; i < netif->hwaddr_len - 1; i++) {
            printf("%02X:", netif->hwaddr[i]);
        }
        printf("%02X\n", netif->hwaddr[netif->hwaddr_len - 1]);
    } else if (netif->flags & NETIF_FLAG_POINTTOPOINT) {
        printf("type: WAN(PPP/SLIP)\n");                                /*  ��Ե�����ӿ�              */
    } else {
        printf("type: General\n");                                      /*  ͨ������ӿ�                */
    }
    
    __netIfSpeed(netif, cSpeed, sizeof(cSpeed));
    
#if LWIP_DHCP
    printf("          DHCP: %s(%s) speed: %s\n", 
                                (netif->flags2 & NETIF_FLAG2_DHCP) ? "Enable" : "Disable",
                                (netif->dhcp) ? "On" : "Off", cSpeed);
#else
    printf("          speed: %s\n", cSpeed);                            /*  ��ӡ�����ٶ�                */
#endif                                                                  /*  LWIP_DHCP                   */
                                                                        
    /*
     *  ��ӡ����Э���ַ��Ϣ
     */
    printf("          inet addr: %d.%d.%d.%d ", ip4_addr1(&netif->ip_addr),
                                                ip4_addr2(&netif->ip_addr),
                                                ip4_addr3(&netif->ip_addr),
                                                ip4_addr4(&netif->ip_addr));
    printf("netmask: %d.%d.%d.%d\n", ip4_addr1(&netif->netmask),
                                     ip4_addr2(&netif->netmask),
                                     ip4_addr3(&netif->netmask),
                                     ip4_addr4(&netif->netmask));

    if (netif->flags & NETIF_FLAG_POINTTOPOINT) {
        printf("          P-to-P: %d.%d.%d.%d ", ip4_addr1(&netif->gw),
                                                 ip4_addr2(&netif->gw),
                                                 ip4_addr3(&netif->gw),
                                                 ip4_addr4(&netif->gw));
    } else {
        printf("          gateway: %d.%d.%d.%d ", ip4_addr1(&netif->gw),
                                                  ip4_addr2(&netif->gw),
                                                  ip4_addr3(&netif->gw),
                                                  ip4_addr4(&netif->gw));
    }
    
    if (netif->flags & NETIF_FLAG_BROADCAST) {                          /*  ��ӡ�㲥��ַ��Ϣ            */
        ipaddrBroadcast.addr = (netif->ip_addr.addr | (~netif->netmask.addr));
        printf("broadcast: %d.%d.%d.%d\n", ip4_addr1(&ipaddrBroadcast),
                                           ip4_addr2(&ipaddrBroadcast),
                                           ip4_addr3(&ipaddrBroadcast),
                                           ip4_addr4(&ipaddrBroadcast));
    } else {
        printf("broadcast: Non\n");
    }
    
    /*
     *  ��ӡ ipv6 ��Ϣ
     */
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        PCHAR       pcAddrStat;
        PCHAR       pcAddrType;
        CHAR        cBuffer[64];
        
        if (ip6_addr_istentative(netif->ip6_addr_state[i])) {
            pcAddrStat = "tentative";
        } else if (ip6_addr_isvalid(netif->ip6_addr_state[i])) {
            pcAddrStat = "valid";
        } else if (ip6_addr_ispreferred(netif->ip6_addr_state[i])) {
            pcAddrStat = "preferred";
        } else {
            continue;
        }
        
        if (ip6_addr_isglobal(&netif->ip6_addr[i])) {
            pcAddrType = "global";
        } else if (ip6_addr_islinklocal(&netif->ip6_addr[i])) {
            pcAddrType = "link";
        } else if (ip6_addr_issitelocal(&netif->ip6_addr[i])) {
            pcAddrType = "site";
        } else if (ip6_addr_isuniquelocal(&netif->ip6_addr[i])) {
            pcAddrType = "uniquelocal";
        } else if (ip6_addr_isloopback(&netif->ip6_addr[i])) {
            pcAddrType = "loopback";
        } else {
            pcAddrType = "unknown";
        }
        
        printf("          inet6 addr: %s Scope:%s <%s>\n", 
               ip6addr_ntoa_r(&netif->ip6_addr[i], cBuffer, sizeof(cBuffer)),
               pcAddrType, pcAddrStat);
    }
    
    /*
     *  ��ӡ�����շ�������Ϣ
     */
    printf("          RX ucast packets:%u nucast packets:%u dropped:%u\n", netif->ifinucastpkts,
                                                                           netif->ifinnucastpkts,
                                                                           netif->ifindiscards);
    printf("          TX ucast packets:%u nucast packets:%u dropped:%u\n", netif->ifoutucastpkts,
                                                                           netif->ifoutnucastpkts,
                                                                           netif->ifoutdiscards);
    printf("          RX bytes:%u TX bytes:%u\n", netif->ifinoctets,
                                                  netif->ifoutoctets);
    printf("\n");
}
/*********************************************************************************************************
** ��������: __netIfShowAll
** ��������: ��ʾ��������ӿ���Ϣ (ip v4)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __netIfShowAll (VOID)
{
    struct netif *netif = netif_list;
    INT           iCounter = 0;
    INT           i;
    CHAR          cName[5] = "null";                                    /*  ��ǰĬ��·������ӿ���      */

    for (; netif != LW_NULL; netif = netif->next) {
        __netIfShow(LW_NULL, netif);
        iCounter++;
    }

#if LWIP_DNS > 0
    for (i = 0; i < DNS_MAX_SERVERS; i++) {
        ip_addr_t ipaddr = dns_getserver((u8_t)i);
        printf("dns%d: %d.%d.%d.%d\n", (i),
                                       ip4_addr1(&ipaddr),
                                       ip4_addr2(&ipaddr),
                                       ip4_addr3(&ipaddr),
                                       ip4_addr4(&ipaddr));
    }
#endif                                                                  /*  LWIP_DNS                    */

    if (netif_default) {
        cName[0] = netif_default->name[0];
        cName[1] = netif_default->name[1];
        cName[2] = (CHAR)(netif_default->num + '0');
        cName[3] = PX_EOS;
    }
    
    printf("default device is: %s\n", cName);                           /*  ��ʾ·�ɶ˿�                */
    printf("total net interface: %d\n", iCounter);
}
/*********************************************************************************************************
** ��������: __netIfSet
** ��������: ����ָ������ӿ���Ϣ (ip v4)
** �䡡��  : netif     ����ӿ�
**           pcItem    ������Ŀ����
**           ipaddr    ��ַ��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __netIfSet (struct netif  *netif, CPCHAR  pcItem, ip_addr_t  ipaddr)
{
    ip_addr_t  ipaddrInet;
    ip_addr_t  ipaddrMask;
    ip_addr_t  ipaddrGw;

    if (netif == LW_NULL) {
        return;
    }
    
    ipaddrInet = netif->ip_addr;
    ipaddrMask = netif->netmask;
    ipaddrGw   = netif->gw;

    if (lib_strcmp(pcItem, "inet") == 0) {
        netifapi_netif_set_addr(netif, &ipaddr, &ipaddrMask, &ipaddrGw);
    } else if (lib_strcmp(pcItem, "netmask") == 0) {
        netifapi_netif_set_addr(netif, &ipaddrInet, &ipaddr, &ipaddrGw);
    } else if (lib_strcmp(pcItem, "gateway") == 0) {
        netifapi_netif_set_addr(netif, &ipaddrInet, &ipaddrMask, &ipaddr);
    } else {
        fprintf(stderr, "argments error!\n");
    }
}
/*********************************************************************************************************
** ��������: __tshellIfconfig
** ��������: ϵͳ���� "ifconfig"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfconfig (INT  iArgC, PCHAR  *ppcArgV)
{
    struct netif    *netif;
    struct in_addr   inaddr;
    ip_addr_t        ipaddr;

    if (iArgC == 1) {
        LWIP_NETIF_LOCK();
        __netIfShowAll();                                               /*  ��ӡ����������Ϣ            */
        LWIP_NETIF_UNLOCK();
        return  (ERROR_NONE);
    
    } else if (iArgC == 2) {
        LWIP_NETIF_LOCK();
        __netIfShow(ppcArgV[1], LW_NULL);                               /*  ��ӡָ��������Ϣ            */
        LWIP_NETIF_UNLOCK();
        return  (ERROR_NONE);
    }

    /*
     *  ��������
     */
    if (iArgC >= 4) {
        if (lib_strcmp(ppcArgV[1], "dns") == 0) {
            /*
             *  ָ�� DNS ����
             */
            INT     iDnsIndex = 0;
            sscanf(ppcArgV[2], "%d", &iDnsIndex);
            if (iDnsIndex >= DNS_MAX_SERVERS) {
                fprintf(stderr, "argments error!\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            if (inet_aton(ppcArgV[3], &inaddr) == 0) {                  /*  ��� IP ��ַ                */
                fprintf(stderr, "address error.\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            ipaddr.addr = inaddr.s_addr;
            dns_setserver((u8_t)iDnsIndex, &ipaddr);                    /*  ���� DNS                    */
        } else {
            /*
             *  ָ������ӿ�����
             */
            INT     iIndex;
            netif = netif_find(ppcArgV[1]);                             /*  ��ѯ����ӿ�                */
            if (netif == LW_NULL) {
                fprintf(stderr, "can not find net interface.\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            for (iIndex = 2; iIndex < (iArgC - 1); iIndex += 2) {       /*  �������ò���                */
                if (inet_aton(ppcArgV[iIndex + 1], &inaddr) == 0) {     /*  ��� IP ��ַ                */
                    fprintf(stderr, "address error.\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                ipaddr.addr = inaddr.s_addr;
                __netIfSet(netif, ppcArgV[iIndex], ipaddr);             /*  ��������ӿ�                */
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIfUp
** ��������: ϵͳ���� "ifup"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfUp (INT  iArgC, PCHAR  *ppcArgV)
{
    struct netif *netif;
    BOOL          bUseDHCP      = LW_FALSE;                             /*  �Ƿ�ʹ���Զ���ȡ IP         */
    BOOL          bShutDownDHCP = LW_FALSE;                             /*  �Ƿ�ǿ�ƹر� DHCP           */

    if (iArgC < 2) {
        fprintf(stderr, "argments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    } else if (iArgC > 2) {
        if (lib_strcmp(ppcArgV[2], "-dhcp") == 0) {
            bUseDHCP = LW_TRUE;                                         /*  ʹ�� DHCP ����              */
        } else if (lib_strcmp(ppcArgV[2], "-nodhcp") == 0) {
            bShutDownDHCP = LW_TRUE;
        }
    }

    netif = netif_find(ppcArgV[1]);                                     /*  ��ѯ����ӿ�                */
    if (netif == LW_NULL) {
        fprintf(stderr, "can not find net interface.\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (netif_is_up(netif)) {                                           /*  �����Ƿ��Ѿ�����            */
#if LWIP_DHCP > 0                                                       /*  ���ȹر�����                */
        if (netif->dhcp && netif->dhcp->pcb) {
            netifapi_netif_common(netif, NULL, dhcp_release);           /*  ��� DHCP ��Լ              */
            netifapi_dhcp_stop(netif);                                  /*  �ͷ���Դ                    */
        }
#endif                                                                  /*  LWIP_DHCP > 0               */
        netifapi_netif_set_down(netif);                                 /*  ��������                    */
    }

    netifapi_netif_set_up(netif);                                       /*  ��������                    */

#if LWIP_DHCP > 0
    if (bUseDHCP) {
        netif->flags2 |= NETIF_FLAG2_DHCP;                              /*  ʹ�� DHCP ����              */
    } else if (bShutDownDHCP) {
        netif->flags2 &= ~NETIF_FLAG2_DHCP;                             /*  ǿ�ƹر� DHCP               */
    }

    if (netif->flags2 & NETIF_FLAG2_DHCP) {
        ip_addr_t  inaddrNone;

        lib_bzero(&inaddrNone, sizeof(ip_addr_t));
        netifapi_netif_set_addr(netif, &inaddrNone, &inaddrNone, &inaddrNone);
                                                                        /*  ���е�ַ����Ϊ 0            */
        printf("DHCP client starting...\n");
        if (netifapi_dhcp_start(netif) < ERR_OK) {
            printf("DHCP client serious error.\n");
        } else {
            printf("DHCP client start.\n");
        }
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

    printf("net interface \"%s\" set up.\n", ppcArgV[1]);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIfDown
** ��������: ϵͳ���� "ifdown"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfDown (INT  iArgC, PCHAR  *ppcArgV)
{
    struct netif *netif;

    if (iArgC != 2) {
        fprintf(stderr, "argments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    netif = netif_find(ppcArgV[1]);                                     /*  ��ѯ����ӿ�                */
    if (netif == LW_NULL) {
        fprintf(stderr, "can not find net interface.\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (!netif_is_up(netif)) {
        fprintf(stderr, "net interface already set down.\n");
        return  (PX_ERROR);
    }

#if LWIP_DHCP > 0
    if (netif->dhcp && netif->dhcp->pcb) {
        netifapi_netif_common(netif, NULL, dhcp_release);               /*  ��� DHCP ��Լ              */
        netifapi_dhcp_stop(netif);                                      /*  �ͷ���Դ                    */
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

    netifapi_netif_set_down(netif);                                     /*  ��������                    */

    printf("net interface \"%s\" set down.\n", ppcArgV[1]);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIfRouter
** ��������: ϵͳ���� "ifrouter"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfRouter (INT  iArgC, PCHAR  *ppcArgV)
{
    printf("this command has been removed! please use 'route' command instead.\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellArp
** ��������: ϵͳ���� "arp"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellArp (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        fprintf(stderr, "argments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcmp(ppcArgV[1], "-a") == 0) {                            /*  ��ʾ arp ��                 */
        INT     iFd;
        CHAR    cBuffer[512];
        ssize_t sstNum;
        
        iFd = open("/proc/net/arp", O_RDONLY);
        if (iFd < 0) {
            fprintf(stderr, "can not open /proc/net/arp : %s\n", lib_strerror(errno));
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
    
    } else if (lib_strcmp(ppcArgV[1], "-s") == 0) {                     /*  ����һ����̬ת����ϵ        */
        INT             i;
        INT             iTemp[6];
        ip_addr_t       ipaddr;
        struct eth_addr ethaddr;
        err_t           err;
        
        if (iArgC != 4) {
            fprintf(stderr, "argments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        ipaddr.addr = ipaddr_addr(ppcArgV[2]);
        if (ipaddr.addr == IPADDR_NONE) {
            fprintf(stderr, "bad inet address : %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        if (sscanf(ppcArgV[3], "%02x:%02x:%02x:%02x:%02x:%02x",
                   &iTemp[0], &iTemp[1], &iTemp[2], 
                   &iTemp[3], &iTemp[4], &iTemp[5]) != 6) {
            fprintf(stderr, "bad physical address : %s\n", ppcArgV[3]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        for (i = 0; i < 6; i++) {
            ethaddr.addr[i] = (u8_t)iTemp[i];
        }
        
        LOCK_TCPIP_CORE();
        err = etharp_add_static_entry(&ipaddr, &ethaddr);
        UNLOCK_TCPIP_CORE();
        
        return  (err ? PX_ERROR : ERROR_NONE);
    
    } else if (lib_strcmp(ppcArgV[1], "-d") == 0) {                     /*  ɾ��һ����̬ת����ϵ        */
        ip_addr_t       ipaddr;
        err_t           err;
        
        if (iArgC != 3) {                                               /*  ɾ��ȫ��ת����ϵ            */
            struct netif *netif;
            
            LWIP_NETIF_LOCK();
            for (netif = netif_list; netif != LW_NULL; netif = netif->next) {
                if (netif->flags & NETIF_FLAG_ETHARP) {
                    netifapi_netif_common(netif, etharp_cleanup_netif, LW_NULL);
                }
            }
            LWIP_NETIF_UNLOCK();
            
            return  (ERROR_NONE);
        }
        
        ipaddr.addr = ipaddr_addr(ppcArgV[2]);
        if (ipaddr.addr == IPADDR_NONE) {
            fprintf(stderr, "bad inet address : %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        LOCK_TCPIP_CORE();
        err = etharp_remove_static_entry(&ipaddr);
        UNLOCK_TCPIP_CORE();
        
        return  (err ? PX_ERROR : ERROR_NONE);
    
    } else {
        fprintf(stderr, "argments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
}
/*********************************************************************************************************
** ��������: __tshellNetInit
** ��������: ע����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetInit (VOID)
{
    __tshellRouteInit();                                                /*  ע�� route ����             */

    API_TShellKeywordAdd("netstat", __tshellNetstat);
    API_TShellFormatAdd("netstat",  " {[-wtux --A] -i | [hrigs]}");
    API_TShellHelpAdd("netstat",    _G_cNetstatHelp);

    API_TShellKeywordAdd("ifconfig", __tshellIfconfig);
    API_TShellFormatAdd("ifconfig",  " [netifname] [{inet | netmask | gateway}] [address]");
    API_TShellHelpAdd("ifconfig",    "show or set net interface parameter.\n"
                                     "if there are no argments, it will show all interface parameter\n"
                                     "set interface like following:\n"
                                     "ifconfig en1 inet    192.168.0.3\n"
                                     "ifconfig en1 netmask 255.255.255.0\n"
                                     "ifconfig en1 gateway 192.168.0.1\n"
                                     "ifconfig dns 0       192.168.0.2\n");

    API_TShellKeywordAdd("ifup", __tshellIfUp);
    API_TShellFormatAdd("ifup", " [netifname] [{-dhcp | -nodhcp}]");
    API_TShellHelpAdd("ifup",   "set net interface enable\n"
                                "\"-dncp\"   mean use dhcp client get net address.\n"
                                "\"-nodncp\" mean MUST NOT use dhcp.\n");

    API_TShellKeywordAdd("ifdown", __tshellIfDown);
    API_TShellFormatAdd("ifdown", " [netifname]");
    API_TShellHelpAdd("ifdown",   "set net interface disable.\n");

    API_TShellKeywordAdd("ifrouter", __tshellIfRouter);
    API_TShellFormatAdd("ifrouter", " [netifname]");
    API_TShellHelpAdd("ifrouter",   "set default router net interface.\n");
    
    API_TShellKeywordAdd("arp", __tshellArp);
    API_TShellFormatAdd("arp", " [-a | -s inet_address physical_address | -d inet_address]");
    API_TShellHelpAdd("arp",   "display ro modifies ARP table.\n"
                               "-a      display the ARP table.\n"
                               "-s      add or set a static arp entry.\n"
                               "        eg. arp -s 192.168.1.100 00:11:22:33:44:55\n"
                               "-d      delete a STATIC arp entry.\n"
                               "        eg. arp -d 192.168.1.100\n");
}
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
