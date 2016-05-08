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
** ��   ��   ��: pageLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ����ҳ������.

** BUG:
2009.09.30  ��ʾ virtuals ʱ, ������ϸ.
2009.11.13  ���� DMA ���ӡ�������ʴ�ӡ.
2011.05.17  ����ȱҳ�ж���Ϣ��ʾ.
2011.08.03  ������ҳ��û�� Link ����ҳ��ʱ, ��Ҫʹ��ҳ���ѯ��ȷ��ӳ��������ַ.
2013.06.04  ��ʾ����ռ���Ϣʱ, ������ʾ�����ַ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
#if LW_CFG_VMM_EN > 0
#include "../SylixOS/kernel/vmm/virPage.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64
static const CHAR   _G_cZoneInfoHdr[] = "\n\
ZONE     PHYSICAL           SIZE       PAGESIZE       PGD        FREEPAGE  DMA  USED\n\
---- ---------------- ---------------- -------- ---------------- -------- ----- ----\n";
static const CHAR   _G_cAreaInfoHdr[] = "\n\
     VIRTUAL          SIZE        WRITE CACHE\n\
---------------- ---------------- ----- -----\n";
#else
static const CHAR   _G_cZoneInfoHdr[] = "\n\
ZONE PHYSICAL   SIZE   PAGESIZE    PGD   FREEPAGE  DMA  USED\n\
---- -------- -------- -------- -------- -------- ----- ----\n";
static const CHAR   _G_cAreaInfoHdr[] = "\n\
VIRTUAL    SIZE   WRITE CACHE\n\
-------- -------- ----- -----\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
extern LW_VMM_ZONE          _G_vmzonePhysical[LW_CFG_VMM_ZONE_NUM];     /*  ��������                    */
/*********************************************************************************************************
  �ڲ���������
*********************************************************************************************************/
extern ULONG                __vmmLibVirtualToPhysical(addr_t  ulVirtualAddr, addr_t  *pulPhysical);
/*********************************************************************************************************
** ��������: API_VmmPhysicalShow
** ��������: ��ʾ vmm ����洢����Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmPhysicalShow (VOID)
{
    REGISTER INT                i;
             PLW_MMU_CONTEXT    pmmuctx = __vmmGetCurCtx();
             PCHAR              pcDma;
             size_t             stUsed;

    printf("vmm physical zone show >>\n");
    printf(_G_cZoneInfoHdr);                                            /*  ��ӡ��ӭ��Ϣ                */
    
    __VMM_LOCK();
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        pcDma  = (_G_vmzonePhysical[i].ZONE_uiAttr & LW_ZONE_ATTR_DMA) ? "true" : "false";
        stUsed = (_G_vmzonePhysical[i].ZONE_stSize 
               - (size_t)(_G_vmzonePhysical[i].ZONE_ulFreePage << LW_CFG_VMM_PAGE_SHIFT));
        stUsed = (stUsed / (_G_vmzonePhysical[i].ZONE_stSize / 100));   /*  ��ֹ���                    */
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        printf("%4d %16lx %16zx %8zx %16lx %8ld %-5s %3zd%%\n",
#else
        printf("%4d %08lx %8zx %8zx %08lx %8ld %-5s %3zd%%\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
               i, 
               _G_vmzonePhysical[i].ZONE_ulAddr,
               _G_vmzonePhysical[i].ZONE_stSize,
               (size_t)LW_CFG_VMM_PAGE_SIZE,
               (addr_t)pmmuctx->MMUCTX_pgdEntry,
               _G_vmzonePhysical[i].ZONE_ulFreePage,
               pcDma,
               stUsed);
    }
    __VMM_UNLOCK();
    
    printf("\n");
}
/*********************************************************************************************************
** ��������: __vmmVirtualPrint
** ��������: ��ӡ��Ϣ�ص�����
** �䡡��  : pvmpage  ҳ����Ϣ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __vmmVirtualPrint (PLW_VMM_PAGE  pvmpage)
{
    addr_t  ulVirtualAddr  = pvmpage->PAGE_ulPageAddr;
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    printf("%16lx %16lx ", ulVirtualAddr, (pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#else
    printf("%08lx %8lx ", ulVirtualAddr, (pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */

    if (pvmpage->PAGE_ulFlags & LW_VMM_FLAG_WRITABLE) {
        printf("true  ");
    } else {
        printf("false ");
    }
    
    if (pvmpage->PAGE_ulFlags & LW_VMM_FLAG_CACHEABLE) {
        printf("true\n");
    } else {
        printf("false\n");
    }
}
/*********************************************************************************************************
** ��������: API_VmmVirtualShow
** ��������: ��ʾ vmm ����洢����Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmVirtualShow (VOID)
{
    PLW_MMU_VIRTUAL_DESC pvirdesc = __vmmVirtualDesc();
    
    printf("vmm virtual area show >>\n");
    printf("vmm virtual area from : 0x%08lx, size : 0x%08lx\n", 
                  (addr_t)pvirdesc->ulVirtualStart,
                  (addr_t)pvirdesc->stSize);
    printf("vmm virtual area usage as follow :\n");
                  
    printf(_G_cAreaInfoHdr);                                            /*  ��ӡ��ӭ��Ϣ                */
    
    __VMM_LOCK();
    __areaVirtualSpaceTraversal(__vmmVirtualPrint);                     /*  ��������ռ���, ��ӡ��Ϣ    */
    __VMM_UNLOCK();
    
    printf("\n");
}
/*********************************************************************************************************
** ��������: API_VmmAbortShow
** ��������: ��ʾ vmm ������ֹ��Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_VmmAbortShow (VOID)
{
    LW_VMM_STATUS  vmms;
    
    API_VmmAbortStatus(&vmms);

    printf("vmm abort statistics infomation show >>\n");
    printf("vmm abort (memory access error) counter : %lld\n", vmms.VMMS_i64AbortCounter);
    printf("vmm page fail (alloc success) counter   : %lld\n", vmms.VMMS_i64PageFailCounter);
    printf("vmm alloc physical page error counter   : %lld\n", vmms.VMMS_i64PageLackCounter);
    printf("vmm page map error counter              : %lld\n", vmms.VMMS_i64MapErrCounter);
    
    printf("\n");
}
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
