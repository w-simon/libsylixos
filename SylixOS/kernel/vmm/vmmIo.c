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
** 文   件   名: vmmIo.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 05 月 21 日
**
** 描        述: 平台无关虚拟内存管理, 设备内存映射.

** BUG
2018.04.06  修正 API_VmmIoRemapEx() 针对非对齐物理内存映射错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
** 函数名称: API_VmmIoRemapEx
** 功能描述: 将物理 IO 空间指定内存映射到逻辑空间. (用户可指定 CACHE 与否)
** 输　入  : pvPhysicalAddr     物理内存地址
**           stSize             需要映射的内存大小
**           ulFlags            内存属性
** 输　出  : 映射到的逻辑内存地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemapEx (PVOID  pvPhysicalAddr, size_t stSize, ULONG  ulFlags)
{
    REGISTER addr_t         ulPhyPageAddr;
    REGISTER ULONG          ulPageNum;
    REGISTER ULONG          ulUpPad;
    
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             ULONG          ulError;
    
    ulUpPad        = (addr_t)pvPhysicalAddr & (LW_CFG_VMM_PAGE_SIZE - 1);
    ulPhyPageAddr  = (addr_t)pvPhysicalAddr - ulUpPad;
    stSize        += (size_t)ulUpPad;
    
    ulPageNum = (ULONG)(stSize >> LW_CFG_VMM_PAGE_SHIFT);
    if (stSize & ~LW_CFG_VMM_PAGE_MASK) {
        ulPageNum++;
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __vmmVirDevPageAlloc(ulPageNum);                   /*  分配连续虚拟页面            */
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
    ulError = __vmmLibPageMap(ulPhyPageAddr,
                              pvmpageVirtual->PAGE_ulPageAddr,
                              ulPageNum, 
                              ulFlags);                                 /*  映射为连续虚拟地址          */
    if (ulError) {                                                      /*  映射错误                    */
        __vmmVirDevPageFree(pvmpageVirtual);                            /*  释放虚拟地址空间            */
        __VMM_UNLOCK();
        _ErrorHandle(ulError);
        return  (LW_NULL);
    }
    
    pvmpageVirtual->PAGE_ulFlags = ulFlags;
    
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  插入逻辑空间反查表          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_IOREMAP,
                      pvmpageVirtual->PAGE_ulPageAddr, pvPhysicalAddr, stSize, LW_NULL);
    
    return  ((PVOID)(pvmpageVirtual->PAGE_ulPageAddr + ulUpPad));
}
/*********************************************************************************************************
** 函数名称: API_VmmIoRemap
** 功能描述: 将物理 IO 空间指定内存映射到逻辑空间. (非 CACHE)
** 输　入  : pvPhysicalAddr     物理内存地址
**           stSize             需要映射的内存大小
** 输　出  : 映射到的逻辑内存地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_VmmIoRemap (PVOID  pvPhysicalAddr, size_t stSize)
{
    return  (API_VmmIoRemapEx(pvPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** 函数名称: API_VmmIoUnmap
** 功能描述: 释放 ioremap 占用的逻辑空间
** 输　入  : pvVirtualMem    虚拟地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmIoUnmap (PVOID  pvVirtualAddr)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             addr_t         ulAddr = (addr_t)pvVirtualAddr;
    
    ulAddr &= LW_CFG_VMM_PAGE_MASK;                                     /*  页面对齐地址                */
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return;
    }
    
#if LW_CFG_CACHE_EN > 0
    API_CacheClear(DATA_CACHE, (PVOID)pvmpageVirtual->PAGE_ulPageAddr,
                   (size_t)(pvmpageVirtual->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibPageMap(pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulCount, 
                    LW_VMM_FLAG_FAIL);                                  /*  不允许访问                  */
    
    __areaVirtualUnlinkPage(pvmpageVirtual->PAGE_ulPageAddr,
                            pvmpageVirtual);
    
    __vmmVirDevPageFree(pvmpageVirtual);                                /*  删除虚拟页面                */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_IOUNMAP,
                      pvVirtualAddr, LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_VmmIoRemapNocache
** 功能描述: 将物理 IO 空间指定内存映射到逻辑空间. (非 CACHE)
** 输　入  : pvPhysicalAddr     物理内存地址
**           stSize             需要映射的内存大小
** 输　出  : 映射到的逻辑内存地址
** 全局变量: 
** 调用模块: 
                                           API 函数
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
