/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: cskyMmuCommon.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 14 ��
**
** ��        ��: C-SKY ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "cskyMmu.h"
#include "arch/csky/inc/cskyregs.h"
/*********************************************************************************************************
  ȫ�ֺ궨��
*********************************************************************************************************/
#define CSKY_MMU_TLB_SIZE               _G_ulMmuTlbSize                 /*  TLB �����С                */
/*********************************************************************************************************
  ENTRYLO PFN ����
*********************************************************************************************************/
#define CSKY_ENTRYLO_PFN_MASK           (0xfffff << CSKY_ENTRYLO_PFN_SHIFT)
/*********************************************************************************************************
  UNIQUE ENTRYHI
*********************************************************************************************************/
#define SSEG0                           0x80000000
#define CSKY_UNIQUE_ENTRYHI(idx)        (SSEG0 + ((idx) << (LW_CFG_VMM_PAGE_SHIFT + 1)))
/*********************************************************************************************************
  PAGE ����
*********************************************************************************************************/
#if   LW_CFG_VMM_PAGE_SIZE == (4  * LW_CFG_KB_SIZE)
#define CSKY_MMU_PAGE_MASK              PM_4K
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define CSKY_MMU_PAGE_MASK              PM_16K
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define CSKY_MMU_PAGE_MASK              PM_64K
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K)!
#endif                                                                  /*  LW_CFG_VMM_PAGE_SIZE        */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static ULONG                _G_ulMmuTlbSize  = 128;                     /*  TLB �����С                */
static LW_OBJECT_HANDLE     _G_hPGDPartition = LW_HANDLE_INVALID;       /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition = LW_HANDLE_INVALID;       /*  PTE ������                  */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
extern VOID  cskyMmuContextSet(ULONG  ulValue);
extern VOID  cskyMmuPageMaskSet(ULONG  ulValue);
/*********************************************************************************************************
** ��������: cskyMmuEnable
** ��������: ʹ�� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyMmuEnable (VOID)
{
}
/*********************************************************************************************************
** ��������: cskyMmuDisable
** ��������: ���� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyMmuDisable (VOID)
{
}
/*********************************************************************************************************
** ��������: cskyMmuBuildPgdEntry
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : ulBaseAddr              ����ҳ�����ַ
** �䡡��  : һ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  cskyMmuBuildPgdEntry (addr_t  ulBaseAddr)
{
    return  (ulBaseAddr);                                               /*  һ�����������Ƕ���ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: cskyMmuBuildPteEntry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : ulBaseAddr              ����ҳ��ַ
**           ulFlag                  ��־
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  cskyMmuBuildPteEntry (addr_t  ulBaseAddr, ULONG  ulFlag)
{
    LW_PTE_TRANSENTRY   pteentry;
    ULONG               ulPFN;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ulPFN = ulBaseAddr >> 12;                                       /*  ���� PFN                    */

        pteentry = ulPFN << CSKY_ENTRYLO_PFN_SHIFT;                     /*  ��� PFN                    */

        if (ulFlag & LW_VMM_FLAG_VALID) {
            pteentry |= ENTRYLO_V;                                      /*  ��� V λ                   */
        }

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            pteentry |= ENTRYLO_D;                                      /*  ��� D λ                   */
        }

        if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                           /*  ��� C λ                   */
            pteentry |= ENTRYLO_C;
        } 
    } else {
        pteentry = 0;
    }

    return  (pteentry);
}
/*********************************************************************************************************
** ��������: cskyMmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4096 * sizeof(LW_PGD_TRANSENTRY))
#define PTE_BLOCK_SIZE  ((LW_CFG_MB_SIZE / LW_CFG_VMM_PAGE_SIZE) * sizeof(LW_PTE_TRANSENTRY))

    PVOID   pvPgdTable;
    PVOID   pvPteTable;

    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    ULONG   ulPteNum = bspMmuPteMaxNum();

    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);

    if (!pvPgdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, PTE_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);

    if (!_G_hPGDPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyMmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *cskyMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER ULONG               ulPgdNum;

    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry +
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: cskyMmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *cskyMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  �� PMD ��                   */
}
/*********************************************************************************************************
** ��������: cskyMmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *cskyMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   pmdentry;
    REGISTER ULONG               ulPageNum;

    pmdentry   = (*p_pmdentry);                                         /*  ���һ��ҳ��������          */
    p_pteentry = (LW_PTE_TRANSENTRY *)(pmdentry);                       /*  ��ö���ҳ�����ַ          */

    ulAddr    &= ~LW_CFG_VMM_PGD_MASK;
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry +
                  (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));             /*  ��������ַҳ����������ַ  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: cskyMmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  cskyMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: cskyMmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  cskyMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & ENTRYLO_V) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: cskyMmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *cskyMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;

    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    if (!p_pgdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);

    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  ���� PGD ��                 */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry +
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: cskyMmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: cskyMmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *cskyMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PGD_TRANSENTRY  *p_pgdentry,
                                           addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: cskyMmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: cskyMmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���:
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *cskyMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                            LW_PMD_TRANSENTRY  *p_pmdentry,
                                            addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)cskyMmuBuildPgdEntry((addr_t)p_pteentry);  /*  ���ö���ҳ�����ַ  */

    return  (cskyMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: cskyMmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: cskyMmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           pulPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    ULONG  ulPFN = (pteentry & CSKY_ENTRYLO_PFN_MASK) >>
                    CSKY_ENTRYLO_PFN_SHIFT;                             /*  �������ҳ���              */

    *pulPhysicalAddr = ulPFN << 12;                                     /*  ����ҳ�������ַ            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyMmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  cskyMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;
    LW_PTE_TRANSENTRY  *p_pteentry;
    LW_PTE_TRANSENTRY   pteentry;

    p_pgdentry = cskyMmuPgdOffset(pmmuctx, ulAddr);                     /*  ���һ����������ַ          */
    if (cskyMmuPgdIsOk(*p_pgdentry)) {                                  /*  һ����������Ч              */

        p_pteentry = cskyMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                      ulAddr);                          /*  ��ö�����������ַ          */
        pteentry = *p_pteentry;                                         /*  ��ö���������              */
        if (cskyMmuPteIsOk(pteentry)) {                                 /*  ������������Ч              */
            ULONG   ulFlag = 0;            

            if (pteentry & ENTRYLO_V) {                                 /*  ��Ч                        */
                ulFlag |= LW_VMM_FLAG_VALID;                            /*  ӳ����Ч                    */
            }

            ulFlag |= LW_VMM_FLAG_ACCESS |                              /*  ���Է���                    */
                      LW_VMM_FLAG_EXECABLE;                             /*  ����ִ��                    */

            if (pteentry & ENTRYLO_D) {                                 /*  ��д                        */
                ulFlag |= LW_VMM_FLAG_WRITABLE;
            }
           
            if (pteentry & ENTRYLO_C) {                                 /*  ���� CACHE                  */
                ulFlag |= LW_VMM_FLAG_CACHEABLE;
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: cskyMmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  cskyMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry;
    LW_PTE_TRANSENTRY  *p_pteentry;
    LW_PTE_TRANSENTRY   pteentry;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    p_pgdentry = cskyMmuPgdOffset(pmmuctx, ulAddr);                     /*  ���һ����������ַ          */
    if (cskyMmuPgdIsOk(*p_pgdentry)) {                                  /*  һ����������Ч              */

        p_pteentry = cskyMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                      ulAddr);                          /*  ��ö�����������ַ          */
        pteentry= *p_pteentry;                                          /*  ��ö���������              */
        if (cskyMmuPteIsOk(pteentry)) {                                 /*  ������������Ч              */
            ULONG   ulPFN = (pteentry & CSKY_ENTRYLO_PFN_MASK) >>
                             CSKY_ENTRYLO_PFN_SHIFT;                    /*  �������ҳ���              */
            addr_t  ulPhysicalAddr = ulPFN << 12;                       /*  ����ҳ�������ַ            */

            *p_pteentry = cskyMmuBuildPteEntry(ulPhysicalAddr, ulFlag);
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: cskyMmuMakeTrans
** ��������: ����ҳ��ӳ���ϵ
** �䡡��  : pmmuctx        mmu ������
**           p_pteentry     ��Ӧ��ҳ����
**           ulVirtualAddr  �����ַ
**           paPhysicalAddr �����ַ
**           ulFlag         ��Ӧ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static VOID  cskyMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                               LW_PTE_TRANSENTRY  *p_pteentry,
                               addr_t              ulVirtualAddr,
                               phys_addr_t         paPhysicalAddr,
                               ULONG               ulFlag)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return;
    }

    /*
     * �������������������ö���������
     */
    *p_pteentry = cskyMmuBuildPteEntry(paPhysicalAddr, ulFlag);
}
/*********************************************************************************************************
** ��������: cskyMmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    cskyMmuContextSet((addr_t)pmmuctx->MMUCTX_pgdEntry);
}
/*********************************************************************************************************
** ��������: cskyMmuInvalidateTLB
** ��������: ��Ч TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �ڲ�����Ч Micro TLB
*********************************************************************************************************/
VOID  cskyMmuInvalidateTLB (VOID)
{
    ULONG   ulEntryHiBak = cskyEntryHiRead();
    INT     i;

    for (i = 0; i < CSKY_MMU_TLB_SIZE; i++) {
        cskyIndexWrite(i);
        cskyEntryLo0Write(0);
        cskyEntryLo1Write(0);
        cskyEntryHiWrite(CSKY_UNIQUE_ENTRYHI(i));
        cskyTlbWriteIndexed();
    }

    cskyEntryHiWrite(ulEntryHiBak);
}
/*********************************************************************************************************
** ��������: cskyMmuInvalidateTLBMVA
** ��������: ��Чָ�� MVA �� TLB
** �䡡��  : ulAddr            ָ�� MVA
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �ڲ�������Ч Micro TLB, �ⲿ������ɺ������Ч Micro TLB
*********************************************************************************************************/
VOID  cskyMmuInvalidateTLBMVA (addr_t  ulAddr)
{
    ULONG   ulEntryHiBak = cskyEntryHiRead();
    ULONG   ulEntryHi    = ulAddr & (LW_CFG_VMM_PAGE_MASK << 1);
    INT32   iIndex;
    INT     iReTry;

    for (iReTry = 0; iReTry < 2; iReTry++) {                            /*  �����������һ���� TLB ��Ŀ */
        cskyEntryHiWrite(ulEntryHi);
        cskyTlbProbe();
        iIndex = cskyIndexRead();
        if (iIndex >= 0) {
            cskyIndexWrite(iIndex);
            cskyEntryLo0Write(0);
            cskyEntryLo1Write(0);
            cskyEntryHiWrite(CSKY_UNIQUE_ENTRYHI(iIndex));
            cskyTlbWriteIndexed();

        } else {
            break;
        }
    }

    cskyEntryHiWrite(ulEntryHiBak);
}
/*********************************************************************************************************
** ��������: cskyMmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    REGISTER ULONG   i;

    if (ulPageNum > (CSKY_MMU_TLB_SIZE >> 1)) {
        cskyMmuInvalidateTLB();                                         /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            cskyMmuInvalidateTLBMVA(ulPageAddr);                        /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: cskyMmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyMmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);                                      /*  ��λ CACHE                  */

    cskyMmuPageMaskSet(CSKY_MMU_PAGE_MASK);                             /*  PAGE MASK                   */

    cskyEntryHiWrite(0);                                                /*  ASID = 0                    */

    cskyMmuInvalidateTLB();                                             /*  ��Ч TLB                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyMmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  cskyMmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    _G_ulMmuTlbSize = 128;                                              /*  û�мĴ������Զ��� TLB ��Ŀ */

#if LW_CFG_SMP_EN > 0
    pmmuop->MMUOP_ulOption = LW_VMM_MMU_FLUSH_TLB_MP;
#else
    pmmuop->MMUOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    pmmuop->MMUOP_pfuncMemInit    = cskyMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = cskyMmuGlobalInit;

    pmmuop->MMUOP_pfuncPGDAlloc   = cskyMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree    = cskyMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc   = cskyMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree    = cskyMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc   = cskyMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree    = cskyMmuPteFree;

    pmmuop->MMUOP_pfuncPGDIsOk    = cskyMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk    = cskyMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk    = cskyMmuPteIsOk;

    pmmuop->MMUOP_pfuncPGDOffset  = cskyMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset  = cskyMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset  = cskyMmuPteOffset;

    pmmuop->MMUOP_pfuncPTEPhysGet = cskyMmuPtePhysGet;

    pmmuop->MMUOP_pfuncFlagGet    = cskyMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet    = cskyMmuFlagSet;

    pmmuop->MMUOP_pfuncMakeTrans     = cskyMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = cskyMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = cskyMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = cskyMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = cskyMmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
