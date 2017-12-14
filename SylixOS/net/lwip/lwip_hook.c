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
** 文   件   名: lwip_hook.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 05 月 12 日
**
** 描        述: lwip SylixOS HOOK.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/tcpip.h"
#include "net/if_flags.h"
#include "lwip_hook.h"
/*********************************************************************************************************
  回调点
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        IPHOOK_lineManage;
    FUNCPTR             IPHOOK_pfuncHook;
    CHAR                IPHOOK_cName[1];
} IP_HOOK_NODE;
typedef IP_HOOK_NODE   *PIP_HOOK_NODE;
/*********************************************************************************************************
  回调链
*********************************************************************************************************/
static LW_LIST_LINE_HEADER   _G_plineIpHook;
/*********************************************************************************************************
** 函数名称: lwip_ip_hook
** 功能描述: lwip ip 回调函数
** 输　入  : ip_type       ip 类型 IP_HOOK_V4 / IP_HOOK_V6
**           hook_type     回调类型 IP_HT_PRE_ROUTING / IP_HT_POST_ROUTING / IP_HT_LOCAL_IN ...
**           p             数据包
**           in            输入网络接口
**           out           输出网络接口
** 输　出  : 1: 丢弃
**           0: 通过
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  lwip_ip_hook (int ip_type, int hook_type, struct pbuf *p, struct netif *in, struct netif *out)
{
    PLW_LIST_LINE   pline;
    PIP_HOOK_NODE   pipnod;
    
    for (pline  = _G_plineIpHook;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        pipnod = _LIST_ENTRY(pline, IP_HOOK_NODE, IPHOOK_lineManage);
        if (pipnod->IPHOOK_pfuncHook(ip_type, hook_type, p, in, out)) {
            return  (1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_add
** 功能描述: lwip ip 添加回调函数
** 输　入  : name          ip 回调类型
**           hook          回调函数
** 输　出  : -1: 失败
**            0: 成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  net_ip_hook_add (const char *name, int (*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                                    struct netif *in, struct netif *out))
{
    PIP_HOOK_NODE   pipnod;
    
    if (!name || !hook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pipnod = (PIP_HOOK_NODE)__SHEAP_ALLOC(sizeof(IP_HOOK_NODE) + lib_strlen(name));
    if (!pipnod) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    pipnod->IPHOOK_pfuncHook = hook;
    lib_strcpy(pipnod->IPHOOK_cName, name);
    
    LOCK_TCPIP_CORE();
    _List_Line_Add_Ahead(&pipnod->IPHOOK_lineManage, &_G_plineIpHook);
    UNLOCK_TCPIP_CORE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_delete
** 功能描述: lwip ip 删除回调函数
** 输　入  : hook          回调函数
** 输　出  : -1: 失败
**            0: 成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  net_ip_hook_delete (int (*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                     struct netif *in, struct netif *out))
{
    PLW_LIST_LINE   pline;
    PIP_HOOK_NODE   pipnod;
    
    if (!hook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LOCK_TCPIP_CORE();
    for (pline  = _G_plineIpHook;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        pipnod = _LIST_ENTRY(pline, IP_HOOK_NODE, IPHOOK_lineManage);
        if (pipnod->IPHOOK_pfuncHook == hook) {
            _List_Line_Del(&pipnod->IPHOOK_lineManage, &_G_plineIpHook);
            break;
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (pline) {
        __SHEAP_FREE(pipnod);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_netdev
** 功能描述: 获取 netdev 结构
** 输　入  : pnetif        网络接口
** 输　出  : 网络设备
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
netdev_t  *net_ip_hook_netif_get_netdev (struct netif *pnetif)
{
    netdev_t  *netdev;
    
    if (pnetif) {
        netdev = (netdev_t *)(pnetif->state);
        if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
            return  (netdev);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_ipaddr
** 功能描述: 获取 ipv4 地址
** 输　入  : pnetif        网络接口
** 输　出  : ipv4 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
const ip4_addr_t  *net_ip_hook_netif_get_ipaddr (struct netif *pnetif)
{
    return  (pnetif ? netif_ip4_addr(pnetif) : LW_NULL);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_netmask
** 功能描述: 获取 ipv4 掩码
** 输　入  : pnetif        网络接口
** 输　出  : ipv4 掩码
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
const ip4_addr_t  *net_ip_hook_netif_get_netmask (struct netif *pnetif)
{
    return  (pnetif ? netif_ip4_netmask(pnetif) : LW_NULL);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_gw
** 功能描述: 获取 ipv4 网卡默认网关
** 输　入  : pnetif        网络接口
** 输　出  : ipv4 网卡默认网关
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
const ip4_addr_t  *net_ip_hook_netif_get_gw (struct netif *pnetif)
{
    return  (pnetif ? netif_ip4_gw(pnetif) : LW_NULL);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_ip6addr
** 功能描述: 获取网卡 ipv6 地址
** 输　入  : pnetif        网络接口
**           addr_index    第几个 ipv6 地址
**           addr_state    地址状态 IP6_ADDR_INVALID / IP6_ADDR_VALID / IP6_ADDR_TENTATIVE ...
** 输　出  : ipv6 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
const ip6_addr_t  *net_ip_hook_netif_get_ip6addr (struct netif *pnetif, int  addr_index, int *addr_state)
{
    if (pnetif && (addr_index >= 0 && (addr_index < LWIP_IPV6_NUM_ADDRESSES))) {
        if (addr_state) {
            *addr_state = netif_ip6_addr_state(pnetif, addr_index);
        }
        
        return  (netif_ip6_addr(pnetif, addr_index));
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_hwaddr
** 功能描述: 获取网卡物理地址
** 输　入  : pnetif        网络接口
**           hwaddr_len    物理地址长度
** 输　出  : 物理地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
UINT8  *net_ip_hook_netif_get_hwaddr (struct netif *pnetif, int *hwaddr_len)
{
    if (pnetif) {
        if (hwaddr_len) {
            *hwaddr_len = pnetif->hwaddr_len;
        }
        
        return  (pnetif->hwaddr);
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_index
** 功能描述: 获取网卡 index
** 输　入  : pnetif        网络接口
** 输　出  : index
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  net_ip_hook_netif_get_index (struct netif *pnetif)
{
    return  (pnetif ? pnetif->num : PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_name
** 功能描述: 获取网卡 index
** 输　入  : pnetif        网络接口
**           name          名字
**           size          缓冲区长度
** 输　出  : 名字长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  net_ip_hook_netif_get_name (struct netif *pnetif, char *name, size_t size)
{
    if (pnetif && name && (size > 3)) {
        return  (PX_ERROR);
    }
    
    name[0] = pnetif->name[0];
    name[1] = pnetif->name[1];
    name[2] = (char)(pnetif->num + '0');
    name[3] = PX_EOS;
    
    return  (3);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_type
** 功能描述: 获取网卡类型
** 输　入  : pnetif        网络接口
**           type          接口类型 IFT_PPP / IFT_ETHER / IFT_LOOP / IFT_OTHER ...
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  net_ip_hook_netif_get_type (struct netif *pnetif, int *type)
{
    if (pnetif && type) {
        if ((pnetif->flags & NETIF_FLAG_BROADCAST) == 0) {
            *type = IFT_PPP;
        } else if (pnetif->flags & (NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP)) {
            *type = IFT_ETHER;
        } else if (pnetif->num == 0) {
            *type = IFT_LOOP;
        } else {
            *type = IFT_OTHER;
        }
        
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_flags
** 功能描述: 获取网卡 flags
** 输　入  : pnetif        网络接口
**           flags         接口状态 IFF_UP / IFF_BROADCAST / IFF_POINTOPOINT / IFF_RUNNING ...
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  net_ip_hook_netif_get_flags (struct netif *pnetif, int *flags)
{
    if (pnetif && flags) {
        *flags = netif_get_flags(pnetif);
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: net_ip_hook_netif_get_linkspeed
** 功能描述: 获取网卡链接速度
** 输　入  : pnetif        网络接口
** 输　出  : 链接速度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
UINT64  net_ip_hook_netif_get_linkspeed (struct netif *pnetif)
{
    netdev_t  *netdev;
    
    if (pnetif) {
        netdev = (netdev_t *)(pnetif->state);
        if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
            return  (netdev->speed);
        } else {
            return  (pnetif->link_speed);
        }
    }
    
    return  (0);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
