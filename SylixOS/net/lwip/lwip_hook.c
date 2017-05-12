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

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
