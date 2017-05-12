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
** ��   ��   ��: lwip_hook.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 05 �� 12 ��
**
** ��        ��: lwip SylixOS HOOK.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/tcpip.h"
#include "lwip_hook.h"
/*********************************************************************************************************
  �ص���
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        IPHOOK_lineManage;
    FUNCPTR             IPHOOK_pfuncHook;
    CHAR                IPHOOK_cName[1];
} IP_HOOK_NODE;
typedef IP_HOOK_NODE   *PIP_HOOK_NODE;
/*********************************************************************************************************
  �ص���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER   _G_plineIpHook;
/*********************************************************************************************************
** ��������: lwip_ip_hook
** ��������: lwip ip �ص�����
** �䡡��  : ip_type       ip ���� IP_HOOK_V4 / IP_HOOK_V6
**           hook_type     �ص����� IP_HT_PRE_ROUTING / IP_HT_POST_ROUTING / IP_HT_LOCAL_IN ...
**           p             ���ݰ�
**           in            ��������ӿ�
**           out           �������ӿ�
** �䡡��  : 1: ����
**           0: ͨ��
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: net_ip_hook_add
** ��������: lwip ip ��ӻص�����
** �䡡��  : name          ip �ص�����
**           hook          �ص�����
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: net_ip_hook_delete
** ��������: lwip ip ɾ���ص�����
** �䡡��  : hook          �ص�����
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
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
