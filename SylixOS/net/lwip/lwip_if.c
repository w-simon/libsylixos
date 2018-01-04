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
** ��   ��   ��: lwip_if.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 10 ��
**
** ��        ��: posix net/if �ӿ�.

** BUG:
2011.07.07  _G_ulNetifLock ����.
2014.03.22  �Ż��������ӿ�.
2014.06.24  ���� if_down �� if_up API.
2014.12.01  ֹͣ���������ʹ�� dhcp ��Ҫֹͣ��Լ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "net/if.h"
#include "lwip_if.h"
/*********************************************************************************************************
** ��������: if_down
** ��������: �ر�����
** �䡡��  : ifname        if name
** �䡡��  : �ر��Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_down (const char *ifname)
{
    INT            iError;
    struct netif  *pnetif;
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif) {
        if (pnetif->flags & NETIF_FLAG_UP) {
#if LWIP_DHCP > 0
            if (netif_dhcp_data(pnetif)) {
                netifapi_dhcp_release(pnetif);                          /*  ��� DHCP ��Լ, ͬʱֹͣ����*/
                netifapi_dhcp_stop(pnetif);                             /*  �ͷ���Դ                    */
            }
#endif                                                                  /*  LWIP_DHCP > 0               */
            netifapi_netif_set_down(pnetif);                            /*  ��������                    */
            iError = ERROR_NONE;
        } else {
            _ErrorHandle(EALREADY);
            iError = PX_ERROR;
        }
    } else {
        _ErrorHandle(ENXIO);
        iError = PX_ERROR;
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: if_up
** ��������: ������
** �䡡��  : ifname        if name
** �䡡��  : �����Ƿ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_up (const char *ifname)
{
    INT            iError;
    struct netif  *pnetif;
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif) {
        if (!(pnetif->flags & NETIF_FLAG_UP)) {
            netifapi_netif_set_up(pnetif);
#if LWIP_DHCP > 0
            if (pnetif->flags2 & NETIF_FLAG2_DHCP) {
                ip4_addr_t   inaddrNone;
                lib_bzero(&inaddrNone, sizeof(ip_addr_t));
                netifapi_netif_set_addr(pnetif, &inaddrNone, &inaddrNone, &inaddrNone);
                netifapi_dhcp_start(pnetif);
            }
#endif                                                                  /*  LWIP_DHCP > 0               */
            iError = ERROR_NONE;
        } else {
            _ErrorHandle(EALREADY);
            iError = PX_ERROR;
        }
    } else {
        _ErrorHandle(ENXIO);
        iError = PX_ERROR;
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: if_isup
** ��������: �����Ƿ�ʹ��
** �䡡��  : ifname        if name
** �䡡��  : �����Ƿ�ʹ�� 0: ����  1: ʹ��  -1: �������ִ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_isup (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif) {
        if (pnetif->flags & NETIF_FLAG_UP) {
            iRet = 1;
        } else {
            iRet = 0;
        }
    } else {
        _ErrorHandle(ENXIO);
        iRet = PX_ERROR;
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: if_islink
** ��������: �����Ƿ��Ѿ�����
** �䡡��  : ifname        if name
** �䡡��  : �����Ƿ����� 0: ����  1: û������  -1: �������ִ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_islink (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif) {
        if (pnetif->flags & NETIF_FLAG_LINK_UP) {
            iRet = 1;
        } else {
            iRet = 0;
        }
    } else {
        _ErrorHandle(ENXIO);
        iRet = PX_ERROR;
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: if_set_dhcp
** ��������: �������� dhcp ѡ�� (��������������ʱ����)
** �䡡��  : ifname        if name
**           en            1: ʹ�� dhcp  0: ���� dhcp
** �䡡��  : OK or ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_set_dhcp (const char *ifname, int en)
{
    INT     iRet = PX_ERROR;

#if LWIP_DHCP > 0
    struct netif  *pnetif;
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif) {
        if (pnetif->flags & NETIF_FLAG_UP) {
            _ErrorHandle(EISCONN);
            
        } else {
            if (en) {
                pnetif->flags2 |= NETIF_FLAG2_DHCP;
            } else {
                pnetif->flags2 &= ~NETIF_FLAG2_DHCP;
            }
            iRet = ERROR_NONE;
        }
    } else {
        _ErrorHandle(ENXIO);
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
#else
    _ErrorHandle(ENOSYS);
#endif                                                                  /*  LWIP_DHCP > 0               */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: if_get_dhcp
** ��������: ��ȡ���� dhcp ѡ��
** �䡡��  : ifname        if name
** �䡡��  : 1: ʹ�� dhcp  0: ���� dhcp -1: �������ִ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_get_dhcp (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif) {
        if (pnetif->flags2 & NETIF_FLAG2_DHCP) {
            iRet = 1;
        } else {
            iRet = 0;
        }
    } else {
        _ErrorHandle(ENXIO);
        iRet = PX_ERROR;
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: if_nametoindex
** ��������: map a network interface name to its corresponding index
** �䡡��  : ifname        if name
** �䡡��  : index
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
unsigned  if_nametoindex (const char *ifname)
{
    struct netif    *pnetif;
    unsigned         uiIndex = 0;
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif) {
        uiIndex = (unsigned)pnetif->num;
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    return  (uiIndex);
}
/*********************************************************************************************************
** ��������: if_indextoname
** ��������: map a network interface index to its corresponding name
** �䡡��  : ifindex       if index
**           ifname        if name buffer at least {IF_NAMESIZE} bytes
** �䡡��  : index
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
char *if_indextoname (unsigned  ifindex, char *ifname)
{
    struct netif    *pnetif;

    if (!ifname) {
        errno = EINVAL;
    }
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    pnetif = (struct netif *)netif_get_by_index(ifindex);
    if (pnetif) {
        ifname[0] = pnetif->name[0];
        ifname[1] = pnetif->name[1];
        lib_itoa(pnetif->num, &ifname[2], 10);
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    if (pnetif) {
        return  (ifname);
    } else {
        errno = ENXIO;
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: if_nameindex
** ��������: return all network interface names and indexes
** �䡡��  : NONE
** �䡡��  : An array of structures identifying local interfaces
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct if_nameindex *if_nameindex (void)
{
    struct netif           *pnetif;
    int                     iNum = 1;                                   /*  ��Ҫһ�����е�λ��          */
    struct if_nameindex    *pifnameindexArry;
    
    LWIP_NETIF_LOCK();                                                  /*  �����ٽ���                  */
    for(pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {
        iNum++;
    }
    pifnameindexArry = (struct if_nameindex *)__SHEAP_ALLOC(sizeof(struct if_nameindex) * (size_t)iNum);
    if (pifnameindexArry) {
        int     i = 0;
        
        for (pnetif  = netif_list; 
             pnetif != LW_NULL; 
             pnetif  = pnetif->next) {
            
            pifnameindexArry[i].if_index = (unsigned)pnetif->num;
            pifnameindexArry[i].if_name_buf[0] = pnetif->name[0];
            pifnameindexArry[i].if_name_buf[1] = pnetif->name[1];
            lib_itoa(pnetif->num, &pifnameindexArry[i].if_name_buf[2], 10);
            pifnameindexArry[i].if_name = pifnameindexArry[i].if_name_buf;
            i++;
        }
        
        pifnameindexArry[i].if_index = 0;
        pifnameindexArry[i].if_name_buf[0] = PX_EOS;
        pifnameindexArry[i].if_name = pifnameindexArry[i].if_name_buf;
        
    } else {
        errno = ENOMEM;
    }
    LWIP_NETIF_UNLOCK();                                                /*  �˳��ٽ���                  */
    
    return  (pifnameindexArry);
}
/*********************************************************************************************************
** ��������: if_freenameindex
** ��������: free memory allocated by if_nameindex
             the application shall not use the array of which ptr is the address.
** �䡡��  : ptr           shall be a pointer that was returned by if_nameindex().
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
void  if_freenameindex (struct if_nameindex *ptr)
{
    if (ptr) {
        __SHEAP_FREE(ptr);
    }
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
