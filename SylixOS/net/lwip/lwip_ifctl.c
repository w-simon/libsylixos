/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: lwip_ifctl.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 08 日
**
** 描        述: ioctl 网络接口支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "net/if.h"
#include "net/if_arp.h"
#include "net/if_type.h"
#include "net/if_flags.h"
#include "sys/socket.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip_if.h"
#if LW_CFG_NET_WIRELESS_EN > 0
#include "net/if_wireless.h"
#include "./wireless/lwip_wl.h"
#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
** 函数名称: __ifConfSize
** 功能描述: 获得网络接口列表保存所需要的内存大小
** 输　入  : piSize    所需内存大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __ifConfSize (INT  *piSize)
{
           INT       iNum = 0;
    struct netif    *pnetif;
    
    for (pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {
        iNum += sizeof(struct ifreq);
    }
    
    *piSize = iNum;
}
/*********************************************************************************************************
** 函数名称: __ifConf
** 功能描述: 获得网络接口列表操作
** 输　入  : pifconf   列表保存缓冲
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __ifConf (struct ifconf  *pifconf)
{
           INT           iSize;
           INT           iNum = 0;
    struct netif        *pnetif;
    struct ifreq        *pifreq;
    struct sockaddr_in  *psockaddrin;
    
    iSize = pifconf->ifc_len / sizeof(struct ifreq);                    /*  缓冲区个数                  */
    
    pifreq = pifconf->ifc_req;
    for (pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {
        if (iNum >= iSize) {
            break;
        }
        pifreq->ifr_name[0] = pnetif->name[0];
        pifreq->ifr_name[1] = pnetif->name[1];
        lib_itoa(pnetif->num, &pifreq->ifr_name[2], 10);
        
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        psockaddrin->sin_len    = sizeof(struct sockaddr_in);
        psockaddrin->sin_family = AF_INET;
        psockaddrin->sin_port   = 0;
        psockaddrin->sin_addr.s_addr = netif_ip4_addr(pnetif)->addr;
        
        iNum++;
        pifreq++;
    }
    
    pifconf->ifc_len = iNum * sizeof(struct ifreq);                     /*  读取个数                    */
}
/*********************************************************************************************************
** 函数名称: __ifFindByName
** 功能描述: 通过接口名获取接口结构
** 输　入  : pcName    网络接口名
** 输　出  : 网络接口结构
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static struct netif *__ifFindByName (CPCHAR  pcName)
{
    struct netif  *pnetif;
    INT            iIndex = lib_atoi(&pcName[2]);
    
    for (pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {  
        if ((pcName[0] == pnetif->name[0]) &&
            (pcName[1] == pnetif->name[1]) &&
            (iIndex    == pnetif->num)) {                               /*  匹配网络接口                */
            break;
        }
    }
    
    if (pnetif == LW_NULL) {
        return  (LW_NULL);
    
    } else {
        return  (pnetif);
    }
}
/*********************************************************************************************************
** 函数名称: __ifFindByName
** 功能描述: 通过 index 获取接口结构
** 输　入  : iIndex   网络结构 index
** 输　出  : 网络接口结构
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static struct netif *__ifFindByIndex (INT  iIndex)
{
    struct netif    *pnetif;
    
    for (pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {  
        if ((int)pnetif->num == iIndex) {
            break;
        }
    }
    
    if (pnetif == LW_NULL) {
        return  (LW_NULL);
    
    } else {
        return  (pnetif);
    }
}
/*********************************************************************************************************
** 函数名称: __ifReq6Size
** 功能描述: 获得网络接口 IPv6 地址数目
** 输　入  : ifreq6    ifreq6 请求控制块
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT __ifReq6Size (struct in6_ifreq  *pifreq6)
{
    INT              i;
    INT              iNum   = 0;
    struct netif    *pnetif = __ifFindByIndex(pifreq6->ifr6_ifindex);
    
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (PX_ERROR);
    }
    
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(pnetif->ip6_addr_state[i])) {
            iNum++;
        }
    }
    
    pifreq6->ifr6_len = iNum * sizeof(struct in6_ifr_addr);             /*  回写实际大小                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ifSubIoctlIf
** 功能描述: 网络接口 ioctl 操作 (针对网卡接口)
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifSubIoctlIf (INT  iCmd, PVOID  pvArg)
{
    INT              iRet   = PX_ERROR;
    INT              iFlags;
    struct ifreq    *pifreq = LW_NULL;
    struct netif    *pnetif;
    
    pifreq = (struct ifreq *)pvArg;
    
    pnetif = __ifFindByName(pifreq->ifr_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }

    switch (iCmd) {
    
    case SIOCGIFFLAGS:                                                  /*  获取网卡 flag               */
        pifreq->ifr_flags = netif_get_flags(pnetif);
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFFLAGS:                                                  /*  设置网卡 flag               */
        iFlags = netif_get_flags(pnetif);
        if (iFlags != pifreq->ifr_flags) {
            if (!pnetif->ioctl) {
                break;
            }
            iRet = pnetif->ioctl(pnetif, SIOCSIFFLAGS, pvArg);
            if (iRet < ERROR_NONE) {
                break;
            }
            if (pifreq->ifr_flags & IFF_PROMISC) {
                pnetif->flags2 |= NETIF_FLAG2_PROMISC;
            } else {
                pnetif->flags2 |= ~NETIF_FLAG2_PROMISC;
            }
            if (pifreq->ifr_flags & IFF_ALLMULTI) {
                pnetif->flags2 |= NETIF_FLAG2_ALLMULTI;
            } else {
                pnetif->flags2 |= ~NETIF_FLAG2_ALLMULTI;
            }
            if (pifreq->ifr_flags & IFF_UP) {
                netif_set_up(pnetif);
            } else {
                netif_set_down(pnetif);
            }
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFTYPE:                                                   /*  获得网卡类型                */
        if ((pnetif->flags & NETIF_FLAG_BROADCAST) == 0) {
            pifreq->ifr_type = IFT_PPP;
        } else if (pnetif->flags & (NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP)) {
            pifreq->ifr_type = IFT_ETHER;
        } else if (pnetif->num == 0) {
            pifreq->ifr_type = IFT_LOOP;
        } else {
            pifreq->ifr_type = IFT_OTHER;
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFINDEX:                                                  /*  获得网卡 index              */
        pifreq->ifr_ifindex = (int)pnetif->num;
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFMTU:                                                    /*  获得网卡 mtu                */
        pifreq->ifr_mtu = pnetif->mtu;
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFMTU:                                                    /*  设置网卡 mtu                */
        if (pifreq->ifr_mtu != pnetif->mtu) {
            if (pnetif->ioctl) {
                if (pnetif->ioctl(pnetif, SIOCSIFMTU, pvArg) == 0) {
                    pnetif->mtu = pifreq->ifr_mtu;
                    iRet = ERROR_NONE;
                }
            }
        }
        _ErrorHandle(ENOSYS);
        break;
        
    case SIOCGIFHWADDR:                                                 /*  获得物理地址                */
        if (pnetif->flags & (NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP)) {
            INT i;
            for (i = 0; i < IFHWADDRLEN; i++) {
                pifreq->ifr_hwaddr.sa_data[i] = pnetif->hwaddr[i];
            }
            pifreq->ifr_hwaddr.sa_family = ARPHRD_ETHER;
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFHWADDR:                                                 /*  设置 mac 地址               */
        if (pnetif->flags & (NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP)) {
            INT i;
            INT iIsUp = netif_is_up(pnetif);
            if (pnetif->ioctl) {
                netif_set_down(pnetif);                                 /*  关闭网口                    */
                iRet = pnetif->ioctl(pnetif, SIOCSIFHWADDR, pvArg);
                if (iRet == ERROR_NONE) {
                    for (i = 0; i < IFHWADDRLEN; i++) {
                        pnetif->hwaddr[i] = pifreq->ifr_hwaddr.sa_data[i];
                    }
                }
                if (iIsUp) {
                    netif_set_up(pnetif);                               /*  重启网口                    */
                }
            } else {
                _ErrorHandle(ENOTSUP);
            }
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifSubIoctl4
** 功能描述: 网络接口 ioctl 操作 (针对 ipv4)
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifSubIoctl4 (INT  iCmd, PVOID  pvArg)
{
           INT           iRet   = PX_ERROR;
    struct ifreq        *pifreq = LW_NULL;
    struct netif        *pnetif;
    struct sockaddr_in  *psockaddrin;
    
    pifreq = (struct ifreq *)pvArg;
    
    pnetif = __ifFindByName(pifreq->ifr_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
    case SIOCGIFADDR:                                                   /*  获取地址操作                */
    case SIOCGIFNETMASK:
    case SIOCGIFDSTADDR:
    case SIOCGIFBRDADDR:
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        psockaddrin->sin_len    = sizeof(struct sockaddr_in);
        psockaddrin->sin_family = AF_INET;
        psockaddrin->sin_port   = 0;
        break;
        
    case SIOCSIFADDR:                                                   /*  设置地址操作                */
    case SIOCSIFNETMASK:
    case SIOCSIFDSTADDR:
    case SIOCSIFBRDADDR:
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        break;
    }
    
    switch (iCmd) {                                                     /*  命令处理器                  */
        
    case SIOCGIFADDR:                                                   /*  获取网卡 IP                 */
        psockaddrin->sin_addr.s_addr = netif_ip4_addr(pnetif)->addr;
        iRet = ERROR_NONE;
        break;
    
    case SIOCGIFNETMASK:                                                /*  获取网卡 mask               */
        psockaddrin->sin_addr.s_addr = netif_ip4_netmask(pnetif)->addr;
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFDSTADDR:                                                /*  获取网卡目标地址            */
        if ((pnetif->flags & NETIF_FLAG_BROADCAST) == 0) {
            psockaddrin->sin_addr.s_addr = INADDR_ANY;
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCGIFBRDADDR:                                                /*  获取网卡广播地址            */
        if (pnetif->flags & NETIF_FLAG_BROADCAST) {
            psockaddrin->sin_addr.s_addr = (netif_ip4_addr(pnetif)->addr 
                                         | (~(netif_ip4_netmask(pnetif)->addr)));
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFADDR:                                                   /*  设置网卡地址                */
        if (psockaddrin->sin_family == AF_INET) {
            ip4_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            netif_set_ipaddr(pnetif, &ipaddr);
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case SIOCSIFNETMASK:                                                /*  设置网卡掩码                */
        if (psockaddrin->sin_family == AF_INET) {
            ip4_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            netif_set_netmask(pnetif, &ipaddr);
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case SIOCSIFDSTADDR:                                                /*  设置网卡目标地址            */
        if ((pnetif->flags & NETIF_FLAG_BROADCAST) == 0) {
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFBRDADDR:                                                /*  设置网卡广播地址            */
        if (pnetif->flags & NETIF_FLAG_BROADCAST) {
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifSubIoctl6
** 功能描述: 网络接口 ioctl 操作 (针对 ipv6)
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifSubIoctl6 (INT  iCmd, PVOID  pvArg)
{
#define __LWIP_GET_IPV6_FROM_NETIF() \
        pifr6addr->ifr6a_addr.un.u32_addr[0] = ip_2_ip6(&pnetif->ip6_addr[i])->addr[0]; \
        pifr6addr->ifr6a_addr.un.u32_addr[1] = ip_2_ip6(&pnetif->ip6_addr[i])->addr[1]; \
        pifr6addr->ifr6a_addr.un.u32_addr[2] = ip_2_ip6(&pnetif->ip6_addr[i])->addr[2]; \
        pifr6addr->ifr6a_addr.un.u32_addr[3] = ip_2_ip6(&pnetif->ip6_addr[i])->addr[3];
        
#define __LWIP_SET_IPV6_TO_NETIF() \
        ip_2_ip6(&pnetif->ip6_addr[i])->addr[0] = pifr6addr->ifr6a_addr.un.u32_addr[0]; \
        ip_2_ip6(&pnetif->ip6_addr[i])->addr[1] = pifr6addr->ifr6a_addr.un.u32_addr[1]; \
        ip_2_ip6(&pnetif->ip6_addr[i])->addr[2] = pifr6addr->ifr6a_addr.un.u32_addr[2]; \
        ip_2_ip6(&pnetif->ip6_addr[i])->addr[3] = pifr6addr->ifr6a_addr.un.u32_addr[3]; 
        
#define __LWIP_CMP_IPV6_WITH_NETIF() \
        (ip_2_ip6(&pnetif->ip6_addr[i])->addr[0] == pifr6addr->ifr6a_addr.un.u32_addr[0]) && \
        (ip_2_ip6(&pnetif->ip6_addr[i])->addr[1] == pifr6addr->ifr6a_addr.un.u32_addr[1]) && \
        (ip_2_ip6(&pnetif->ip6_addr[i])->addr[2] == pifr6addr->ifr6a_addr.un.u32_addr[2]) && \
        (ip_2_ip6(&pnetif->ip6_addr[i])->addr[3] == pifr6addr->ifr6a_addr.un.u32_addr[3]) 

           INT           i;
           INT           iSize;
           INT           iNum = 0;
           INT           iRet    = PX_ERROR;
    struct in6_ifreq    *pifreq6 = LW_NULL;
    struct in6_ifr_addr *pifr6addr;
    struct netif        *pnetif;
    
    pifreq6 = (struct in6_ifreq *)pvArg;
    
    pnetif = __ifFindByIndex(pifreq6->ifr6_ifindex);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }
    
    iSize = pifreq6->ifr6_len / sizeof(struct in6_ifr_addr);            /*  缓冲区个数                  */
    pifr6addr = pifreq6->ifr6_addr_array;
    
    switch (iCmd) {                                                     /*  命令处理器                  */
    
    case SIOCGIFADDR6:                                                  /*  获取网卡 IP                 */
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            if (iNum >= iSize) {
                break;
            }
            if (ip6_addr_isvalid(pnetif->ip6_addr_state[i])) {
                __LWIP_GET_IPV6_FROM_NETIF();
                if (ip6_addr_isloopback(ip_2_ip6(&pnetif->ip6_addr[i]))) {
                    pifr6addr->ifr6a_prefixlen = 128;
                } else if (ip6_addr_islinklocal(ip_2_ip6(&pnetif->ip6_addr[i]))) {
                    pifr6addr->ifr6a_prefixlen = 6;
                } else {
                    pifr6addr->ifr6a_prefixlen = 64;                    /*  TODO: 目前无法获取          */
                }
                iNum++;
                pifr6addr++;
            }
        }
        pifreq6->ifr6_len = iNum * sizeof(struct in6_ifr_addr);         /*  回写实际大小                */
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFADDR6:                                                  /*  设置网卡 IP                 */
        if (iSize != 1) {                                               /*  每次只能设置一个 IP 地址    */
            _ErrorHandle(EOPNOTSUPP);
            break;
        }
        if (IN6_IS_ADDR_LOOPBACK(&pifr6addr->ifr6a_addr) ||
            IN6_IS_ADDR_LINKLOCAL(&pifr6addr->ifr6a_addr)) {            /*  不能手动设置这两种类型的地址*/
            _ErrorHandle(EADDRNOTAVAIL);
            break;
        }
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {                 /*  首先试图添加                */
            if (ip6_addr_isinvalid(pnetif->ip6_addr_state[i])) {
                __LWIP_SET_IPV6_TO_NETIF();
                pnetif->ip6_addr_state[i] = IP6_ADDR_VALID | IP6_ADDR_TENTATIVE;
                netif_ip6_addr_set_valid_life(pnetif, i, IP6_ADDR_LIFE_STATIC);
                netif_ip6_addr_set_pref_life(pnetif, i, IP6_ADDR_LIFE_STATIC);
                break;
            }
        }
        if (i >= LWIP_IPV6_NUM_ADDRESSES) {                             /*  无法添加                    */
            for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {             /*  优先覆盖暂定地址            */
                if (!ip6_addr_islinklocal(ip_2_ip6(&pnetif->ip6_addr[i])) &&
                    ip6_addr_istentative(pnetif->ip6_addr_state[i])) {
                    __LWIP_SET_IPV6_TO_NETIF();
                    pnetif->ip6_addr_state[i] = IP6_ADDR_VALID | IP6_ADDR_TENTATIVE;
                    netif_ip6_addr_set_valid_life(pnetif, i, IP6_ADDR_LIFE_STATIC);
                    netif_ip6_addr_set_pref_life(pnetif, i, IP6_ADDR_LIFE_STATIC);
                    break;
                }
            }
        }
        if (i >= LWIP_IPV6_NUM_ADDRESSES) {                             /*  无法添加                    */
            for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {             /*  替换非 linklocal 地址       */
                if (!ip6_addr_islinklocal(ip_2_ip6(&pnetif->ip6_addr[i]))) {
                    __LWIP_SET_IPV6_TO_NETIF();
                    pnetif->ip6_addr_state[i] = IP6_ADDR_VALID | IP6_ADDR_TENTATIVE;
                    netif_ip6_addr_set_valid_life(pnetif, i, IP6_ADDR_LIFE_STATIC);
                    netif_ip6_addr_set_pref_life(pnetif, i, IP6_ADDR_LIFE_STATIC);
                    break;
                }
            }
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCDIFADDR6:                                                  /*  删除一个 IPv6 地址          */
        if (iSize != 1) {                                               /*  每次只能设置一个 IP 地址    */
            _ErrorHandle(EOPNOTSUPP);
            break;
        }
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            if (ip6_addr_isvalid(pnetif->ip6_addr_state[i])) {
                if (__LWIP_CMP_IPV6_WITH_NETIF()) {                     /*  TODO 没有判断前缀长度       */
                    pnetif->ip6_addr_state[i] = IP6_ADDR_INVALID;
                    break;
                }
            }
        }
        iRet = ERROR_NONE;
        break;
        
    default:
        _ErrorHandle(ENOSYS);                                           /*  TODO: 其他命令暂未实现      */
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifSubIoctlCommon
** 功能描述: 通用网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifSubIoctlCommon (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    switch (iCmd) {
    
    case SIOCGIFCONF: {                                                 /*  获得网卡列表                */
            struct ifconf *pifconf = (struct ifconf *)pvArg;
            __ifConf(pifconf);
            iRet = ERROR_NONE;
            break;
        }
        
    case SIOCGIFNUM: {                                                  /*  获得网络接口数量            */
            if (pvArg) {
                *(INT *)pvArg = netif_get_num();
                iRet = ERROR_NONE;
            } else {
                _ErrorHandle(EINVAL);
            }
            break;
        }
    
    case SIOCGSIZIFCONF: {                                              /*  SIOCGIFCONF 所需缓冲大小    */
            INT  *piSize = (INT *)pvArg;
            __ifConfSize(piSize);
            iRet = ERROR_NONE;
            break;
        }
    
    case SIOCGSIZIFREQ6: {                                              /*  获得指定网口 ipv6 地址数量  */
            struct in6_ifreq *pifreq6 = (struct in6_ifreq *)pvArg;
            iRet = __ifReq6Size(pifreq6);
            break;
        }
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlInet
** 功能描述: INET 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __ifIoctlInet (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if (pvArg == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (iRet);
    }
    
    if (iCmd == SIOCGIFNAME) {                                          /*  获得网卡名                  */
        struct ifreq *pifreq = (struct ifreq *)pvArg;
        if (if_indextoname(pifreq->ifr_ifindex, pifreq->ifr_name)) {
            iRet = ERROR_NONE;
        }
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
    case SIOCGIFCONF:                                                   /*  通用网络接口操作            */
    case SIOCGIFNUM:
    case SIOCGSIZIFCONF:
    case SIOCGSIZIFREQ6:
        LWIP_NETIF_LOCK();                                              /*  进入临界区                  */
        iRet = __ifSubIoctlCommon(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  退出临界区                  */
        break;
    
    case SIOCSIFFLAGS:                                                  /*  基本网络接口操作            */
    case SIOCGIFFLAGS:
    case SIOCGIFTYPE:
    case SIOCGIFINDEX:
    case SIOCGIFMTU:
    case SIOCSIFMTU:
    case SIOCGIFHWADDR:
    case SIOCSIFHWADDR:
        LWIP_NETIF_LOCK();                                              /*  进入临界区                  */
        iRet = __ifSubIoctlIf(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  退出临界区                  */
        break;
    
    case SIOCGIFADDR:                                                   /*  ipv4 操作                   */
    case SIOCGIFNETMASK:
    case SIOCGIFDSTADDR:
    case SIOCGIFBRDADDR:
    case SIOCSIFADDR:
    case SIOCSIFNETMASK:
    case SIOCSIFDSTADDR:
    case SIOCSIFBRDADDR:
        LWIP_NETIF_LOCK();                                              /*  进入临界区                  */
        iRet = __ifSubIoctl4(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  退出临界区                  */
        break;
        
    case SIOCGIFADDR6:                                                  /*  ipv6 操作                   */
    case SIOCGIFNETMASK6:
    case SIOCGIFDSTADDR6:
    case SIOCSIFADDR6:
    case SIOCSIFNETMASK6:
    case SIOCSIFDSTADDR6:
    case SIOCDIFADDR6:
        LWIP_NETIF_LOCK();                                              /*  进入临界区                  */
        iRet = __ifSubIoctl6(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  退出临界区                  */
        break;
    
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlWireless
** 功能描述: WEXT 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_NET_WIRELESS_EN > 0

INT  __ifIoctlWireless (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if ((iCmd >= SIOCIWFIRST) && (iCmd <= SIOCIWLASTPRIV)) {            /*  无线连接设置                */
        LWIP_NETIF_LOCK();                                              /*  进入临界区                  */
        iRet = wext_handle_ioctl(iCmd, (struct ifreq *)pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  退出临界区                  */
        if (iRet) {
            _ErrorHandle(lib_abs(iRet));
            iRet = PX_ERROR;
        }
    } else {
        _ErrorHandle(ENOSYS);
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
** 函数名称: __ifIoctlPacket
** 功能描述: PACKET 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __ifIoctlPacket (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if (pvArg == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (iRet);
    }
    
    if (iCmd == SIOCGIFNAME) {                                          /*  获得网卡名                  */
        struct ifreq *pifreq = (struct ifreq *)pvArg;
        if (if_indextoname(pifreq->ifr_ifindex, pifreq->ifr_name)) {
            iRet = ERROR_NONE;
        }
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
    case SIOCGIFCONF:                                                   /*  通用网络接口操作            */
    case SIOCGIFNUM:
    case SIOCGSIZIFCONF:
    case SIOCGSIZIFREQ6:
        LWIP_NETIF_LOCK();                                              /*  进入临界区                  */
        iRet = __ifSubIoctlCommon(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  退出临界区                  */
        break;
    
    case SIOCSIFFLAGS:                                                  /*  基本网络接口操作            */
    case SIOCGIFFLAGS:
    case SIOCGIFTYPE:
    case SIOCGIFINDEX:
    case SIOCGIFMTU:
    case SIOCSIFMTU:
    case SIOCGIFHWADDR:
    case SIOCSIFHWADDR:
        LWIP_NETIF_LOCK();                                              /*  进入临界区                  */
        iRet = __ifSubIoctlIf(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  退出临界区                  */
        break;
    
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
