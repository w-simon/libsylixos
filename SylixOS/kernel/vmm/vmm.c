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
** ��   ��   ��: vmm.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ����ҳ������.

** BUG
2009.03.08  ���е��ڴ��ͷŶ���ҳ���趨Ϊ����ʧЧ״̬.
2009.04.15  ����ҳ��ֱ��ӳ���߼��ĵ�ַ������, ����ʹ BSP ���򷽱㴦��߶��쳣����������.
2009.06.18  ����ͨ�������ַ��ѯ�����ַ�� API.
2009.06.23  ���� API_VmmDmaAllocAlign ����. �ѽ��һЩ����� DMA ���ڴ���������Ҫ��.
2009.07.11  API_VmmLibInit() �����Ե���һ��.
2009.11.10  DMA �ڴ����, ����ֵΪ�����ַ.
2009.11.13  ��� vmmMalloc �������ڴ���Ƭ���� BUG.
            �����ȡ zone �� virtual ��Ϣ�� API.
2010.08.13  ����ע��.
2011.03.02  �����޸��ڴ����Ե� API �������Խ���Щ�ڴ���Ϊֻ��, Ϊ�ں�ģ��Ĵ���������.
2011.05.16  �� vmmMalloc vmmFree �������� vmmMalloc.c �ļ���.
2011.08.02  ȥ�� page alloc ��ز���, �����ڴ������� dma alloc ���.
2011.12.08  API_VmmLibInit() ���ȳ�ʼ������ҳ����ƿ��.
2011.12.10  ��ȡ��ӳ���ϵ���߸ı�ӳ���ϵ����ʱ, ��Ҫ��д����Ч cache.
2012.04.18  ���� posix getpagesize api.
2013.03.16  ����������ڴ�ķ��亯��, ��Щ�����ڴ���������ֱ��ʹ��, ��������Ч�������ַӳ���ʹ��.
            DMA �����ڴ���亯������������ڴ����ֱ�ӷ���, ��Ϊ DMA �����ڴ������ռ�����ظ�.
2013.07.20  SMP ����� MMU ��ʼ���������Ӻ˷���ĳ�ʼ������.
2014.09.18  ���� API_VmmIoRemapEx() ��ָ���ڴ�����.
2015.05.14  API_VmmLibSecondaryInit() ���� VMM ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __VMM_MAIN_FILE
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
   
   �����Ǵ���Ӳ�� MMU ��Ԫ�Ĵ�����ϵͳ�ṹͼ
   
                                                   physical address
                                                                         RAM
   +---------------+    write    +---------------+     write       +-------------+
   |               |  ---------> |               | --------------> |             |
   |   PROCESSOR   |    read     |      MMU      |     read        |             |
   |               |  <--------- |               | <-------------- |   physical  |
   +---------------+             +---------------+ <------\        |    memory   |
                                                          |        |             |
                   virtual address                        |        |             |
                                                          |        |             |
                                                          |        |-------------|
                                                          |        |             |
                                                          \------- | translation |
                                                                   |    table    |
                                                                   |             |
                                                                   +-------------+
  
  ע��: ���е��ͷŲ�����û��ִ�� cache ��д���߸��²���, ���һ���ɵ��������.
*********************************************************************************************************/
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
  �ⲿȫ�ֱ�������
*********************************************************************************************************/
extern LW_VMM_ZONE          _G_vmzonePhysical[LW_CFG_VMM_ZONE_NUM];     /*  ��������                    */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
LW_MMU_OP                   _G_mmuOpLib;                                /*  MMU ������                  */
/*********************************************************************************************************
  ������
*********************************************************************************************************/
LW_OBJECT_HANDLE            _G_ulVmmLock;
/*********************************************************************************************************
** ��������: API_VmmGetLibBlock
** ��������: ���ϵͳ�� LW_MMU_OP �ṹ��(���� BSP �����ʼ�� MMU ϵͳ)
** �䡡��  : NONE
** �䡡��  : &_G_mmuOpLib
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_MMU_OP  *API_VmmGetLibBlock (VOID)
{
    return  (&_G_mmuOpLib);
}
/*********************************************************************************************************
** ��������: API_VmmLibPrimaryInit
** ��������: ��ʼ�� vmm �� (ע��: LW_VMM_ZONE_DESC �ṹ���ڵ�Ԫ����һ��Ҫ�� LW_CFG_VMM_ZONE_NUM һ��)
** �䡡��  : vmzone        �����ڴ���
**           mmugdesc      ȫ��ӳ���ϵ������
**           pcMachineName �������еĻ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ģʽΪ���� MMU ��ʼ��
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmLibPrimaryInit (LW_VMM_ZONE_DESC       vmzone[],
                              LW_MMU_GLOBAL_DESC     mmugdesc[],
                              CPCHAR                 pcMachineName)
{
    static BOOL                 bIsInit  = LW_FALSE;
           PLW_MMU_VIRTUAL_DESC pvirdesc = __vmmVirtualDesc();
           INT                  i;
           ULONG                ulError;
           ULONG                ulPageNum = 0;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }

    if ((vmzone == LW_NULL) || (mmugdesc == LW_NULL)) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        ulPageNum += (ULONG)(vmzone[i].ZONED_stSize >> LW_CFG_VMM_PAGE_SHIFT);
    }
    
    ulError = __pageCbInit(ulPageNum);                                  /*  ��ʼ������ҳ����ƿ��      */
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        ulError = __vmmPhysicalCreate((ULONG)i, vmzone[i].ZONED_ulAddr, 
                                                vmzone[i].ZONED_stSize,
                                                vmzone[i].ZONED_uiAttr);/*  ��ʼ������ zone             */
        if (ulError) {
            _ErrorHandle(ulError);
            return  (ulError);
        }
        
        ulError = __areaPhysicalSpaceInit((ULONG)i, vmzone[i].ZONED_ulAddr, 
                                                    vmzone[i].ZONED_stSize);
        if (ulError) {
            _ErrorHandle(ulError);
            return  (ulError);
        }
    }
    
    ulError = __vmmVirtualCreate(pvirdesc->ulVirtualStart, 
                                 pvirdesc->stSize);                     /*  ��ʼ���߼��ռ�              */
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    ulError = __areaVirtualSpaceInit(pvirdesc->ulVirtualStart, 
                                     pvirdesc->stSize);
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    _G_ulVmmLock = API_SemaphoreMCreate("vmm_lock", LW_PRIO_DEF_CEILING, 
                                        LW_OPTION_INHERIT_PRIORITY |
                                        LW_OPTION_WAIT_PRIORITY | 
                                        LW_OPTION_DELETE_SAFE |
                                        LW_OPTION_OBJECT_GLOBAL,        /*  �������ȼ��ȴ�              */
                                        LW_NULL);
    if (!_G_ulVmmLock) {
        return  (API_GetLastError());
    }
    
    __vmmMapInit();                                                     /*  ��ʼ��ӳ������            */
    
    ulError = __vmmLibPrimaryInit(mmugdesc, pcMachineName);             /*  ��ʼ���ײ� MMU              */
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    bIsInit = LW_TRUE;
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "MMU initilaized.\r\n");

    return  (ERROR_NONE);                                               /*  ��ʼ���ײ� MMU              */
}
/*********************************************************************************************************
** ��������: API_VmmLibSecondaryInit
** ��������: ��ʼ�� vmm �� (�Ӻ� MMU ��ʼ��)
** �䡡��  : pcMachineName     �������еĻ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API  
ULONG  API_VmmLibSecondaryInit (CPCHAR  pcMachineName)
{
    ULONG    ulError;
    
    ulError = __vmmLibSecondaryInit(pcMachineName);
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "secondary MMU initilaized.\r\n");

    return  (ulError);
}
                             
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: API_VmmMmuEnable
** ��������: ���� MMU, MMU ����ǰ�������ַ�������κα仯.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmMmuEnable (VOID)
{
    KN_SMP_MB();
    __VMM_MMU_ENABLE();                                                 /*  ���� MMU                    */
    KN_SMP_MB();
}
/*********************************************************************************************************
** ��������: API_VmmMmuDisable
** ��������: ֹͣ MMU, MMU ֹͣǰ�������ַ�������κα仯.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmMmuDisable (VOID)
{
    KN_SMP_MB();
    __VMM_MMU_DISABLE();                                                /*  ֹͣ MMU                    */
    KN_SMP_MB();
}
/*********************************************************************************************************
** ��������: API_VmmPhyAlloc
** ��������: �������ڴ������������������ҳ
** �䡡��  : stSize     ��Ҫ������ڴ��С
** �䡡��  : ������ҳ�׵�ַ (�����ַ, ����ֱ��ʹ��!)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmPhyAlloc (size_t  stSize)
{
    return  (API_VmmPhyAllocAlign(stSize, LW_CFG_VMM_PAGE_SIZE, LW_ZONE_ATTR_NONE));
}
/*********************************************************************************************************
** ��������: API_VmmPhyAlloc
** ��������: �������ڴ������������������ҳ, ��չ�ӿ�.
** �䡡��  : stSize     ��Ҫ������ڴ��С
**           uiAttr     ��Ҫ���������ҳ������
** �䡡��  : ������ҳ�׵�ַ (�����ַ, ����ֱ��ʹ��!)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmPhyAllocEx (size_t  stSize, UINT  uiAttr)
{
    return  (API_VmmPhyAllocAlign(stSize, LW_CFG_VMM_PAGE_SIZE, uiAttr));
}
/*********************************************************************************************************
** ��������: API_VmmPhyAllocAlign
** ��������: �������ڴ������������������ҳ, (��������ϵ)
** �䡡��  : stSize     ��Ҫ������ڴ��С
**           stAlign    �����ϵ
**           uiAttr     ��Ҫ���������ҳ������
** �䡡��  : ������ҳ�׵�ַ (�����ַ, ����ֱ��ʹ��!)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmPhyAllocAlign (size_t  stSize, size_t  stAlign, UINT  uiAttr)
{
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
    
    REGISTER PLW_VMM_PAGE   pvmpage;
             ULONG          ulZoneIndex;

    if (stAlign & (stAlign - 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iAlign invalidate.\r\n");
        _ErrorHandle(ERROR_VMM_ALIGN);
        return  (LW_NULL);
    }
    
    if (stAlign < LW_CFG_VMM_PAGE_SIZE) {
        stAlign = LW_CFG_VMM_PAGE_SIZE;
    }
    
    if (stExcess) {
        ulPageNum++;
    }
    
    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    pvmpage = __vmmPhysicalPageAllocAlign(ulPageNum, 
                                          stAlign, uiAttr,
                                          &ulZoneIndex);                /*  ������������ҳ��            */
    if (pvmpage  == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_LOW_PHYSICAL_PAGE);
        return  (LW_NULL);
    }
    
    pvmpage->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;
    pvmpage->PAGE_ulFlags = LW_VMM_FLAG_FAIL;                           /*  ��¼��ҳ����                */
    
    __areaPhysicalInsertPage(ulZoneIndex, 
                             pvmpage->PAGE_ulPageAddr, pvmpage);        /*  ��������ռ䷴���          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_PHY_ALLOC,
                      pvmpage->PAGE_ulPageAddr, stSize, stAlign, LW_NULL);
    
    return  ((PVOID)pvmpage->PAGE_ulPageAddr);                          /*  ֱ�ӷ��������ڴ��ַ        */
}
/*********************************************************************************************************
** ��������: API_VmmPhyFree
** ��������: �ͷ����������ҳ
** �䡡��  : pvPhyMem     ������ҳ�׵�ַ (�����ַ)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmPhyFree (PVOID  pvPhyMem)
{
    REGISTER PLW_VMM_PAGE   pvmpage;
             addr_t         ulAddr = (addr_t)pvPhyMem;
             ULONG          ulZoneIndex;

    __VMM_LOCK();
    ulZoneIndex = __vmmPhysicalGetZone(ulAddr);
    if (ulZoneIndex >= LW_CFG_VMM_ZONE_NUM) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_PHYSICAL_PAGE);                          /*  �޷�Ѱ�ҵ�ָ��������ҳ��    */
        return;
    }
    
    pvmpage = __areaPhysicalSearchPage(ulZoneIndex, ulAddr);
    if (pvmpage  == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_PHYSICAL_PAGE);                          /*  �޷������ѯ����ҳ����ƿ�  */
        return;
    }
    
    __areaPhysicalUnlinkPage(ulZoneIndex, 
                             pvmpage->PAGE_ulPageAddr, 
                             pvmpage);                                  /*  �����ַ������ͷ�          */
    
    __vmmPhysicalPageFree(pvmpage);                                     /*  �ͷ�����ҳ��                */
    __VMM_UNLOCK();

    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_PHY_FREE,
                      pvPhyMem, LW_NULL);
}
/*********************************************************************************************************
** ��������: API_VmmDmaAlloc
** ��������: �������ڴ������������������ҳ, ��Ҫ���� DMA ���������ڴ����.
** �䡡��  : stSize     ��Ҫ������ڴ��С
** �䡡��  : ������ҳ�׵�ַ (�����ַ, �˵�ַ��Ӧ�������ַ������ LW_ZONE_ATTR_DMA ������)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmDmaAlloc (size_t  stSize)
{
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);

    REGISTER PLW_VMM_PAGE   pvmpage;
             ULONG          ulZoneIndex;
             ULONG          ulError;
    
    if (stExcess) {
        ulPageNum++;
    }
    
    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    pvmpage = __vmmPhysicalPageAlloc(ulPageNum, LW_ZONE_ATTR_DMA, 
                                     &ulZoneIndex);                     /*  ������������ҳ��            */
    if (pvmpage  == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_LOW_PHYSICAL_PAGE);
        return  (LW_NULL);
    }
    
    ulError = __vmmLibPageMap(pvmpage->PAGE_ulPageAddr,                 /*  ��ʹ�� CACHE                */
                              pvmpage->PAGE_ulPageAddr,
                              ulPageNum, 
                              LW_VMM_FLAG_DMA);                         /*  ӳ���߼�����ͬһ��ַ        */
    if (ulError) {                                                      /*  ӳ�����                    */
        __vmmPhysicalPageFree(pvmpage);                                 /*  �ͷ�����ҳ��                */
        __VMM_UNLOCK();
        _ErrorHandle(ulError);
        return  (LW_NULL);
    }
    
    pvmpage->PAGE_ulMapPageAddr = pvmpage->PAGE_ulPageAddr;             /*  ��¼ӳ���ϵ                */
    pvmpage->PAGE_ulFlags = LW_VMM_FLAG_DMA;                            /*  ��¼��ҳ����                */
    
    __areaPhysicalInsertPage(ulZoneIndex, 
                             pvmpage->PAGE_ulPageAddr, pvmpage);        /*  ��������ռ䷴���          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_DMA_ALLOC,
                      pvmpage->PAGE_ulPageAddr, stSize, LW_CFG_VMM_PAGE_SIZE, LW_NULL);
    
    return  ((PVOID)pvmpage->PAGE_ulPageAddr);                          /*  ֱ�ӷ��������ڴ��ַ        */
}
/*********************************************************************************************************
** ��������: API_VmmDmaAllocAlign
** ��������: �������ڴ������������������ҳ, ��Ҫ���� DMA ���������ڴ����.(��ָ���ڴ��ϵ)
** �䡡��  : stSize     ��Ҫ������ڴ��С
**           stAlign    �����ϵ
** �䡡��  : ������ҳ�׵�ַ (�����ַ, �˵�ַ��Ӧ�������ַ������ LW_ZONE_ATTR_DMA ������)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_VmmDmaAllocAlign (size_t  stSize, size_t  stAlign)
{
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
    
    REGISTER PLW_VMM_PAGE   pvmpage;
             ULONG          ulZoneIndex;
             ULONG          ulError;

    if (stAlign & (stAlign - 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iAlign invalidate.\r\n");
        _ErrorHandle(ERROR_VMM_ALIGN);
        return  (LW_NULL);
    }
    if (stAlign < LW_CFG_VMM_PAGE_SIZE) {
        stAlign = LW_CFG_VMM_PAGE_SIZE;
    }
    
    if (stExcess) {
        ulPageNum++;
    }
    
    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    pvmpage = __vmmPhysicalPageAllocAlign(ulPageNum, 
                                          stAlign, LW_ZONE_ATTR_DMA,
                                          &ulZoneIndex);                /*  ������������ҳ��            */
    if (pvmpage  == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_LOW_PHYSICAL_PAGE);
        return  (LW_NULL);
    }
    
    ulError = __vmmLibPageMap(pvmpage->PAGE_ulPageAddr,                 /*  ��ʹ�� CACHE                */
                              pvmpage->PAGE_ulPageAddr,
                              ulPageNum, 
                              LW_VMM_FLAG_DMA);                         /*  ӳ���߼�����ͬһ��ַ        */
    if (ulError) {                                                      /*  ӳ�����                    */
        __vmmPhysicalPageFree(pvmpage);                                 /*  �ͷ�����ҳ��                */
        __VMM_UNLOCK();
        _ErrorHandle(ulError);
        return  (LW_NULL);
    }
    
    pvmpage->PAGE_ulMapPageAddr = pvmpage->PAGE_ulPageAddr;
    pvmpage->PAGE_ulFlags = LW_VMM_FLAG_DMA;                            /*  ��¼��ҳ����                */
    
    __areaPhysicalInsertPage(ulZoneIndex, 
                             pvmpage->PAGE_ulPageAddr, pvmpage);        /*  ��������ռ䷴���          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_DMA_ALLOC,
                      pvmpage->PAGE_ulPageAddr, stSize, stAlign, LW_NULL);
                      
    return  ((PVOID)pvmpage->PAGE_ulPageAddr);                          /*  ֱ�ӷ��������ڴ��ַ        */
}
/*********************************************************************************************************
** ��������: API_VmmDmaFree
** ��������: �ͷ����������ҳ
** �䡡��  : pvDmaMem     ������ҳ�׵�ַ (�����ַ)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmDmaFree (PVOID  pvDmaMem)
{
    REGISTER PLW_VMM_PAGE   pvmpage;
             addr_t         ulAddr = (addr_t)pvDmaMem;
             ULONG          ulZoneIndex;

    __VMM_LOCK();
    ulZoneIndex = __vmmPhysicalGetZone(ulAddr);
    if (ulZoneIndex >= LW_CFG_VMM_ZONE_NUM) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_PHYSICAL_PAGE);                          /*  �޷�Ѱ�ҵ�ָ��������ҳ��    */
        return;
    }
    
    pvmpage = __areaPhysicalSearchPage(ulZoneIndex, ulAddr);
    if (pvmpage  == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_PHYSICAL_PAGE);                          /*  �޷������ѯ����ҳ����ƿ�  */
        return;
    }

#if LW_CFG_CACHE_EN > 0
    API_CacheClearPage(DATA_CACHE, 
                       (PVOID)pvmpage->PAGE_ulPageAddr,
                       (PVOID)pvmpage->PAGE_ulPageAddr,
                       (size_t)(pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibPageMap(pvmpage->PAGE_ulPageAddr,
                    pvmpage->PAGE_ulPageAddr,
                    pvmpage->PAGE_ulCount, 
                    LW_VMM_FLAG_FAIL);                                  /*  ���������                  */
    
    __areaPhysicalUnlinkPage(ulZoneIndex, 
                             pvmpage->PAGE_ulPageAddr, 
                             pvmpage);                                  /*  �����ַ������ͷ�          */
    
    __vmmPhysicalPageFree(pvmpage);                                     /*  �ͷ�����ҳ��                */
    __VMM_UNLOCK();

    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_DMA_FREE,
                      pvDmaMem, LW_NULL);
}
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
PVOID  API_VmmIoRemapEx (PVOID  pvPhysicalAddr, 
                         size_t stSize,
                         ULONG  ulFlags)
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
    pvmpageVirtual = __vmmVirtualPageAlloc(ulPageNum);                  /*  ������������ҳ��            */
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
        __vmmVirtualPageFree(pvmpageVirtual);                           /*  �ͷ������ַ�ռ�            */
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
PVOID  API_VmmIoRemap (PVOID  pvPhysicalAddr, 
                       size_t stSize)
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
    
    __vmmVirtualPageFree(pvmpageVirtual);                               /*  ɾ������ҳ��                */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_IOUNMAP,
                      pvVirtualAddr, LW_NULL);
}
/*********************************************************************************************************
** ��������: API_VmmIoRemapNocache
** ��������: ������ IO �ռ�ָ���ڴ�ӳ�䵽�߼��ռ�. (�� CACHE)
** �䡡��  : pvPhysicalAddr     �����ڴ��ַ
**           ulSize             ��Ҫӳ����ڴ��С
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
/*********************************************************************************************************
** ��������: API_VmmMap
** ��������: ��ָ������ռ�ӳ�䵽ָ�����߼��ռ�
** �䡡��  : pvVirtualAddr      ��Ҫӳ��������ַ
**           pvPhysicalAddr     �����ڴ��ַ
**           stSize             ��Ҫӳ����ڴ��С
**           ulFlag             �ڴ���������: LW_VMM_FLAG_UNWRITABLE, LW_VMM_FLAG_CACHEABLE ...
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : pvVirtualAddr ���ܳ����� BSP ���õ������ַ�ռ���, ������Ӱ���ں� VMM �����������.
             �����ô˺���.
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmMap (PVOID  pvVirtualAddr,
                   PVOID  pvPhysicalAddr,
                   size_t stSize, 
                   ULONG  ulFlag)
{
    REGISTER addr_t ulVirtualAddr  = (addr_t)pvVirtualAddr;
    REGISTER addr_t ulPhysicalAddr = (addr_t)pvPhysicalAddr;
    
    REGISTER ULONG  ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
             
    REGISTER ULONG  ulError;
    
    if (stExcess) {
        ulPageNum++;                                                    /*  ȷ����ҳ����                */
    }

    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (ERROR_VMM_VIRTUAL_ADDR);
    }
    
    if (!ALIGNED(pvVirtualAddr,  LW_CFG_VMM_PAGE_SIZE) ||
        !ALIGNED(pvPhysicalAddr, LW_CFG_VMM_PAGE_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_VMM_ALIGN);
    }
    
    __VMM_LOCK();
    ulError = __vmmLibPageMap(ulPhysicalAddr, ulVirtualAddr, ulPageNum, ulFlag);
    __VMM_UNLOCK();
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_VmmSetFlag
** ��������: ����ָ���߼���ַ�ռ���ڴ��������. ����Ϊ����ռ��������ĵ�ַ.
** �䡡��  : pvVirtualAddr      �����ַ
**           ulFlag             �ڴ���������: LW_VMM_FLAG_UNWRITABLE, LW_VMM_FLAG_CACHEABLE ...
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmSetFlag (PVOID  pvVirtualAddr, ULONG  ulFlag)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             addr_t         ulVirtualAddr = (addr_t)pvVirtualAddr;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulVirtualAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  �޷������ѯ����ҳ����ƿ�  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
#if LW_CFG_CACHE_EN > 0
    __vmmPhysicalPageClearAll(pvmpageVirtual);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibSetFlag(ulVirtualAddr, 
                    pvmpageVirtual->PAGE_ulCount, ulFlag);              /*  ����ӳ����Щҳ��            */
    
    pvmpageVirtual->PAGE_ulFlags = ulFlag;                              /*  ��¼��Ȩ����Ϣ              */
    
    __vmmPhysicalPageSetFlagAll(pvmpageVirtual, ulFlag);                /*  ������������ҳ��� flag     */
    
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_SETFLAG,
                      pvVirtualAddr, ulFlag, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmGetFlag
** ��������: ��ȡָ���߼���ַ���ڴ��������.
** �䡡��  : pvVirtualAddr      �����ַ
**           pulFlag            ��ȡ���ڴ���������: LW_VMM_FLAG_UNWRITABLE, LW_VMM_FLAG_CACHEABLE ...
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmGetFlag (PVOID  pvVirtualAddr, 
                       ULONG *pulFlag)
{
    REGISTER ULONG  ulError;

    if (pulFlag == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __VMM_LOCK();
    ulError = __vmmLibGetFlag((addr_t)pvVirtualAddr, pulFlag);
    __VMM_UNLOCK();
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_VmmVirtualToPhysical
** ��������: ͨ�������ַ��ѯ��Ӧ�������ַ (vmm dma alloc ����Ҫͨ���˺�������ȡ��ʵ�������ַ)
** �䡡��  : ulVirtualAddr      �����ַ
**           pulPhysicalAddr    ���ص������ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmVirtualToPhysical (addr_t  ulVirtualAddr, addr_t  *pulPhysicalAddr)
{
    REGISTER ULONG  ulError;

    __VMM_LOCK();
    ulError = __vmmLibVirtualToPhysical(ulVirtualAddr, pulPhysicalAddr);
    __VMM_UNLOCK();
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_VmmPhysicalToVirtual
** ��������: ͨ�������ַ��ѯ�����ַ
** �䡡��  : ulPhysicalAddr      �����ַ
**           pulVirtualAddr      ���ص������ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmPhysicalToVirtual (addr_t  ulPhysicalAddr, addr_t  *pulVirtualAddr)
{
    _ErrorHandle(ENOSYS);
    return  (ENOSYS);
}
/*********************************************************************************************************
** ��������: API_VmmVirtualIsInside
** ��������: ��ѯָ���ĵ�ַ�Ƿ�������ռ���
** �䡡��  : ulAddr             ��ַ
** �䡡��  : �Ƿ��������ַ�ռ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
BOOL  API_VmmVirtualIsInside (addr_t  ulAddr)
{
    return  (__vmmVirtualIsInside(ulAddr));                             /*  ����Ҫʹ�� VMM LOCK         */
}
/*********************************************************************************************************
** ��������: API_VmmZoneStatus
** ��������: ��� zone �����
** �䡡��  : ulZoneIndex        ������������
**           pulPhysicalAddr    �����ַ
**           pstSize            ��С
**           pulPgd             PDG �������
**           pulFreePage        ����ҳ��ĸ���
**           puiAttr            ��������   ����: LW_ZONE_ATTR_DMA
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmZoneStatus (ULONG     ulZoneIndex,
                          addr_t   *pulPhysicalAddr,
                          size_t   *pstSize,
                          addr_t   *pulPgd,
                          ULONG    *pulFreePage,
                          BOOL     *puiAttr)
{
    PLW_MMU_CONTEXT    pmmuctx = __vmmGetCurCtx();
    
    if (ulZoneIndex >= LW_CFG_VMM_ZONE_NUM) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __VMM_LOCK();
    if (pulPhysicalAddr) {
        *pulPhysicalAddr = _G_vmzonePhysical[ulZoneIndex].ZONE_ulAddr;
    }
    if (pstSize) {
        *pstSize = _G_vmzonePhysical[ulZoneIndex].ZONE_stSize;
    }
    if (pulPgd) {
        *pulPgd = (addr_t)pmmuctx->MMUCTX_pgdEntry;
    }
    if (pulFreePage) {
        *pulFreePage = _G_vmzonePhysical[ulZoneIndex].ZONE_ulFreePage;
    }
    if (puiAttr) {
        *puiAttr = _G_vmzonePhysical[ulZoneIndex].ZONE_uiAttr;
    }
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VmmVirtualStatus
** ��������: ��� virtual �����
** �䡡��  : pulVirtualAddr     �����ַ
**           pstSize            ��С
**           pulFreePage        ҳ��ĸ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_VmmVirtualStatus (addr_t   *pulVirtualAddr,
                             size_t   *pstSize,
                             ULONG    *pulFreePage)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
    
    __VMM_LOCK();
    if (pulVirtualAddr) {
        *pulVirtualAddr = pmmuctx->MMUCTX_vmzoneVirtual.ZONE_ulAddr;
    }
    if (pstSize) {
        *pstSize = pmmuctx->MMUCTX_vmzoneVirtual.ZONE_stSize;
    }
    if (pulFreePage) {
        *pulFreePage = pmmuctx->MMUCTX_vmzoneVirtual.ZONE_ulFreePage;
    }
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: getpagesize
** ��������: ��� pagesize 
** �䡡��  : NONE
** �䡡��  : pagesize
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int getpagesize (void)
{
    return  (LW_CFG_VMM_PAGE_SIZE);
}
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
