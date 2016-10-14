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
** ��   ��   ��: vmmIo.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 05 �� 21 ��
**
** ��        ��: ƽ̨�޹������ڴ����, �豸�ڴ�ӳ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
** ��������: API_VmmIoRemapEx
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�û���ָ�� CACHE ���)
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
**           ulFlags            �ڴ�����
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemapEx (PVOID  pvPhysicalAddr, size_t stSize, ULONG  ulFlags)
{
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);

    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             ULONG          ulError;
    
    if (stExcess) {
        ulPageNum++;                                                    /*  ȷ����ҳ����                */
    }

    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __vmmVirDevPageAlloc(ulPageNum);                   /*  ������������ҳ��            */
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
    ulError = __vmmLibPageMap((addr_t)pvPhysicalAddr,                   /*  ��ʹ�� CACHE                */
                              pvmpageVirtual->PAGE_ulPageAddr,
                              ulPageNum, 
                              ulFlags);                                 /*  ӳ��Ϊ���������ַ          */
    if (ulError) {                                                      /*  ӳ�����                    */
        __vmmVirDevPageFree(pvmpageVirtual);                            /*  �ͷ������ַ�ռ�            */
        __VMM_UNLOCK();
        _ErrorHandle(ulError);
        return  (LW_NULL);
    }
    
    pvmpageVirtual->PAGE_ulFlags = ulFlags;
    
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  �����߼��ռ䷴���          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_IOREMAP,
                      pvmpageVirtual->PAGE_ulPageAddr, pvPhysicalAddr, stSize, LW_NULL);
    
    return  ((PVOID)pvmpageVirtual->PAGE_ulPageAddr);
}
/*********************************************************************************************************
** ��������: API_VmmIoRemap
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�� CACHE)
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemap (PVOID  pvPhysicalAddr, size_t stSize)
{
    return  (API_VmmIoRemapEx(pvPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** ��������: API_VmmIoUnmap
** ��������: �ͷ� ioremap ռ�õ��߼��ռ�
** �䡡��  : pvVirtualMem    �����ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmIoUnmap (PVOID  pvVirtualAddr)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             addr_t         ulAddr = (addr_t)pvVirtualAddr;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return;
    }
    
#if LW_CFG_CACHE_EN > 0
    API_CacheClear(DATA_CACHE, (PVOID)pvmpageVirtual->PAGE_ulPageAddr,
                   (size_t)(pvmpageVirtual->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibPageMap(pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulCount, 
                    LW_VMM_FLAG_FAIL);                                  /*  ���������                  */
    
    __areaVirtualUnlinkPage(pvmpageVirtual->PAGE_ulPageAddr,
                            pvmpageVirtual);
    
    __vmmVirDevPageFree(pvmpageVirtual);                                /*  ɾ������ҳ��                */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_IOUNMAP,
                      pvVirtualAddr, LW_NULL);
}
/*********************************************************************************************************
** ��������: API_VmmIoRemapNocache
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�� CACHE)
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
** �䡡��  : ӳ�䵽���߼��ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemapNocache (PVOID  pvPhysicalAddr, 
                              size_t stSize)
{
    return  (API_VmmIoRemapEx(pvPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
