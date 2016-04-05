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
** ��   ��   ��: pageTable.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ҳ�����ز�����.

** BUG:
2009.06.18  ���������ַ�������ַ�Ĳ�ѯ API �� BSP �ӿ�.
2013.12.21  �����Ƿ��ִ��ѡ��.
*********************************************************************************************************/

#ifndef __PAGETABLE_H
#define __PAGETABLE_H

/*********************************************************************************************************
  mmu ҳ���־
*********************************************************************************************************/

#define LW_VMM_FLAG_VALID               0x01                            /*  ӳ����Ч                    */
#define LW_VMM_FLAG_UNVALID             0x00                            /*  ӳ����Ч                    */

#define LW_VMM_FLAG_ACCESS              0x02                            /*  ���Է���                    */
#define LW_VMM_FLAG_UNACCESS            0x00                            /*  ���ܷ���                    */

#define LW_VMM_FLAG_WRITABLE            0x04                            /*  ����д����                  */
#define LW_VMM_FLAG_UNWRITABLE          0x00                            /*  ������д����                */

#define LW_VMM_FLAG_EXECABLE            0x08                            /*  ����ִ�д���                */
#define LW_VMM_FLAG_UNEXECABLE          0x00                            /*  ������ִ�д���              */

#define LW_VMM_FLAG_CACHEABLE           0x10                            /*  ���Ի���                    */
#define LW_VMM_FLAG_UNCACHEABLE         0x00                            /*  �����Ի���                  */

#define LW_VMM_FLAG_BUFFERABLE          0x20                            /*  ����д����                  */
#define LW_VMM_FLAG_UNBUFFERABLE        0x00                            /*  ������д����                */

#define LW_VMM_FLAG_GUARDED             0x40                            /*  �����ϸ��Ȩ�޼��          */
#define LW_VMM_FLAG_UNGUARDED           0x00                            /*  �������ϸ��Ȩ�޼��        */

/*********************************************************************************************************
  Ĭ��ҳ���־
*********************************************************************************************************/

#define LW_VMM_FLAG_EXEC                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_EXECABLE |     \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_BUFFERABLE |   \
                                         LW_VMM_FLAG_GUARDED)           /*  ��ִ������                  */

#define LW_VMM_FLAG_READ                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_BUFFERABLE |   \
                                         LW_VMM_FLAG_GUARDED)           /*  ֻ������                    */
                                         
#define LW_VMM_FLAG_RDWR                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_WRITABLE |     \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_BUFFERABLE |   \
                                         LW_VMM_FLAG_GUARDED)           /*  ��д����                    */

#define LW_VMM_FLAG_DMA                 (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_WRITABLE |     \
                                         LW_VMM_FLAG_GUARDED)           /*  ����Ӳ��ӳ��                */
                                         
#define LW_VMM_FLAG_FAIL                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_UNACCESS |     \
                                         LW_VMM_FLAG_GUARDED)           /*  �������������              */

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  mmu ִ�й���
*********************************************************************************************************/

typedef struct {
    ULONG                    MMUOP_ulOption;                            /*  MMU ѡ��                    */
#define LW_VMM_MMU_FLUSH_TLB_MP     0x01                                /*  ÿһ�����Ƿ�Ҫ����      */

    FUNCPTR                  MMUOP_pfuncMemInit;                        /*  ��ʼ���ڴ�, (ҳ���Ŀ¼��)  */
    FUNCPTR                  MMUOP_pfuncGlobalInit;                     /*  ��ʼ��ȫ��ӳ���ϵ          */
    
    LW_PGD_TRANSENTRY     *(*MMUOP_pfuncPGDAlloc)();                    /*  ���� PGD �ռ�               */
    VOIDFUNCPTR              MMUOP_pfuncPGDFree;                        /*  �ͷ� PGD �ռ�               */
    LW_PMD_TRANSENTRY     *(*MMUOP_pfuncPMDAlloc)();                    /*  ���� PMD �ռ�               */
    VOIDFUNCPTR              MMUOP_pfuncPMDFree;                        /*  �ͷ� PMD �ռ�               */
    LW_PTE_TRANSENTRY     *(*MMUOP_pfuncPTEAlloc)();                    /*  ���� PTE �ռ�               */
    VOIDFUNCPTR              MMUOP_pfuncPTEFree;                        /*  �ͷ� PTE �ռ�               */

    BOOLFUNCPTR              MMUOP_pfuncPGDIsOk;                        /*  PGD ������Ƿ���ȷ          */
    BOOLFUNCPTR              MMUOP_pfuncPMDIsOk;                        /*  PMD ������Ƿ���ȷ          */
    BOOLFUNCPTR              MMUOP_pfuncPTEIsOk;                        /*  PTE ������Ƿ���ȷ          */

    LW_PGD_TRANSENTRY     *(*MMUOP_pfuncPGDOffset)();                   /*  ͨ����ַ���ָ�� PGD ����   */
    LW_PMD_TRANSENTRY     *(*MMUOP_pfuncPMDOffset)();                   /*  ͨ����ַ���ָ�� PMD ����   */
    LW_PTE_TRANSENTRY     *(*MMUOP_pfuncPTEOffset)();                   /*  ͨ����ַ���ָ�� PTE ����   */
    
    FUNCPTR                  MMUOP_pfuncPTEPhysGet;                     /*  ͨ�� PTE ��Ŀ��ȡ�����ַ   */
    
    ULONGFUNCPTR             MMUOP_pfuncFlagGet;                        /*  ���ҳ���־                */
    FUNCPTR                  MMUOP_pfuncFlagSet;                        /*  ����ҳ���־ (��ǰδʹ��)   */
    
    VOIDFUNCPTR              MMUOP_pfuncMakeTrans;                      /*  ����ҳ��ת����ϵ������      */
    VOIDFUNCPTR              MMUOP_pfuncMakeCurCtx;                     /*  ���ǰ��ҳ��ת����ϵ      */
    VOIDFUNCPTR              MMUOP_pfuncInvalidateTLB;                  /*  ��Ч TLB ��                 */
    
    VOIDFUNCPTR              MMUOP_pfuncSetEnable;                      /*  ���� MMU                    */
    VOIDFUNCPTR              MMUOP_pfuncSetDisable;                     /*  �ر� MMU                    */
} LW_MMU_OP;
typedef LW_MMU_OP           *PLW_MMU_OP;

extern  LW_MMU_OP            _G_mmuOpLib;                               /*  MMU ����������              */
/*********************************************************************************************************
  MMU ���ѡ����Ϣ
*********************************************************************************************************/
#define __VMM_MMU_OPTION()                      _G_mmuOpLib.MMUOP_ulOption

/*********************************************************************************************************
  MMU ������
*********************************************************************************************************/

#define __VMM_MMU_MEM_INIT(pmmuctx)             (_G_mmuOpLib.MMUOP_pfuncMemInit) ?  \
            _G_mmuOpLib.MMUOP_pfuncMemInit(pmmuctx) : (PX_ERROR)
#define __VMM_MMU_GLOBAL_INIT(pcmachine)        (_G_mmuOpLib.MMUOP_pfuncGlobalInit) ?    \
            _G_mmuOpLib.MMUOP_pfuncGlobalInit(pcmachine) : (PX_ERROR)
            
/*********************************************************************************************************
  MMU ҳ�濪�����ͷ�
*********************************************************************************************************/

#define __VMM_MMU_PGD_ALLOC(pmmuctx, ulAddr)                (_G_mmuOpLib.MMUOP_pfuncPGDAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPGDAlloc(pmmuctx, ulAddr) : (LW_NULL)
#define __VMM_MMU_PGD_FREE(p_pgdentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPGDFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPGDFree(p_pgdentry);                         \
        }
#define __VMM_MMU_PMD_ALLOC(pmmuctx, p_pgdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPMDAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPMDAlloc(pmmuctx, p_pgdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PMD_FREE(p_pmdentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPMDFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPMDFree(p_pmdentry); \
        }
#define __VMM_MMU_PTE_ALLOC(pmmuctx, p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPTEAlloc(pmmuctx, p_pmdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PTE_FREE(p_pteentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPTEFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPTEFree(p_pteentry); \
        }
        
/*********************************************************************************************************
  MMU ҳ���������ж�
*********************************************************************************************************/

#define __VMM_MMU_PGD_NONE(pgdentry)    (_G_mmuOpLib.MMUOP_pfuncPGDIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPGDIsOk(pgdentry)) : (LW_TRUE)
#define __VMM_MMU_PMD_NONE(pmdentry)    (_G_mmuOpLib.MMUOP_pfuncPMDIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPMDIsOk(pmdentry)) : (LW_TRUE)
#define __VMM_MMU_PTE_NONE(pteentry)    (_G_mmuOpLib.MMUOP_pfuncPTEIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPTEIsOk(pteentry)) : (LW_TRUE)
            
/*********************************************************************************************************
  MMU ҳ����������ȡ
*********************************************************************************************************/

#define __VMM_MMU_PGD_OFFSET(pmmuctx, ulAddr)       (_G_mmuOpLib.MMUOP_pfuncPGDOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPGDOffset(pmmuctx, ulAddr) : (LW_NULL)
#define __VMM_MMU_PMD_OFFSET(p_pgdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPMDOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPMDOffset(p_pgdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PTE_OFFSET(p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTEOffset(p_pmdentry, ulAddr) : (LW_NULL)
            
/*********************************************************************************************************
  MMU ��ȡ�����ַ
*********************************************************************************************************/

#define __VMM_MMU_PHYS_GET(pteentry, pulPhysicalAddr)   (_G_mmuOpLib.MMUOP_pfuncPTEPhysGet) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTEPhysGet(pteentry, pulPhysicalAddr) : (PX_ERROR)
            
/*********************************************************************************************************
  MMU ҳ����������־
*********************************************************************************************************/

#define __VMM_MMU_FLAG_GET(pmmuctx, ulAddr)         (_G_mmuOpLib.MMUOP_pfuncFlagGet) ?  \
            _G_mmuOpLib.MMUOP_pfuncFlagGet(pmmuctx, ulAddr) : 0ul
#define __VMM_MMU_FLAG_SET(pmmuctx, ulAddr, ulFlag) (_G_mmuOpLib.MMUOP_pfuncFlagSet) ?  \
            _G_mmuOpLib.MMUOP_pfuncFlagSet(pmmuctx, ulAddr, (ulFlag)) : (PX_ERROR)
            
/*********************************************************************************************************
  MMU �ڲ�����
*********************************************************************************************************/

#define __VMM_MMU_MAKE_TRANS(pmmuctx, p_pteentry, ulVirtualAddr, ulPhysicalAddr, ulFlag)   \
        if (_G_mmuOpLib.MMUOP_pfuncMakeTrans) { \
            _G_mmuOpLib.MMUOP_pfuncMakeTrans(pmmuctx, p_pteentry,                       \
                                             ulVirtualAddr, ulPhysicalAddr, (ulFlag));  \
        }
#define __VMM_MMU_MAKE_CURCTX(pmmuctx)  \
        if (_G_mmuOpLib.MMUOP_pfuncMakeCurCtx) {    \
            _G_mmuOpLib.MMUOP_pfuncMakeCurCtx(pmmuctx); \
        }
#define __VMM_MMU_INV_TLB(pmmuctx)  \
        if (_G_mmuOpLib.MMUOP_pfuncInvalidateTLB) { \
            _G_mmuOpLib.MMUOP_pfuncInvalidateTLB(pmmuctx); \
        }
#define __VMM_MMU_ENABLE()  \
        if (_G_mmuOpLib.MMUOP_pfuncSetEnable) { \
            _G_mmuOpLib.MMUOP_pfuncSetEnable(); \
        }
#define __VMM_MMU_DISABLE() \
        if (_G_mmuOpLib.MMUOP_pfuncSetDisable) { \
            _G_mmuOpLib.MMUOP_pfuncSetDisable(); \
        }
        
/*********************************************************************************************************
  mmu ��Ϣ
*********************************************************************************************************/

typedef struct __lw_mmu_context {
    LW_VMM_ZONE              MMUCTX_vmzoneVirtual;                      /*  �����ַ��ҳ����            */
    LW_VMM_AREA              MMUCTX_vmareaVirtualSpace;                 /*  �����ַ�ռ�                */
    LW_PGD_TRANSENTRY       *MMUCTX_pgdEntry;                           /*  PGD ����ڵ�ַ              */
} LW_MMU_CONTEXT;
typedef LW_MMU_CONTEXT      *PLW_MMU_CONTEXT;

/*********************************************************************************************************
  VMM �ڲ�ƥ��
*********************************************************************************************************/

static LW_INLINE  LW_PGD_TRANSENTRY  *__vmm_pgd_alloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    if (pmmuctx->MMUCTX_pgdEntry == LW_NULL) {
        return  (__VMM_MMU_PGD_ALLOC(pmmuctx, ulAddr));
    } else {
        return  (__VMM_MMU_PGD_OFFSET(pmmuctx, ulAddr));
    }
}

static LW_INLINE  VOID  __vmm_pgd_free (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    __VMM_MMU_PGD_FREE(p_pgdentry);
}

static LW_INLINE LW_PMD_TRANSENTRY   *__vmm_pmd_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PGD_TRANSENTRY *p_pgdentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PGD_NONE(*p_pgdentry)) {
        return  (__VMM_MMU_PMD_ALLOC(pmmuctx, p_pgdentry, ulAddr));
    } else {
        return  (__VMM_MMU_PMD_OFFSET(p_pgdentry, ulAddr));
    }
}

static LW_INLINE VOID  __vmm_pmd_free (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    __VMM_MMU_PMD_FREE(p_pmdentry);
}

static LW_INLINE LW_PTE_TRANSENTRY   *__vmm_pte_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PMD_TRANSENTRY *p_pmdentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PMD_NONE(*p_pmdentry)) {
        return  (__VMM_MMU_PTE_ALLOC(pmmuctx, p_pmdentry, ulAddr));
    } else {
        return  (__VMM_MMU_PTE_OFFSET(p_pmdentry, ulAddr));
    }
}

static LW_INLINE VOID  __vmm_pte_free (LW_PTE_TRANSENTRY  *p_pteentry)
{
    __VMM_MMU_PTE_FREE(p_pteentry);
}

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

PLW_MMU_CONTEXT         __vmmGetCurCtx(VOID);                           /*  get current mmu context     */
ULONG                   __vmmLibPrimaryInit(PLW_MMU_GLOBAL_DESC  pmmugdesc,
                                            CPCHAR               pcMachineName);
                                                                        /*  init current mmu context    */
#if LW_CFG_SMP_EN > 0
ULONG                   __vmmLibSecondaryInit(CPCHAR  pcMachineName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

ULONG                   __vmmLibPageMap(addr_t ulPhysicalAddr, 
                                        addr_t ulVirtualAddr, 
                                        ULONG  ulPageNum, 
                                        ULONG  ulFlag);                 /*  mmu map                     */
ULONG                   __vmmLibGetFlag(addr_t  ulVirtualAddr, ULONG  *pulFlag);
ULONG                   __vmmLibSetFlag(addr_t  ulVirtualAddr, ULONG  ulFlag);
ULONG                   __vmmLibVirtualToPhysical(addr_t  ulVirtualAddr, addr_t  *pulPhysicalAddr);

/*********************************************************************************************************
  bsp api
*********************************************************************************************************/

LW_API LW_MMU_OP       *API_VmmGetLibBlock(VOID);                       /*  BSP get mmu op lib          */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __PAGETABLE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
