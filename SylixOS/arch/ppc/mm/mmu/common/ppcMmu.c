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
** ��   ��   ��: ppcMmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 01 �� 14 ��
**
** ��        ��: PowerPC ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "./ppcMmuHashPageTbl.h"
/*********************************************************************************************************
  һ�����������Ͷ���
*********************************************************************************************************/
#define COARSE_TBASE        (1)                                         /*  ����������ҳ�����ַ        */
/*********************************************************************************************************
  �������������Ͷ���
*********************************************************************************************************/
#define FAIL_DESC           (0)                                         /*  �任ʧЧ                    */
#define SMALLPAGE_DESC      (2)                                         /*  Сҳ����ַ                  */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE ������                  */
static PLW_MMU_CONTEXT      _G_pMmuContext[LW_CFG_MAX_PROCESSORS];      /*  ������                      */
static UINT                 _G_uiTlbNr;                                 /*  TLB ��Ŀ                    */
static UINT8                _G_ucWIM4CacheBuffer;
static UINT8                _G_ucWIM4Cache;
/*********************************************************************************************************
  ����Ȩ�� PP
*********************************************************************************************************/
#define PP_RW               (2)
#define PP_RO               (1)
#define PP_NORW             (0)
/*********************************************************************************************************
  WIMG
*********************************************************************************************************/
#define G_BIT               (1 << 0)
#define M_BIT               (1 << 1)
#define I_BIT               (1 << 2)
#define W_BIT               (1 << 3)

#define WIM_MASK            (W_BIT | I_BIT | M_BIT)
/*********************************************************************************************************
    OS Two-Level page table:

        L1 page table
                             L2 page table
        +-----------+                                Phy page
        | PGD ENTRY | -----> +-----------+
        +-----------+        | PTE ENTRY | -----> +-----------+
        |           |        +-----------+        |           |
        |           |        |           |        |           |
        |   4096    |        |    256    |        |           |
        |           |        |           |        |           |
        |           |        +-----------+        +-----------+
        +-----------+

    L1 page table size: 4GB / 1MB * sizeof(PGD ENTRY) = 4096 * 4 = 16 KB
    L2 page table size: 1MB / sizeof(PAGE) * size(PTE ENTRY) = 256 * 4 = 1 KB
*********************************************************************************************************/
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID  ppcMmuInvalidateTLBNr(UINT  uiTlbNr);
extern VOID  ppcMmuInvalidateTLBEA(UINT32  uiEffectiveAddr);
extern VOID  ppcMmuEnable(VOID);
extern VOID  ppcMmuDisable(VOID);
/*********************************************************************************************************
** ��������: ppcMmuInvalidateTLBNr
** ��������: ��Ч���е� TBL
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  VOID  ppcMmuInvalidateTLB (VOID)
{
    ppcMmuInvalidateTLBNr(_G_uiTlbNr);
}
/*********************************************************************************************************
** ��������: ppcMmuFlags2Attr
** ��������: ���� SylixOS Ȩ�ޱ�־, ���� MPC32 MMU Ȩ�ޱ�־
** �䡡��  : ulFlag                  �ڴ����Ȩ��
**           pucPP                   ����Ȩ��
**           pucWIMG                 CACHE ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcMmuFlags2Attr (ULONG  ulFlag, UINT8  *pucPP, UINT8  *pucWIMG,  UINT8  *pucExec)
{
    UINT8  ucWIMG;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  �Ƿ�ӵ�з���Ȩ��            */
        if (ulFlag & LW_VMM_FLAG_WRITABLE) {                            /*  �Ƿ��д                    */
            *pucPP = PP_RW;

        } else {
            *pucPP = PP_RO;
        }
    } else {
        *pucPP = PP_NORW;
    }

    if ((ulFlag & LW_VMM_FLAG_CACHEABLE) &&
        (ulFlag & LW_VMM_FLAG_BUFFERABLE)) {                            /*  CACHE �� BUFFER ����        */
        ucWIMG = _G_ucWIM4CacheBuffer;

    } else if (ulFlag & LW_VMM_FLAG_CACHEABLE) {
        ucWIMG = _G_ucWIM4Cache;

    } else {
        ucWIMG = I_BIT | M_BIT;
    }

    *pucWIMG = ucWIMG;

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        *pucExec = 1;
    } else {
        *pucExec = 0;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcMmuAttr2Flags
** ��������: ���� MPC32 MMU Ȩ�ޱ�־, ���� SylixOS Ȩ�ޱ�־
** �䡡��  : ucPP                    ����Ȩ��
**           ucWIMG                  CACHE ���Ʋ���
**           pulFlag                 �ڴ����Ȩ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcMmuAttr2Flags (UINT8  ucPP, UINT8  ucWIMG, UINT8  ucExec, ULONG *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    switch (ucPP) {

    case PP_RW:
        *pulFlag |= LW_VMM_FLAG_ACCESS;
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
        break;

    case PP_RO:
        *pulFlag |= LW_VMM_FLAG_ACCESS;
        break;

    case PP_NORW:
    default:
        break;
    }

    if ((ucWIMG & WIM_MASK) == _G_ucWIM4CacheBuffer) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_BUFFERABLE;

    } else if ((ucWIMG & WIM_MASK) == _G_ucWIM4CacheBuffer) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;
    }

    *pulFlag |= LW_VMM_FLAG_GUARDED;

    if (ucExec) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcMmuBuildPgdesc
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : uiBaseAddr              ����ַ     (�λ���ַ������ҳ�����ַ)
**           ucPP                    ����Ȩ��
**           ucWIMG                  CACHE ���Ʋ���
**           ucType                  һ������������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppcMmuBuildPgdesc (UINT32  uiBaseAddr,
                                             UINT8   ucPP,
                                             UINT8   ucWIMG,
                                             UINT8   ucExec,
                                             UINT8   ucType)
{
    LW_PGD_TRANSENTRY   uiPgdDescriptor;

    switch (ucType) {

    case COARSE_TBASE:                                                  /*  �����ȶ���ҳ��������        */
        uiPgdDescriptor = (uiBaseAddr & 0xFFFFFC00)
                         | ucType;
        break;

    default:
        uiPgdDescriptor = 0;                                            /*  ����ʧЧ                    */
        break;
    }

    return  (uiPgdDescriptor);
}
/*********************************************************************************************************
** ��������: ppcMmuBuildPtentry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : uiBaseAddr              ����ַ     (ҳ��ַ)
**           ucPP                    ����Ȩ��
**           ucWIMG                  CACHE ���Ʋ���
**           ucType                  ��������������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppcMmuBuildPtentry (UINT32  uiBaseAddr,
                                              UINT8   ucPP,
                                              UINT8   ucWIMG,
                                              UINT8   ucExec,
                                              UINT8   ucType)
{
    LW_PTE_TRANSENTRY   uiPteDescriptor;

    switch (ucType) {

    case SMALLPAGE_DESC:                                                /*  Сҳ������                  */
        /*
         * ����� R �� C λ����Ϊ 0����Ϊ������ PTE word1 �󣬻���Ч TLB��
         * ���Բ��õ��� PTE ���Ӧ�� TLB ��һ�µ�����
         */
        uiPteDescriptor.PTE_bRef        = 0;
        uiPteDescriptor.PTE_bChange     = 0;
        uiPteDescriptor.PTE_ucPP        = ucPP;
        uiPteDescriptor.PTE_ucWIMG      = ucWIMG;
        uiPteDescriptor.PTE_uiRPN       = uiBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;
        uiPteDescriptor.PTE_ucReserved1 = ucType;
        uiPteDescriptor.PTE_bReserved2  = ucExec;
        break;

    default:
        uiPteDescriptor.PTE_uiValue = 0;                                /*  ����ʧЧ                    */
        break;
    }

    return  (uiPteDescriptor);
}
/*********************************************************************************************************
** ��������: ppcMmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : һ��ҳ�����ַ��Ҫ���� 16 KByte ����, ����Ŀӳ�� 1 MByte �ռ�.
             ����ҳ�����ַ��Ҫ����  1 KByte ����, ����Ŀӳ�� 4 KByte �ռ�.
*********************************************************************************************************/
static INT  ppcMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (16 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  ( 1 * LW_CFG_KB_SIZE)

    PVOID   pvPgdTable;
    PVOID   pvPteTable;

    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    ULONG   ulPteNum = bspMmuPteMaxNum();

    INT     iError;

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

    iError = ppcMmuHashPageTblInit(MMU_PTE_MIN_SIZE_128M);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not init hashed page table.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcMmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcMmuGlobalInit (CPCHAR  pcMachineName)
{
    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  �� Core0 ��λ Cache         */
        archCacheReset(pcMachineName);                                  /*  ��λ Cache                  */
    }

    ppcMmuInvalidateTLB();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcMmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *ppcMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) << 2));              /*  ���һ��ҳ����������ַ      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcMmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *ppcMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  �� PMD ��                   */
}
/*********************************************************************************************************
** ��������: ppcMmuPteOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *ppcMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;

    uiTemp = (UINT32)(*p_pmdentry);                                     /*  ��ö���ҳ��������          */

    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp & (~(LW_CFG_KB_SIZE - 1)));
                                                                        /*  ��ô����ȶ���ҳ�����ַ    */
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry
               | ((ulAddr >> 10) & 0x3FC));                             /*  ��ö�Ӧ�����ַҳ��������  */
                                                                        /*  ��ַ                        */
    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: ppcMmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  ppcMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (((pgdentry & 0x03) == COARSE_TBASE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: ppcMmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  ppcMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (((pteentry.PTE_ucReserved1 & 0x03) == SMALLPAGE_DESC) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: ppcMmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *ppcMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);

    if (!p_pgdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  �µ� PGD ����Ч��ҳ����     */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) << 2));              /*  pgd offset                  */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcMmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~((16 * LW_CFG_KB_SIZE) - 1)));

    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcMmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *ppcMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                          LW_PGD_TRANSENTRY  *p_pgdentry,
                                          addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcMmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: ppcMmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���:
** ����ģ��: SylixOS ӳ����ϻ��Զ�����, �������ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *ppcMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PMD_TRANSENTRY  *p_pmdentry,
                                           addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)ppcMmuBuildPgdesc((UINT32)p_pteentry,
                                                       PP_RW,
                                                       0,
                                                       1,
                                                       COARSE_TBASE);   /*  ���ö���ҳ�����ַ          */

    return  (ppcMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: ppcMmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(LW_CFG_KB_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** ��������: ppcMmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           pulPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    *pulPhysicalAddr = (addr_t)(pteentry.PTE_uiRPN << LW_CFG_VMM_PAGE_SHIFT);   /*  ��������ַ        */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcMmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  ppcMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcMmuPgdOffset(pmmuctx, ulAddr);
    INT                 iDescType;
    UINT8               ucPP;                                           /*  �洢Ȩ��                    */
    UINT8               ucWIMG;                                         /*  CACHE �뻺��������          */
    UINT8               ucExec;
    ULONG               ulFlag;

    iDescType = (*p_pgdentry) & 0x03;                                   /*  ���һ��ҳ������            */
    if (iDescType == COARSE_TBASE) {                                    /*  ���ڴ����ȶ���ҳ��ӳ��      */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);

        if (ppcMmuPteIsOk(*p_pteentry)) {
            ucPP    = p_pteentry->PTE_ucPP;
            ucWIMG  = p_pteentry->PTE_ucWIMG;
            ucExec  = p_pteentry->PTE_bReserved2;

            ppcMmuAttr2Flags(ucPP, ucWIMG, ucExec, &ulFlag);
            return  (ulFlag);

        } else {
            return  (LW_VMM_FLAG_UNVALID);
        }
    } else {
        return  (LW_VMM_FLAG_UNVALID);
    }
}
/*********************************************************************************************************
** ��������: ppcMmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���ڸı��˵���Ŀ��ҳ��, VMM �������������������Ч���, ����������Ҫ��Чָ����Ŀ�Ŀ��.
*********************************************************************************************************/
static INT  ppcMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcMmuPgdOffset(pmmuctx, ulAddr);
    INT                 iDescType;
    UINT8               ucPP;                                           /*  �洢Ȩ��                    */
    UINT8               ucWIMG;                                         /*  CACHE �뻺��������          */
    UINT8               ucExec;
    UINT8               ucType;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;

    } else {
        ucType = FAIL_DESC;                                             /*  ���ʽ�ʧЧ                  */
    }

    if (ppcMmuFlags2Attr(ulFlag, &ucPP, &ucWIMG, &ucExec) < 0) {        /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    iDescType = (*p_pgdentry) & 0x03;                                   /*  ���һ��ҳ������            */
    if (iDescType == COARSE_TBASE) {                                    /*  ���ڴ����ȶ���ҳ��ӳ��      */
        REGISTER LW_PTE_TRANSENTRY  *p_pteentry = ppcMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                                     ulAddr);
        if (ppcMmuPteIsOk(*p_pteentry)) {
            addr_t   ulPhysicalAddr = (addr_t)(p_pteentry->PTE_uiRPN << LW_CFG_VMM_PAGE_SHIFT);

            *p_pteentry = ppcMmuBuildPtentry((UINT32)ulPhysicalAddr,
                                             ucPP, ucWIMG, ucExec, ucType);
            ppcMmuHashPageTblFlagSet(ulAddr,
                                     p_pteentry->PTE_uiValue);

            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ppcMmuMakeTrans
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
static VOID  ppcMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              phys_addr_t         paPhysicalAddr,
                              ULONG               ulFlag)
{
    UINT8   ucPP;                                                       /*  �洢Ȩ��                    */
    UINT8   ucWIMG;                                                     /*  CACHE �뻺��������          */
    UINT8   ucExec;
    UINT8   ucType;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;

    } else {
        ucType = FAIL_DESC;                                             /*  ���ʽ�ʧЧ                  */
    }

    if (ppcMmuFlags2Attr(ulFlag, &ucPP, &ucWIMG, &ucExec) < 0) {        /*  ��Ч��ӳ���ϵ              */
        return;
    }

    *p_pteentry = ppcMmuBuildPtentry((UINT32)paPhysicalAddr,
                                     ucPP, ucWIMG, ucExec, ucType);

    ppcMmuHashPageTblMakeTrans(ulVirtualAddr,
                               p_pteentry->PTE_uiValue);
}
/*********************************************************************************************************
** ��������: ppcMmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ULONG   ulCPUId = LW_CPU_GET_CUR_ID();

    _G_pMmuContext[ulCPUId] = pmmuctx;
}
/*********************************************************************************************************
** ��������: ppcMmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > (_G_uiTlbNr >> 1)) {
        ppcMmuInvalidateTLB();                                          /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            ppcMmuInvalidateTLBEA((UINT32)ulPageAddr);                  /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: ppcMmuPteMissHandle
** ��������: ���� PTE ƥ��ʧ���쳣
** �䡡��  : ulAddr        �쳣����
** �䡡��  : ��ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  ppcMmuPteMissHandle (addr_t  ulAddr)
{
    ULONG               ulCPUId = LW_CPU_GET_CUR_ID();
    LW_PGD_TRANSENTRY  *p_pgdentry;
    INT                 iDescType;

    if (!_G_pMmuContext[ulCPUId]) {
        return  (LW_VMM_ABORT_TYPE_MAP);
    }

    p_pgdentry = ppcMmuPgdOffset(_G_pMmuContext[ulCPUId], ulAddr);
    iDescType  = (*p_pgdentry) & 0x03;                                  /*  ���һ��ҳ������            */
    if (iDescType == COARSE_TBASE) {                                    /*  ���ڴ����ȶ���ҳ��ӳ��      */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);

        if (ppcMmuPteIsOk(*p_pteentry)) {
            ppcMmuHashPageTblPteMiss(ulAddr, p_pteentry->PTE_uiValue);
            return  (0);

        } else {
            return  (LW_VMM_ABORT_TYPE_MAP);
        }
    } else {
        return  (LW_VMM_ABORT_TYPE_MAP);
    }
}
/*********************************************************************************************************
** ��������: ppcMmuPtePreLoad
** ��������: PTE Ԥ����
** �䡡��  : ulAddr        ���ݷ��ʵ�ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ppcMmuPtePreLoad (addr_t  ulAddr)
{
    ULONG               ulCPUId = LW_CPU_GET_CUR_ID();
    LW_PGD_TRANSENTRY  *p_pgdentry;
    INT                 iDescType;

    if (!_G_pMmuContext[ulCPUId]) {
        return  (PX_ERROR);
    }

    p_pgdentry = ppcMmuPgdOffset(_G_pMmuContext[ulCPUId], ulAddr);
    iDescType  = (*p_pgdentry) & 0x03;                                  /*  ���һ��ҳ������            */
    if (iDescType == COARSE_TBASE) {                                    /*  ���ڴ����ȶ���ҳ��ӳ��      */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);

        if (ppcMmuPteIsOk(*p_pteentry)) {
            ppcMmuHashPageTblPtePreLoad(ulAddr, p_pteentry->PTE_uiValue);
            return  (ERROR_NONE);

        } else {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: bspMmuTlbSize
** ��������: ��� TLB ����Ŀ
** �䡡��  : NONE
** �䡡��  : TLB ����Ŀ
** ȫ�ֱ���:
** ����ģ��:
**
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuTlbSize (VOID)
{
    return  (128);                                                      /*  128 �ʺ� 750 MPC83XX ����   */
}
/*********************************************************************************************************
** ��������: ppcMmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcMmuInit (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    _G_uiTlbNr           = bspMmuTlbSize();                             /*  ��� TLB ����Ŀ             */
    _G_ucWIM4CacheBuffer = M_BIT;
    _G_ucWIM4Cache       = W_BIT | M_BIT;

    pmmuop->MMUOP_ulOption           = 0ul;                             /*  tlbsync ָ����Զ����ͬ��  */
    pmmuop->MMUOP_pfuncMemInit       = ppcMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = ppcMmuGlobalInit;

    pmmuop->MMUOP_pfuncPGDAlloc      = ppcMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = ppcMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = ppcMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = ppcMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = ppcMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = ppcMmuPteFree;

    pmmuop->MMUOP_pfuncPGDIsOk       = ppcMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = ppcMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = ppcMmuPteIsOk;

    pmmuop->MMUOP_pfuncPGDOffset     = ppcMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = ppcMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = ppcMmuPteOffset;

    pmmuop->MMUOP_pfuncPTEPhysGet    = ppcMmuPtePhysGet;

    pmmuop->MMUOP_pfuncFlagGet       = ppcMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = ppcMmuFlagSet;

    pmmuop->MMUOP_pfuncMakeTrans     = ppcMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = ppcMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = ppcMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = ppcMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = ppcMmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
