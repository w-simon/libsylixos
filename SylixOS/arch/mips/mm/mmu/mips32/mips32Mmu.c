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
2016.04.06  �޸�TLB��Ч��EntryHi Register ����(JZ4780֧��)
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
static LW_OBJECT_HANDLE     _G_hPGDPartition    = LW_HANDLE_INVALID;    /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static PVOID                _G_pvPTETable       = LW_NULL;              /*  PTE ��                      */
static UINT32               _G_uiTlbSize        = 0;                    /*  TLB �����С                */
static INT32                _G_iHwNeverExecBit  = -1;                   /*  Ӳ���Ĵ�������ִ��λ        */
static UINT32               _G_uiEntryLoPFNMask = 0;                    /*  ENTRYLO PFN ������          */
static BOOL                 _G_bIsIncITLB       = LW_FALSE;             /*  �Ƿ���Ҫ���� ITLB           */
/*********************************************************************************************************
  ENTRYLO
*********************************************************************************************************/
#define MIPS32_ENTRYLO_G_BIT            (1 << 0)
#define MIPS32_ENTRYLO_V_BIT            (1 << 1)
#define MIPS32_ENTRYLO_D_BIT            (1 << 2)

#define MIPS32_ENTRYLO_C_SHIFT          (3)
#define MIPS32_ENTRYLO_C_MASK           (0x07 << MIPS32_ENTRYLO_C_SHIFT)

#define MIPS32_ENTRYLO_PFN_DEFAULT_MASK (UINT32)(~0x3F)
#define MIPS32_ENTRYLO_PFN_MASK         _G_uiEntryLoPFNMask
#define MIPS32_ENTRYLO_PFN_SHIFT        (6)

#define MIPS32_ENTRYHI_VPN_SHIFT        (13)

#define MIPS32_PTE_EXEC_SHIFT           (0)
#define MIPS32_FLUSH_ITLB               {  mipsCp0DiagWrite(0x04);  }

#define UNIQUE_ENTRYHI(idx)             (A_K1BASE + ((idx) << (LW_CFG_VMM_PAGE_SHIFT + 1)))

#define MIPS32_TLB_4K_PAGE_SIZE_MASK   (0x00000000)
#define MIPS32_TLB_16K_PAGE_SIZE_MASK  (0x00006000)
#define MIPS32_TLB_64K_PAGE_SIZE_MASK  (0x0001E000)
#define MIPS32_TLB_256K_PAGE_SIZE_MASK (0x0007E000)
#define MIPS32_TLB_1M_PAGE_SIZE_MASK   (0x001FE000)
#define MIPS32_TLB_4M_PAGE_SIZE_MASK   (0x007FE000)
#define MIPS32_TLB_16M_PAGE_SIZE_MASK  (0x01FFE000)
#define MIPS32_TLB_64M_PAGE_SIZE_MASK  (0x07FFE000)
#define MIPS32_TLB_256M_PAGE_SIZE_MASK (0x1FFFE000)
/*********************************************************************************************************
  TLB ����
*********************************************************************************************************/
#define MIPS_MMU_TLB_WRITE()            MIPS_EXEC_INS("TLBWI"); MIPS_EXEC_INS ("EHB")
#define MIPS_MMU_TLB_READ()             MIPS_EXEC_INS("TLBR");  MIPS_EXEC_INS ("EHB")
#define MIPS_MMU_TLB_PROBE()            MIPS_EXEC_INS("TLBP");  MIPS_EXEC_INS ("EHB")
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
*********************************************************************************************************/
static VOID  mips32MmuInvalidateTLB (VOID)
{
    UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
    INT     i;

    for (i = 0; i < _G_uiTlbSize; i++) {
        mipsCp0IndexWrite(i);
        mipsCp0EntryLo0Write(0);
        mipsCp0EntryLo1Write(0);
        mipsCp0EntryHiWrite(UNIQUE_ENTRYHI(i));
        mipsCp0PageMaskWrite(MIPS32_TLB_4K_PAGE_SIZE_MASK);

        MIPS_MMU_TLB_WRITE();
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);

    if (_G_bIsIncITLB) {
        MIPS32_FLUSH_ITLB;
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
    INT     i;

    for (i = 0; i < _G_uiTlbSize; i++) {
        mipsCp0IndexWrite(i);

        MIPS_MMU_TLB_READ();

        uiEntryLo0 = mipsCp0EntryLo0Read();
        uiEntryLo1 = mipsCp0EntryLo1Read();
        uiEntryHi  = mipsCp0EntryHiRead();

        _PrintFormat("TLB[%02d]: uiEntryLo0=0x%08x, uiEntryLo1=0x%08x, uiEntryHi=0x%08x\r\n",
                     i, uiEntryLo0, uiEntryLo1, uiEntryHi);
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);

    if (_G_bIsIncITLB) {
        MIPS32_FLUSH_ITLB;
    }
}
/*********************************************************************************************************
** ��������: mips32MmuInvalidateTLBMVA
** ��������: ��Чָ�� MVA �� TLB
** �䡡��  : ulAddr            ָ�� MVA
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mips32MmuInvalidateTLBMVA (addr_t  ulAddr)
{
    UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
    INT32   iIndex;

    while (1) {
        mipsCp0EntryHiWrite((ulAddr >> (LW_CFG_VMM_PAGE_SHIFT + 1)) << MIPS32_ENTRYHI_VPN_SHIFT);

        MIPS_MMU_TLB_PROBE();

        iIndex = mipsCp0IndexRead();
        if (iIndex >= 0) {
            mipsCp0IndexWrite(iIndex);

            mipsCp0EntryLo0Write(0);
            mipsCp0EntryLo1Write(0);
            mipsCp0EntryHiWrite(UNIQUE_ENTRYHI(iIndex));

            MIPS_MMU_TLB_WRITE();
        } else {
            break;
        }
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);

    if (_G_bIsIncITLB) {
        MIPS32_FLUSH_ITLB;
    }
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
        uiPFN = uiBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;                    /*  ���� PFN                    */

        uiDescriptor = uiPFN << MIPS32_ENTRYLO_PFN_SHIFT;               /*  ��� PFN                    */

        if (ulFlag & LW_VMM_FLAG_VALID) {
            uiDescriptor |= MIPS32_ENTRYLO_V_BIT;                       /*  ��� V λ                   */
        }

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            uiDescriptor |= MIPS32_ENTRYLO_D_BIT;                       /*  ��� D λ                   */
        }

        uiDescriptor |= MIPS32_ENTRYLO_G_BIT;                           /*  ��� G λ                   */

        if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                           /*  ��� C λ                   */
            uiDescriptor |= MIPS_CACHABLE_NONCOHERENT << MIPS32_ENTRYLO_C_SHIFT;
        }

        if (!(ulFlag & LW_VMM_FLAG_EXECABLE)) {
            if (_G_iHwNeverExecBit >= 0) {
                uiDescriptor |= 1 << _G_iHwNeverExecBit;
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
#define PGD_BLOCK_SIZE  (16 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  ( 1 * LW_CFG_KB_SIZE)

    PVOID   pvPgdTable;
    PVOID   pvPteTable;
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN(PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);

    /*
     * PTE ���СΪ 4MByte����Ҫ 8MByte �������д�� Context �Ĵ���
     */
    pvPteTable = __KHEAP_ALLOC_ALIGN(4 * LW_CFG_MB_SIZE, 8 * LW_CFG_MB_SIZE);
    
    if (!pvPgdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }
    
    lib_bzero(pvPteTable, 4 * LW_CFG_MB_SIZE);

    _G_pvPTETable = pvPteTable;

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, 1, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_hPGDPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }
    
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
    UINT32  uiMMUSize;
    UINT32  uiConfig = mipsCp0ConfigRead();                             /*  �� Config0                  */

    if (((uiConfig & M_ConfigMT) >> S_ConfigMT) != 1) {                 /*  Config0 MT �� != 1��û�� MMU*/
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No MMU!\r\n");
        return  (PX_ERROR);
    }

    if (uiConfig & (M_ConfigMore)) {                                    /*  �� Config1                  */
        uiConfig     = mipsCp0Config1Read();                            /*  �� Config1                  */
        uiMMUSize    = (uiConfig >> 25) & 0x3F;                         /*  ��� MMUSize ��             */
        _G_uiTlbSize = uiMMUSize + 1;

    } else {
        _G_uiTlbSize = 64;                                              /*  �������                    */
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "MMU TLB size = %d.\r\n", _G_uiTlbSize);

    archCacheReset(pcMachineName);                                      /*  ��λ Cache                  */
    
    mips32MmuInvalidateTLB();                                           /*  ��Ч TLB                    */
    mipsCp0EntryHiWrite(0);                                             /*  ASID = 0                    */
    mipsCp0PageMaskWrite(MIPS32_TLB_4K_PAGE_SIZE_MASK);                 /*  4K ҳ���С                 */
    mipsCp0WiredWrite(0);                                               /*  ȫ����������滻            */

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
    
    /*
     * ulAddr >> LW_CFG_VMM_PGD_SHIFT ���� PGD �ţ�
     *
     * �� LW_PGD_TRANSENTRY �Ĵ�СΪ 4
     */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) * 4));               /*  ���һ��ҳ����������ַ      */
               
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

    uiTemp = (UINT32)(*p_pmdentry);                                     /*  ���һ��ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp);                         /*  ��ö���ҳ�����ַ          */

    /*
     * ulAddr >> (LW_CFG_VMM_PAGE_SHIFT - 2) �൱�� (ulAddr >> LW_CFG_VMM_PAGE_SHIFT) * 4
     *
     * ulAddr >> LW_CFG_VMM_PAGE_SHIFT ȡ��ҳ�ţ�ȫ�֣��Ƕ��ڣ�
     *
     * �� LW_PTE_TRANSENTRY �Ĵ�СΪ 4
     *
     * & 0x3FC == & 0b1111111100, ����ҳ��ռ 8 λ��LW_CFG_VMM_PGD_SHIFT - LW_CFG_VMM_PAGE_SHIFT
     * һ���� 256 ��ҳ�棩
     */
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry
               | (((ulAddr >> (LW_CFG_VMM_PAGE_SHIFT - 2)) & 0x3FC)));  /*  ��������ַҳ����������ַ  */
    
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
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  ��Чһ��ҳ����              */

    /*
     * p_pgdentry >> LW_CFG_VMM_PGD_SHIFT ���� PGD �ţ�
     *
     * �� LW_PGD_TRANSENTRY �Ĵ�СΪ 4
     */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) << 2));              /*  ���һ��ҳ����������ַ      */
               
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
    /*
     * �ͷ�ʱ���뵽 PGD_BLOCK_SIZE����Ϊ����ʱ���뵽 PGD_BLOCK_SIZE
     */
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
    LW_PTE_TRANSENTRY  *p_pteentry;

    /*
     * ulAddr >> LW_CFG_VMM_PGD_SHIFT ���� PGD ��
     * ÿ PGD һ������ҳ��
     * ����ҳ��Ĵ�СΪ PTE_BLOCK_SIZE
     */
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)_G_pvPTETable
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) * PTE_BLOCK_SIZE));

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
    UINT32   uiPFN   = (pteentry & MIPS32_ENTRYLO_PFN_MASK)
                       >> MIPS32_ENTRYLO_PFN_SHIFT;                     /*  �������ҳ���              */

    *pulPhysicalAddr = uiPFN << LW_CFG_VMM_PAGE_SHIFT;                  /*  ����ҳ�������ַ            */

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

            if (_G_iHwNeverExecBit >= 0) {
                if (!(uiDescriptor & (1 << _G_iHwNeverExecBit))) {
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
                ulFlag |= LW_VMM_FLAG_CACHEABLE;
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
            UINT32   uiPFN = (uiDescriptor & MIPS32_ENTRYLO_PFN_MASK)
                              >> MIPS32_ENTRYLO_PFN_SHIFT;              /*  �������ҳ��                */
            addr_t   ulPhysicalAddr = uiPFN << LW_CFG_VMM_PAGE_SHIFT;   /*  ����ҳ�������ַ            */

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
    UINT32      uiEntryHiBak = mipsCp0EntryHiRead();
    INT32       iIndex;
    UINT32      uiEntryLo;
    ULONG       ulAbortType = LW_VMM_ABORT_TYPE_MAP;

    mipsCp0EntryHiWrite((ulAbortAddr >> (LW_CFG_VMM_PAGE_SHIFT + 1)) << MIPS32_ENTRYHI_VPN_SHIFT);

    MIPS_MMU_TLB_PROBE();

    iIndex = mipsCp0IndexRead();
    if (iIndex >= 0) {
        MIPS_MMU_TLB_READ();

        if ((ulAbortAddr >> LW_CFG_VMM_PAGE_SHIFT) % 2) {
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
            ulAbortType = LW_VMM_ABORT_TYPE_TERMINAL;
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
    ULONG   i;

    if (ulPageNum > 16) {
        mips32MmuInvalidateTLB();                                       /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            mips32MmuInvalidateTLBMVA((addr_t)ulPageAddr);              /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
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
    if (lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X) == 0) {
        _G_iHwNeverExecBit  = 30;
        _G_uiEntryLoPFNMask = MIPS32_ENTRYLO_PFN_DEFAULT_MASK & (~(0x3 << 30));

    } else {
        if (lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X) == 0) {
            _G_bIsIncITLB = LW_TRUE;
        }

        _G_iHwNeverExecBit  = -1;
        _G_uiEntryLoPFNMask = MIPS32_ENTRYLO_PFN_DEFAULT_MASK;
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
