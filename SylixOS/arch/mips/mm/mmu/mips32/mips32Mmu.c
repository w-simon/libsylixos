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
** ��   ��   ��: mips32Mmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 10 �� 12 ��
**
** ��        ��: MIPS32 ��ϵ���� MMU ����.
**
** BUG:
2016.04.06  �޸� TLB ��Ч�� EntryHi Register ����(JZ4780 ֧��)
2016.06.14  Ϊ֧�ַ� 4K ��Сҳ���ع�����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition = LW_HANDLE_INVALID;       /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static PVOID                _G_pvPTETable    = LW_NULL;                 /*  PTE ��                      */
static UINT32               _G_uiTlbSize     = 0;                       /*  TLB �����С                */
static INT                  _G_iMachineType  = MIPS_MACHINE_TYPE_24KF;  /*  ��������                    */
/*********************************************************************************************************
  ��о Loongson2x 3x ϵ�д��������е� TLB(VTLB �� JTLB һ������, �Ͳ����ж���)
*********************************************************************************************************/
static BOOL                 _G_bLsHasITLB = LW_FALSE;                   /*  �Ƿ��� ITLB                 */
static BOOL                 _G_bLsHasDTLB = LW_FALSE;                   /*  �Ƿ��� DTLB                 */
static BOOL                 _G_bLsHasFTLB = LW_FALSE;                   /*  �Ƿ��� FTLB                 */
static BOOL                 _G_bLsHasNE   = LW_FALSE;                   /*  �Ƿ��� NE λ                */
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define MIPS32_TLB_SIZE     _G_uiTlbSize                                /*  TLB �����С                */
/*********************************************************************************************************
  ENTRYLO
*********************************************************************************************************/
#define MIPS32_ENTRYLO_G_BIT            (1 << 0)
#define MIPS32_ENTRYLO_V_BIT            (1 << 1)
#define MIPS32_ENTRYLO_D_BIT            (1 << 2)

#define MIPS32_ENTRYLO_C_SHIFT          (3)
#define MIPS32_ENTRYLO_C_MASK           (0x07 << MIPS32_ENTRYLO_C_SHIFT)

#define MIPS32_ENTRYLO_PFN_SHIFT        (6)
#define MIPS32_ENTRYLO_PFN_MASK         (0xffffff << MIPS32_ENTRYLO_PFN_SHIFT)

#define MIPS32_ENTRYLO_NE_BIT           (1 << 30)
/*********************************************************************************************************
  ENTRYHI
*********************************************************************************************************/
#define MIPS32_UNIQUE_ENTRYHI(idx)      (A_K1BASE + ((idx) << (LW_CFG_VMM_PAGE_SHIFT + 1)))
/*********************************************************************************************************
  PAGE_MASK
*********************************************************************************************************/
#define MIPS32_PAGE_MASK_4K             0x00000000
#define MIPS32_PAGE_MASK_16K            0x00006000
#define MIPS32_PAGE_MASK_64K            0x0001e000

#if   LW_CFG_VMM_PAGE_SIZE == (4  * LW_CFG_KB_SIZE)
#define MIPS32_PAGE_MASK                MIPS32_PAGE_MASK_4K
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define MIPS32_PAGE_MASK                MIPS32_PAGE_MASK_16K
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define MIPS32_PAGE_MASK                MIPS32_PAGE_MASK_64K
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K)!
#endif                                                                  /*  LW_CFG_VMM_PAGE_SIZE        */
/*********************************************************************************************************
  TLB ����
*********************************************************************************************************/
#define MIPS32_MMU_TLB_WRITE()          do { MIPS_EXEC_INS("TLBWI"); MIPS_EXEC_INS("EHB"); } while(0)
#define MIPS32_MMU_TLB_READ()           do { MIPS_EXEC_INS("TLBR");  MIPS_EXEC_INS("EHB"); } while(0)
#define MIPS32_MMU_TLB_PROBE()          do { MIPS_EXEC_INS("TLBP");  MIPS_EXEC_INS("EHB"); } while(0)
/*********************************************************************************************************
** ��������: mips32MmuInvalidateMicroTLB
** ��������: ��Ч Micro TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips32MmuInvalidateMicroTLB (VOID)
{
    if (_G_bLsHasITLB && _G_bLsHasDTLB) {
        mipsCp0DiagWrite((1 << 3) |
                         (1 << 2));                                     /*  ��� ITLB DTLB              */

    } else if (_G_bLsHasITLB) {
        mipsCp0DiagWrite(1 << 2);                                       /*  ��� ITLB                   */
    }
}
/*********************************************************************************************************
** ��������: mips32MmuEnable
** ��������: ʹ�� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips32MmuEnable (VOID)
{
}
/*********************************************************************************************************
** ��������: mips32MmuDisable
** ��������: ���� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips32MmuDisable (VOID)
{
}
/*********************************************************************************************************
** ��������: mips32MmuInvalidateTLB
** ��������: ��Ч TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �ڲ�����Ч Micro TLB
*********************************************************************************************************/
static VOID  mips32MmuInvalidateTLB (VOID)
{
    if (_G_bLsHasITLB && _G_bLsHasDTLB && _G_bLsHasFTLB) {
        mipsCp0DiagWrite((1 << 13) |
                         (1 << 12) |
                         (1 <<  3) |
                         (1 <<  2));                                    /*  ��� ITLB DTLB VTLB FTLB    */

    } else {
        UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
        INT     i;

        for (i = 0; i < MIPS32_TLB_SIZE; i++) {
            mipsCp0IndexWrite(i);

            mipsCp0EntryLo0Write(0);
            mipsCp0EntryLo1Write(0);

            mipsCp0EntryHiWrite(MIPS32_UNIQUE_ENTRYHI(i));

            MIPS32_MMU_TLB_WRITE();
        }

        mipsCp0EntryHiWrite(uiEntryHiBak);

        mips32MmuInvalidateMicroTLB();                                  /*  ��Ч Micro TLB              */
    }
}
/*********************************************************************************************************
** ��������: mips32MmuDumpTLB
** ��������: Dump TLB
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mips32MmuDumpTLB (VOID)
{
             UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
             UINT32  uiEntryLo0;
             UINT32  uiEntryLo1;
             UINT32  uiEntryHi;
             UINT32  uiPageMask;
    REGISTER INT     i;

    for (i = 0; i < MIPS32_TLB_SIZE; i++) {
        mipsCp0IndexWrite(i);

        MIPS32_MMU_TLB_READ();

        uiEntryLo0 = mipsCp0EntryLo0Read();
        uiEntryLo1 = mipsCp0EntryLo1Read();
        uiEntryHi  = mipsCp0EntryHiRead();
        uiPageMask = mipsCp0PageMaskRead();

        _PrintFormat("TLB[%02d]: EntryLo0=0x%08x, EntryLo1=0x%08x, "
                     "EntryHi=0x%08x, PageMask=0x%08x\r\n",
                     i, uiEntryLo0, uiEntryLo1, uiEntryHi, uiPageMask);
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);
}
/*********************************************************************************************************
** ��������: mips32MmuInvalidateTLBMVA
** ��������: ��Чָ�� MVA �� TLB
** �䡡��  : ulAddr            ָ�� MVA
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �ڲ�������Ч Micro TLB, �ⲿ������ɺ������Ч Micro TLB
*********************************************************************************************************/
static VOID  mips32MmuInvalidateTLBMVA (addr_t  ulAddr)
{
             UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
    REGISTER UINT32  uiEntryHi    = ulAddr & (LW_CFG_VMM_PAGE_MASK << 1);
    REGISTER INT32   iIndex;
    REGISTER INT     iReTry;

    for (iReTry = 0; iReTry < 2; iReTry++) {                            /*  �����������һ���� TLB ��Ŀ */
        mipsCp0EntryHiWrite(uiEntryHi);

        MIPS32_MMU_TLB_PROBE();

        iIndex = mipsCp0IndexRead();
        if (iIndex >= 0) {
            mipsCp0IndexWrite(iIndex);

            mipsCp0EntryLo0Write(0);
            mipsCp0EntryLo1Write(0);

            mipsCp0EntryHiWrite(MIPS32_UNIQUE_ENTRYHI(iIndex));

            MIPS32_MMU_TLB_WRITE();
        } else {
            break;
        }
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);
}
/*********************************************************************************************************
** ��������: mips32MmuBuildPgdesc
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : uiBaseAddr              ����ҳ�����ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  mips32MmuBuildPgdesc (UINT32  uiBaseAddr)
{
    return  (uiBaseAddr);                                               /*  һ�����������Ƕ���ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: mips32MmuBuildPtentry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : uiBaseAddr              ����ҳ��ַ
**           uiFlag
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  mips32MmuBuildPtentry (UINT32  uiBaseAddr,
                                                 ULONG   ulFlag)
{
    LW_PTE_TRANSENTRY   uiDescriptor;
    UINT32              uiPFN;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        uiPFN = uiBaseAddr >> 12;                                       /*  ���� PFN                    */

        uiDescriptor = uiPFN << MIPS32_ENTRYLO_PFN_SHIFT;               /*  ��� PFN                    */

        if (ulFlag & LW_VMM_FLAG_VALID) {
            uiDescriptor |= MIPS32_ENTRYLO_V_BIT;                       /*  ��� V λ                   */
        }

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            uiDescriptor |= MIPS32_ENTRYLO_D_BIT;                       /*  ��� D λ                   */
        }

        uiDescriptor |= MIPS32_ENTRYLO_G_BIT;                           /*  ��� G λ                   */

        if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                           /*  ��� C λ                   */
                                                                        /*  ��о 3A ��Ϊһ���� CACHE    */
            uiDescriptor |= MIPS_CACHABLE_NONCOHERENT << MIPS32_ENTRYLO_C_SHIFT;
        } else {
            uiDescriptor |= MIPS_UNCACHED << MIPS32_ENTRYLO_C_SHIFT;
        }

        if (_G_bLsHasNE) {
            if (!(ulFlag & LW_VMM_FLAG_EXECABLE)) {                     /*  ����ִ��                    */
                uiDescriptor |= MIPS32_ENTRYLO_NE_BIT;                  /*  ��� NE λ                  */
            }
        }
    } else {
        uiDescriptor = 0;
    }

    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: mips32MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mips32MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4096 * sizeof(LW_PGD_TRANSENTRY))
#define PTE_BLOCK_SIZE  ((LW_CFG_MB_SIZE / LW_CFG_VMM_PAGE_SIZE) * sizeof(LW_PTE_TRANSENTRY))
#define PTE_TABLE_SIZE  ((LW_CFG_GB_SIZE / LW_CFG_VMM_PAGE_SIZE) * 4 * sizeof(LW_PTE_TRANSENTRY))

    PVOID   pvPgdTable;
    PVOID   pvPteTable;
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN(PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    if (!pvPgdTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }

    /*
     * PTE ����Ҫ 8MByte �������д�� Context �Ĵ���
     */
    pvPteTable = __KHEAP_ALLOC_ALIGN(PTE_TABLE_SIZE, 8 * LW_CFG_MB_SIZE);
    if (!pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        __KHEAP_FREE(pvPgdTable);
        return  (PX_ERROR);
    }

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, 1, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_hPGDPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        __KHEAP_FREE(pvPgdTable);
        __KHEAP_FREE(pvPteTable);
        return  (PX_ERROR);
    }
    
    lib_bzero(pvPteTable, PTE_TABLE_SIZE);

    _G_pvPTETable = pvPteTable;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32MmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mips32MmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);                                      /*  ��λ CACHE                  */
    
    mipsCp0PageMaskWrite(MIPS32_PAGE_MASK);                             /*  PAGE MASK                   */

    mipsCp0EntryHiWrite(0);                                             /*  ASID = 0                    */

    mipsCp0WiredWrite(0);                                               /*  ȫ����������滻            */

    mips32MmuInvalidateTLB();                                           /*  ��Ч TLB                    */

    if (_G_iMachineType == MIPS_MACHINE_TYPE_LS3X) {
        mipsCp0PageGrainWrite(0);                                       /*  ������о 3A ��һЩ��չ����  */
        if (_G_bLsHasFTLB) {
            mipsCp0GSConfigWrite(mipsCp0GSConfigRead() | (1 << 22));    /*  ֻ�� VTLB, ���� FTLB        */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *mips32MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER UINT32              uiPgdNum;
    
    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *mips32MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  MIPS �� PMD ��              */
}
/*********************************************************************************************************
** ��������: mips32MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *mips32MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;
    REGISTER UINT32              uiPageNum;

    uiTemp = (UINT32)(*p_pmdentry);                                     /*  ���һ��ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp);                         /*  ��ö���ҳ�����ַ          */

    ulAddr &= ~LW_CFG_VMM_PGD_MASK;

    uiPageNum = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                  (uiPageNum * sizeof(LW_PTE_TRANSENTRY)));             /*  ��������ַҳ����������ַ  */
    
    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  mips32MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mips32MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  mips32MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & MIPS32_ENTRYLO_V_BIT) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mips32MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *mips32MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    REGISTER UINT32              uiPgdNum;
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  ��Чһ��ҳ����              */

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mips32MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *mips32MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                             LW_PGD_TRANSENTRY  *p_pgdentry,
                                             addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: mips32MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mips32MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: mips32MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *mips32MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PMD_TRANSENTRY  *p_pmdentry,
                                              addr_t              ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiPgdNum;

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)_G_pvPTETable | (uiPgdNum * PTE_BLOCK_SIZE));

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)mips32MmuBuildPgdesc((UINT32)p_pteentry);  /*  ���ö���ҳ�����ַ  */

    return  (mips32MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: mips32MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mips32MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    (VOID)p_pteentry;
}
/*********************************************************************************************************
** ��������: mips32MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           pulPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mips32MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    UINT32   uiPFN   = (pteentry & MIPS32_ENTRYLO_PFN_MASK) >>
                        MIPS32_ENTRYLO_PFN_SHIFT;                       /*  �������ҳ���              */

    *pulPhysicalAddr = uiPFN << 12;                                     /*  ����ҳ�������ַ            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  mips32MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = mips32MmuPgdOffset(pmmuctx, ulAddr);/*  ���һ����������ַ         */

    if (mips32MmuPgdIsOk(*p_pgdentry)) {                                /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = mips32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                            ulAddr);    /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  ��ö���������              */

        if (mips32MmuPteIsOk(uiDescriptor)) {                           /*  ������������Ч              */
            ULONG    ulFlag = 0;
            UINT32   uiCache;

            if (uiDescriptor & MIPS32_ENTRYLO_V_BIT) {                  /*  ��Ч                        */
            	ulFlag |= LW_VMM_FLAG_VALID;                         	/*  ӳ����Ч                    */
            }

            ulFlag |= LW_VMM_FLAG_ACCESS;                               /*  ���Է���                    */

            if (_G_bLsHasNE) {
                if (!(uiDescriptor & MIPS32_ENTRYLO_NE_BIT)) {
                    ulFlag |= LW_VMM_FLAG_EXECABLE;                     /*  ����ִ��                    */
                }
            } else {
                ulFlag |= LW_VMM_FLAG_EXECABLE;                         /*  ����ִ��                    */
            }

            if (uiDescriptor & MIPS32_ENTRYLO_D_BIT) {                  /*  ��д                        */
                ulFlag |= LW_VMM_FLAG_WRITABLE;
            }

            uiCache = (uiDescriptor & MIPS32_ENTRYLO_C_MASK)
                       >> MIPS32_ENTRYLO_C_SHIFT;                       /*  ��� CACHE ����             */
            switch (uiCache) {

            case MIPS_UNCACHED:                                         /*  ������ CACHE                */
                break;

            case MIPS_CACHABLE_NONCOHERENT:                             /*  ���� CACHE, ��������һ����  */
                ulFlag |= LW_VMM_FLAG_CACHEABLE;                        /*  ��о 3A ��Ϊһ���� CACHE    */
                break;

            default:
                break;
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: mips32MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  mips32MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = mips32MmuPgdOffset(pmmuctx, ulAddr);/*  ���һ����������ַ         */

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (mips32MmuPgdIsOk(*p_pgdentry)) {                                /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = mips32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                            ulAddr);    /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  ��ö���������              */

        if (mips32MmuPteIsOk(uiDescriptor)) {                           /*  ������������Ч              */
            UINT32   uiPFN = (uiDescriptor & MIPS32_ENTRYLO_PFN_MASK) >>
                              MIPS32_ENTRYLO_PFN_SHIFT;                 /*  �������ҳ���              */
            addr_t   ulPhysicalAddr = uiPFN << 12;                      /*  ����ҳ�������ַ            */

            /*
             * �������������������ö���������
             */
            *p_pteentry = (LW_PTE_TRANSENTRY)mips32MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                                   ulFlag);
            return  (ERROR_NONE);

        } else {
            return  (PX_ERROR);
        }

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: mips32MmuMakeTrans
** ��������: ����ҳ��ӳ���ϵ
** �䡡��  : pmmuctx        mmu ������
**           p_pteentry     ��Ӧ��ҳ����
**           ulVirtualAddr  �����ַ
**           ulPhysicalAddr �����ַ
**           ulFlag         ��Ӧ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static VOID  mips32MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                 LW_PTE_TRANSENTRY  *p_pteentry,
                                 addr_t              ulVirtualAddr,
                                 addr_t              ulPhysicalAddr,
                                 addr_t              ulFlag)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return;
    }

    /*
     * �������������������ö���������
     */
    *p_pteentry = (LW_PTE_TRANSENTRY)mips32MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                           ulFlag);
}
/*********************************************************************************************************
** ��������: mipsMmuTlbLoadStoreExcHandle
** ��������: MMU TLB ����/�洢�쳣����
** �䡡��  : ulAbortAddr       ��ֹ��ַ
** �䡡��  : ��ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  mipsMmuTlbLoadStoreExcHandle (addr_t  ulAbortAddr)
{
    ULONG       ulAbortType  = LW_VMM_ABORT_TYPE_MAP;
    UINT32      uiEntryHiBak = mipsCp0EntryHiRead();
    UINT32      uiMask;
    UINT32      uiEntryLo;
    INT32       iIndex;
    BOOL        bIsEntryLo1;

    mipsCp0EntryHiWrite(ulAbortAddr & (LW_CFG_VMM_PAGE_MASK << 1));

    MIPS32_MMU_TLB_PROBE();

    iIndex = mipsCp0IndexRead();
    if (iIndex >= 0) {
        MIPS32_MMU_TLB_READ();

        uiMask = mipsCp0PageMaskRead() | ~(LW_CFG_VMM_PAGE_MASK << 1);

        bIsEntryLo1 = !!(ulAbortAddr & uiMask & ~(uiMask >> 1));
        if (bIsEntryLo1) {
            uiEntryLo = mipsCp0EntryLo1Read();
        } else {
            uiEntryLo = mipsCp0EntryLo0Read();
        }

        if (uiEntryLo & MIPS32_ENTRYLO_V_BIT) {
            /*
             * TLB ����ӳ����Ŀ, ����ӳ����Ч, �����ϲ�Ӧ�ó����������,
             * ������о 1B ��������ż�������(QEMU ��������),
             * ����������о 1B �������� BUG, ������Ӧ�����������
             */
            ulAbortType = 0;
        } else {
            /*
             * TLB ��Ч�쳣(��ȷ���)
             */
            ulAbortType = LW_VMM_ABORT_TYPE_MAP;
        }
    } else {
        /*
         * ��Ҫ TLB ����, ����Ƶ� TLB ������Ʋ�������ͨ���쳣���������
         */
        ULONG  ulNesting = LW_CPU_GET_CUR_NESTING();
        if (ulNesting > 1) {
        	/*
        	 * ��� TLB �����쳣�������쳣��, ֱ����ֹ
        	 */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "TLB refill error.\r\n");
            ulAbortType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
        } else {
        	/*
        	 * ��� TLB �����쳣���Ƿ����쳣��, ����֮, Ҳ������ TLB ����
             * QEMU ���� Qt ʱ�����, ��о 1B ��������������
        	 */
            ulAbortType = 0;
        }
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);

    return  (ulAbortType);
}
/*********************************************************************************************************
** ��������: mips32MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mips32MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    mipsCp0ContextWrite((UINT32)_G_pvPTETable);                         /*  �� PTE ��д�� Context �Ĵ���*/
}
/*********************************************************************************************************
** ��������: mips32MmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips32MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    REGISTER ULONG   i;

    if (ulPageNum > (MIPS32_TLB_SIZE >> 1)) {
        mips32MmuInvalidateTLB();                                       /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            mips32MmuInvalidateTLBMVA((addr_t)ulPageAddr);              /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }

        mips32MmuInvalidateMicroTLB();                                  /*  ��Ч Micro TLB              */
    }
}
/*********************************************************************************************************
** ��������: mips32MmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  mips32MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    UINT32  uiMMUSize;
    UINT32  uiConfig = mipsCp0ConfigRead();                             /*  �� Config0                  */
    UINT8   ucMT;

    ucMT = ((uiConfig & M_ConfigMT) >> S_ConfigMT);
    if (ucMT != 1) {                                                    /*  Config0 MT �� != 1��û�� MMU*/
        _DebugFormat(__PRINTMESSAGE_LEVEL, "Warning: Config register MMU type is not standard: %d!\r\n", ucMT);
    }

    if (uiConfig & (M_ConfigMore)) {                                    /*  �� Config1                  */
        uiConfig     = mipsCp0Config1Read();                            /*  �� Config1                  */
        uiMMUSize    = (uiConfig >> 25) & 0x3F;                         /*  ��� MMUSize ��             */
        _G_uiTlbSize = uiMMUSize + 1;

    } else {
        _G_uiTlbSize = 64;                                              /*  �������                    */
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s MMU TLB size = %d.\r\n", pcMachineName, MIPS32_TLB_SIZE);

    if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS1X;
        _G_bLsHasNE     = LW_TRUE;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS2X;
        _G_bLsHasITLB   = LW_TRUE;                                      /*  Loongson2x ���� ITLB        */
        _G_bLsHasNE     = LW_TRUE;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS3X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS3X;
        _G_bLsHasITLB   = LW_TRUE;                                      /*  Loongson3x ���� ITLB        */
        _G_bLsHasNE     = LW_TRUE;

        switch (mipsCp0PRIdRead() & 0xf) {

        case PRID_REV_LOONGSON3A_R2:                                    /*  Loongson3A2000              */
        case PRID_REV_LOONGSON3A_R3:                                    /*  Loongson3A3000              */
            _G_bLsHasDTLB = LW_TRUE;                                    /*  �� ITLB, DTLB, VTLB, FTLB   */
            _G_bLsHasFTLB = LW_TRUE;
            break;

        case PRID_REV_LOONGSON3A_R1:                                    /*  Loongson3A1000              */
        case PRID_REV_LOONGSON3B_R1:                                    /*  Loongson3B1000              */
        case PRID_REV_LOONGSON3B_R2:                                    /*  Loongson3B2000              */
        default:                                                        /*  ֻ�� JTLB �� ITLB           */
            break;
        }

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_JZ47XX) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_JZ47XX;
    }

#if LW_CFG_SMP_EN > 0
    pmmuop->MMUOP_ulOption = LW_VMM_MMU_FLUSH_TLB_MP;
#else
    pmmuop->MMUOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pmmuop->MMUOP_pfuncMemInit       = mips32MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = mips32MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc      = mips32MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = mips32MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = mips32MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = mips32MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = mips32MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = mips32MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk       = mips32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = mips32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = mips32MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset     = mips32MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = mips32MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = mips32MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet    = mips32MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet       = mips32MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = mips32MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = mips32MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = mips32MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = mips32MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = mips32MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = mips32MmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
