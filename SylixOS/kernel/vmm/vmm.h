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
** ��   ��   ��: vmm.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����.

** BUG:
2009.11.10  LW_VMM_ZONEDESC �м��� DMA �����жϷ�, �����������Ƿ�ɹ� DMA ʹ��.
            DMA �ڴ����, ����ֵΪ�����ַ.
2011.03.18  �� LW_VMM_ZONEDESC ����Ϊ LW_VMM_ZONE_DESC.
*********************************************************************************************************/

#ifndef __VMM_H
#define __VMM_H

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  zone describer
  
  ע��:
  
  ��д BSP ʱ��Ҫ�����ڴ����, SylixOS �����ڴ����һ��Ϊ���¸�ʽ:
  
  |<-- ���������ַ��ͬ -->|
  |                        |
  +-----------+------------+--------------------------------+
  |  KERN MEM |  DMA ZONE  |          COMMON VMM            |
  |           |            |                                |
  |  �ں��ڴ� |  ATTR_DMA  |          ATTR_NONE             |
  +-----------+------------+--------------------------------+
              |                                             |
              |<----------- VMM ��������� ---------------->|
  
  ����ֻ�Ǿ���, �ڴ��˳��û��Ҫ��, ����, �ں��ڴ���Բ���Ҫ����ǰ��, VMM ���������Ҳ���Բ��������.
  ����, SylixOS �� BSP ������Ҫ��: "�ں��ڴ�"�;���"ATTR_DMA"���Ե� DMA �ڴ��ַ, ������"����ռ�"���ص�!
  ������˵��"����ռ�"���� LW_MMU_VIRTUAL_DESC ��������οռ�, Ĭ�� 3GB~4GB ֮��, ����ͨ���ں����������޸�
  ����, �ں��ڴ��� ATTR_DMA �����ַ�������ַ��ʼ������ͬ. 
  (��Ϊ�ܶ������̶�ʹ�� sys_malloc() ���� DMA �ڴ�, ͨ�� cacheFlush �� cacheInvalidate �����汣��һ��, 
   ������Щ�ڴ��㷨ʹ�������ַ����, �����ں��ڴ��� ATTR_DMA �����ַ�������ַ��ʼ������ͬ)
  
  "ATTR_NONE"���Ժ�����ռ��ַ���ص���, ��Ϊ vmmDmaAlloc �������ڴ��ַ�������ַ, �������������ַҲ��
  ��ͬ��. Ҳ����˵: �ں��ڴ� �� ATTR_DMA �ڴ������ƽ��ӳ��, ���ҵ�ַ����������ռ��ص�!
  
  COMMON VMM �������ַ�ǿ���������ռ��ص���, ��Ϊ��������ַ������ֱ�ӷ���, �����������Ҫ���ʵ�
  �Ĵ����������ַ�ռ��е�ַ�ص�, �����ʹ�� vmmIoRemap, vmmIoUnmap ��ӳ�������ڴ�, Ȼ����ܲ���, Ҳ����˵
  ����������ռ��е�ַ�ص����ڴ涼��Ҫͨ�� vmm ���������ṩ����غ������ܷ���, (�����ȷ�����Ӧ�������ַ)
  ����ֱ��ʹ�������ַ����.
  
  һ��� BSP �г��� COMMON VMM �������ⶼ��С, COMMON VMM ��ͨ�� VMM �ڴ����˼, �������м��صĳ������
  ģ�鶼������������ڴ������е�, ��������ʵ��ֻ�ܿ��� VMM ��������ǵ�����ռ�!
  SylixOS �ṩ����ռ�һ�����ʵ������ռ��ܶ�, һ���� 32 λ�����Ͽ��Դﵽ 3GB.
*********************************************************************************************************/
#define LW_ZONE_ATTR_NONE    0x0000                                     /*  ����������                  */
#define LW_ZONE_ATTR_DMA     0x0001                                     /*  ��������Ա� DMA ʹ��       */

typedef struct __lw_vmm_zone_desc {
    addr_t                   ZONED_ulAddr;                              /*  ��ʼ��ַ                    */
    size_t                   ZONED_stSize;                              /*  ���򳤶�                    */
    UINT                     ZONED_uiAttr;                              /*  ��������                    */
} LW_VMM_ZONE_DESC;
typedef LW_VMM_ZONE_DESC    *PLW_VMM_ZONE_DESC;

/*********************************************************************************************************
  mmu ȫ�ֳ�ʼ��ӳ���ϵ�ڵ�
*********************************************************************************************************/

typedef struct __lw_mmu_global_desc {
    addr_t                   ulVirtualAddr;                             /*  �����ַ                    */
    addr_t                   ulPhysicalAddr;                            /*  �����ַ                    */
    size_t                   stSize;                                    /*  ����                        */
    ULONG                    ulFlag;                                    /*  ��־                        */
} LW_MMU_GLOBAL_DESC;
typedef LW_MMU_GLOBAL_DESC  *PLW_MMU_GLOBAL_DESC;

/*********************************************************************************************************
  mmu �����ַ�ռ�
*********************************************************************************************************/

typedef struct __lw_mmu_virtual_desc {
    addr_t                   ulVirtualSwitch;                           /*  �ڲ�ҳ�潻���ռ��ַ(�Զ�)  */
    addr_t                   ulVirtualStart;                            /*  ����ռ���ʼ��ַ            */
    size_t                   stSize;                                    /*  ����ռ䳤��                */
} LW_MMU_VIRTUAL_DESC;
typedef LW_MMU_VIRTUAL_DESC *PLW_MMU_VIRTUAL_DESC;

/*********************************************************************************************************
  vmm ��ǰ״̬
*********************************************************************************************************/

typedef struct __lw_vmm_status {
    INT64                    VMMS_i64AbortCounter;                      /*  �쳣��ֹ����                */
    INT64                    VMMS_i64PageFailCounter;                   /*  ȱҳ�ж������������        */
    INT64                    VMMS_i64PageLackCounter;                   /*  ϵͳȱ������ҳ�����        */
    INT64                    VMMS_i64MapErrCounter;                     /*  ӳ��������                */
    INT64                    VMMS_i64SwpCounter;                        /*  ��������                    */
    INT64                    VMMS_i64Reseve[8];
} LW_VMM_STATUS;
typedef LW_VMM_STATUS       *PLW_VMM_STATUS;

/*********************************************************************************************************
  VMM ��ʼ��, ֻ�ܷ��� API_KernelStart �ص���
  
  ��Ϊ SMP ϵͳʱ, API_KernelPrimaryStart    ��Ӧ�����ص����� API_VmmLibPrimaryInit
                   API_KernelSecondaryStart  ��Ӧ�����ص����� API_VmmLibSecondaryInit
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API ULONG        API_VmmLibPrimaryInit(LW_VMM_ZONE_DESC      vmzone[],
                                          LW_MMU_GLOBAL_DESC    mmugdesc[],
                                          CPCHAR                pcMachineName);
                                                                        /*  ��ʼ�� VMM �������������   */
#define API_VmmLibInit      API_VmmLibPrimaryInit

#if LW_CFG_SMP_EN > 0
LW_API ULONG        API_VmmLibSecondaryInit(CPCHAR  pcMachineName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  MMU ������ֹͣ
*********************************************************************************************************/

LW_API VOID         API_VmmMmuEnable(VOID);                             /*  MMU ����                    */

LW_API VOID         API_VmmMmuDisable(VOID);                            /*  MMU ֹͣ                    */

/*********************************************************************************************************
  VMM API (���·��亯�����Է����ȷ����, �ɹ�ֱ�ӷ��ʵ��ڴ�ռ�)
*********************************************************************************************************/

LW_API PVOID        API_VmmMalloc(size_t stSize);                       /*  �����߼������ڴ�, �����ַ  */
LW_API PVOID        API_VmmMallocEx(size_t stSize, ULONG ulFlag);       /*  �����߼������ڴ�, �����ַ  */
LW_API PVOID        API_VmmMallocAlign(size_t stSize, 
                                       size_t stAlign, 
                                       ULONG  ulFlag);                  /*  �����߼������ڴ�, �����ַ  */
LW_API VOID         API_VmmFree(PVOID  pvVirtualMem);                   /*  �������������ڴ�            */

LW_API ULONG        API_VmmVirtualToPhysical(addr_t  ulVirtualAddr, 
                                             addr_t *pulPhysicalAddr);  /*  ͨ�������ַ��ȡ�����ַ    */
                                             
LW_API BOOL         API_VmmVirtualIsInside(addr_t  ulAddr);             /*  ָ����ַ�Ƿ��ڹ��������ռ�*/
                                             
LW_API ULONG        API_VmmZoneStatus(ULONG     ulZoneIndex,
                                      addr_t   *pulPhysicalAddr,
                                      size_t   *pstSize,
                                      addr_t   *pulPgd,
                                      ULONG    *pulFreePage,
                                      BOOL     *puiAttr);               /*  ��������������Ϣ          */
                                      
LW_API ULONG        API_VmmVirtualStatus(addr_t  *pulVirtualAddr,
                                         size_t  *pulSize,
                                         ULONG   *pulFreePage);         /*  �������ռ���Ϣ            */
                                         
/*********************************************************************************************************
  VMM ��չ����
  
  ����������ռ�, �����ֵ�һ�η���ʱ, ��ͨ��ȱҳ�жϷ��������ڴ�, ��ȱҳ�ж����޷���������ҳ��ʱ, ���յ�
  SIGSEGV �źŲ������߳�. 
  
  API_VmmRemapArea() ������������ʹ��, ���������� linux remap_pfn_range() ����.
*********************************************************************************************************/

LW_API PVOID        API_VmmMallocArea(size_t stSize, FUNCPTR  pfuncFiller, PVOID  pvArg);
                                                                        /*  �����߼������ڴ�, �����ַ  */
                                                                        
LW_API PVOID        API_VmmMallocAreaEx(size_t stSize, FUNCPTR  pfuncFiller, PVOID  pvArg, 
                                        INT  iFlags, ULONG ulFlag);     /*  �����߼������ڴ�, �����ַ  */
                                                                        
LW_API PVOID        API_VmmMallocAreaAlign(size_t stSize, size_t stAlign, 
                                           FUNCPTR  pfuncFiller, PVOID  pvArg, 
                                           INT  iFlags, ULONG  ulFlag); /*  �����߼������ڴ�, �����ַ  */
                                                                        
LW_API VOID         API_VmmFreeArea(PVOID  pvVirtualMem);               /*  �������������ڴ�            */

LW_API ULONG        API_VmmExpandArea(PVOID  pvVirtualMem, size_t  stExpSize);
                                                                        /*  ��չ�����ڴ����            */
LW_API PVOID        API_VmmSplitArea(PVOID  pvVirtualMem, size_t  stSize);
                                                                        /*  ��������ڴ����            */
LW_API ULONG        API_VmmMergeArea(PVOID  pvVirtualMem1, PVOID  pvVirtualMem2);
                                                                        /*  �ϲ������ڴ����            */
LW_API ULONG        API_VmmMoveArea(PVOID  pvVirtualTo, PVOID  pvVirtualFrom);
                                                                        /*  �ƶ������ڴ����            */
LW_API ULONG        API_VmmPCountInArea(PVOID  pvVirtualMem, ULONG  *pulPageNum);
                                                                        /*  ͳ��ȱҳ�жϷ�����ڴ�ҳ��  */

LW_API ULONG        API_VmmRemapArea(PVOID  pvVirtualAddr, PVOID  pvPhysicalAddr, 
                                     size_t stSize, ULONG  ulFlag,
                                     FUNCPTR pfuncFiller, PVOID  pvArg);/*  �����µ�ӳ���ϵ            */
                                                                        /*  ������������ mmap ʹ�ô˺���*/
LW_API ULONG        API_VmmInvalidateArea(PVOID  pvVirtualMem, 
                                          PVOID  pvSubMem, 
                                          size_t stSize);               /*  �ͷ������ڴ�, ��������ռ�  */
                                          
LW_API VOID         API_VmmAbortStatus(PLW_VMM_STATUS  pvmms);          /*  ��÷�����ֹͳ����Ϣ        */
       
/*********************************************************************************************************
  VMM ���� loader ���������ں�ģ���ṩ�Ĺ����֧�� (���� loader ������ SylixOS �ں˷����Լ�ʹ��)
*********************************************************************************************************/

LW_API ULONG        API_VmmSetFiller(PVOID  pvVirtualMem, FUNCPTR  pfuncFiller, PVOID  pvArg);
                                                                        /*  ������亯��                */
LW_API ULONG        API_VmmSetFindShare(PVOID  pvVirtualMem, PVOIDFUNCPTR  pfuncFindShare, PVOID  pvArg);
                                                                        /*  ���ò�ѯ������            */
LW_API ULONG        API_VmmPreallocArea(PVOID       pvVirtualMem, 
                                        PVOID       pvSubMem, 
                                        size_t      stSize, 
                                        FUNCPTR     pfuncTempFiller, 
                                        PVOID       pvTempArg,
                                        ULONG       ulFlag);            /*  Ԥ���������ڴ�ҳ��          */
                                        
LW_API ULONG        API_VmmShareArea(PVOID      pvVirtualMem1, 
                                     PVOID      pvVirtualMem2,
                                     size_t     stStartOft1, 
                                     size_t     stStartOft2, 
                                     size_t     stSize,
                                     BOOL       bExecEn,
                                     FUNCPTR    pfuncTempFiller, 
                                     PVOID      pvTempArg);             /*  ���ù�������                */

/*********************************************************************************************************
  �ں�ר�� API �û����� 
  
  �������޸������ڴ����������ҳ��, �޸Ĵ�С�ѷ���ʱ�Ĵ�СΪ��׼
*********************************************************************************************************/

LW_API ULONG        API_VmmSetFlag(PVOID  pvVirtualAddr, 
                                   ULONG  ulFlag);                      /*  ���������ַȨ��            */
                                   
LW_API ULONG        API_VmmGetFlag(PVOID  pvVirtualAddr, 
                                   ULONG *pulFlag);                     /*  ��ȡ�����ַȨ��            */
                                   
/*********************************************************************************************************
  ���� API ֻ������������ڴ�, ��û�в���ӳ���ϵ. ����ֱ��ʹ��, ����ͨ�������ڴ�ӳ�����ʹ��.
*********************************************************************************************************/

LW_API PVOID        API_VmmPhyAlloc(size_t stSize);                     /*  ���������ڴ�                */
LW_API PVOID        API_VmmPhyAllocEx(size_t  stSize, UINT  uiAttr);    /*  ������ͬ, ������ָ���ڴ�����*/
LW_API PVOID        API_VmmPhyAllocAlign(size_t stSize, 
                                         size_t stAlign,
                                         UINT   uiAttr);                /*  ���������ڴ�, ָ�������ϵ  */
LW_API VOID         API_VmmPhyFree(PVOID  pvPhyMem);                    /*  �ͷ������ڴ�                */

/*********************************************************************************************************
  ���� API ֻ������������ʹ��
  
  no cache ������� (dma �������ص���ֱ�ӵ������ַ, ��ƽ��ӳ��, ������ LW_ZONE_ATTR_DMA ����)
*********************************************************************************************************/

LW_API PVOID        API_VmmDmaAlloc(size_t  stSize);                    /*  �������������ڴ�, �����ַ  */
LW_API PVOID        API_VmmDmaAllocAlign(size_t stSize, size_t stAlign);/*  ������ͬ, ������ָ�������ϵ*/
LW_API VOID         API_VmmDmaFree(PVOID  pvDmaMem);                    /*  ���� DMA �ڴ滺����         */

/*********************************************************************************************************
  ���� API ֻ������������ʹ��
  
  �����������Ĵ�����ַ�����ϵͳ�����ַ�ռ����ص�, �����Ĵ���ʱ, һ��Ҫʹ�� ioremap �� iounmap ����
  
  �������ܱ�֤��������ȷ���밲ȫ��.
*********************************************************************************************************/

LW_API PVOID        API_VmmIoRemap(PVOID  pvPhysicalAddr, 
                                   size_t stSize);                      /*  ������ IO ӳ�䵽����ռ�    */
LW_API PVOID        API_VmmIoRemapEx(PVOID  pvPhysicalAddr, 
                                     size_t stSize,
                                     ULONG  ulFlags);
LW_API VOID         API_VmmIoUnmap(PVOID  pvVirtualAddr);               /*  �ͷ� IO ӳ������ռ�        */
LW_API PVOID        API_VmmIoRemapNocache(PVOID  pvPhysicalAddr, 
                                          size_t stSize);
LW_API ULONG        API_VmmMap(PVOID  pvVirtualAddr, 
                               PVOID  pvPhysicalAddr, 
                               size_t stSize, 
                               ULONG  ulFlag);                          /*  ָ���ڴ�ӳ�� (���ô˺���)   */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#else

#ifdef __SYLIXOS_KERNEL
static LW_INLINE ULONG        API_VmmVirtualToPhysical(addr_t  ulVirtualAddr, 
                                                       addr_t *pulPhysicalAddr)
{
    if (pulPhysicalAddr) {
        *pulPhysicalAddr = ulVirtualAddr;
    }
    return  (ERROR_NONE);
}
static LW_INLINE PVOID        API_VmmIoRemap(PVOID  pvPhysicalAddr, 
                                             size_t stSize)
{
    return  (pvPhysicalAddr);
}
static LW_INLINE PVOID        API_VmmIoRemapEx(PVOID  pvPhysicalAddr, 
                                               size_t stSize,
                                               ULONG  ulFlags)
{
    return  (pvPhysicalAddr);
}
static LW_INLINE VOID         API_VmmIoUnmap(PVOID  pvVirtualAddr)
{
}
static LW_INLINE PVOID        API_VmmIoRemapNocache(PVOID  pvPhysicalAddr, 
                                                    size_t stSize)
{
    return  (pvPhysicalAddr);
}
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  API_VmmAbortIsr() Ϊ�쳣������, ֻ���� ARCH ����ʹ��. 
  û��ʹ�� VMM SylixOS ��Ȼ���Դ����쳣���
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct __lw_vmm_abort {

#define LW_VMM_ABORT_TYPE_NOINFO            0                           /*  �ڲ�ʹ��                    */
#define LW_VMM_ABORT_TYPE_TERMINAL          1                           /*  ��ϵ�ṹ������ش���        */
#define LW_VMM_ABORT_TYPE_MAP               2                           /*  ҳ��ӳ����� (MMU ����)     */
                                                                        /*  ��Ҫ uiMethod ��Ƿ�������  */
#define LW_VMM_ABORT_TYPE_PERM              3                           /*  ����Ȩ�޴��� (MMU ����)     */
                                                                        /*  ��Ҫ uiMethod ��Ƿ�������  */
#define LW_VMM_ABORT_TYPE_FPE               4                           /*  �����������쳣              */
#define LW_VMM_ABORT_TYPE_BUS               5                           /*  ���߷����쳣                */
#define LW_VMM_ABORT_TYPE_BREAK             6                           /*  �ϵ��쳣                    */
#define LW_VMM_ABORT_TYPE_SYS               7                           /*  ϵͳ�����쳣                */
#define LW_VMM_ABORT_TYPE_UNDEF             8                           /*  δ����ָ��, ������ SIGILL   */

    UINT32               VMABT_uiType;
    
#define LW_VMM_ABORT_METHOD_READ            1                           /*  ������                      */
#define LW_VMM_ABORT_METHOD_WRITE           2                           /*  д����                      */
#define LW_VMM_ABORT_METHOD_EXEC            3                           /*  ִ�з���                    */
    
    UINT32               VMABT_uiMethod;
} LW_VMM_ABORT;
typedef LW_VMM_ABORT    *PLW_VMM_ABORT;

LW_API VOID         API_VmmAbortIsr(addr_t          ulRetAddr,
                                    addr_t          ulAbortAddr, 
                                    PLW_VMM_ABORT   pabtInfo,
                                    PLW_CLASS_TCB   ptcb);              /*  �쳣�жϷ�����            */

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  vmm api macro
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

#define vmmMalloc               API_VmmMalloc
#define vmmMallocEx             API_VmmMallocEx
#define vmmMallocAlign          API_VmmMallocAlign
#define vmmFree                 API_VmmFree

#define vmmMallocArea           API_VmmMallocArea
#define vmmMallocAreaEx         API_VmmMallocAreaEx
#define vmmMallocAreaAlign      API_VmmMallocAreaAlign
#define vmmFreeArea             API_VmmFreeArea
#define vmmPCountInArea         API_VmmPCountInArea
#define vmmRemapArea            API_VmmRemapArea
#define vmmInvalidateArea       API_VmmInvalidateArea
#define vmmAbortStatus          API_VmmAbortStatus

#define vmmPhyAlloc             API_VmmPhyAlloc
#define vmmPhyAllocEx           API_VmmPhyAllocEx
#define vmmPhyAllocAlign        API_VmmPhyAllocAlign
#define vmmPhyFree              API_VmmPhyFree

#define vmmDmaAlloc             API_VmmDmaAlloc                         /*  ����ֵΪ �����ַ           */
#define vmmDmaAllocAlign        API_VmmDmaAllocAlign                    /*  ����ֵΪ �����ַ           */
#define vmmDmaFree              API_VmmDmaFree

#define vmmIoRemap              API_VmmIoRemap
#define vmmIoRemapEx            API_VmmIoRemapEx
#define vmmIoUnmap              API_VmmIoUnmap
#define vmmIoRemapNocache       API_VmmIoRemapNocache

#define vmmMap                  API_VmmMap
#define vmmVirtualToPhysical    API_VmmVirtualToPhysical
#define vmmPhysicalToVirtual    API_VmmPhysicalToVirtual                /*  ��֧��VMM����������ڴ��ѯ */
#define vmmVirtualIsInside      API_VmmVirtualIsInside

#define vmmSetFlag              API_VmmSetFlag
#define vmmGetFlag              API_VmmGetFlag

#define vmmZoneStatus           API_VmmZoneStatus
#define vmmVirtualStatus        API_VmmVirtualStatus

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __VMM_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
