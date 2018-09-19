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
** ��   ��   ��: ppcE500Mmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 04 ��
**
** ��        ��: PowerPC E500 ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/ppcSpr.h"
#include "arch/ppc/common/e500/ppcSprE500.h"
#include "./ppcMmuE500Reg.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition = LW_HANDLE_INVALID;       /*  ϵͳĿǰ��ʹ��һ�� PGD      */
static addr_t               _G_ulPTETable    = 0;                       /*  PTE ��                      */
static UINT                 _G_uiTlbSize     = 0;                       /*  TLB �����С                */
static BOOL                 _G_bMas2MBit     = LW_FALSE;                /*  ���һ����                  */
static BOOL                 _G_bHasMAS7      = LW_FALSE;                /*  �Ƿ��� MAS7 �Ĵ���          */
static BOOL                 _G_bHasHID1      = LW_FALSE;                /*  �Ƿ��� HID1 �Ĵ���          */
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define MMU_MAS2_M          _G_bMas2MBit                                /*  �Ƿ���һ����              */
#define MMU_MAS4_X0D        0                                           /*  Implement depend page attr  */
#define MMU_MAS4_X1D        0                                           /*  Implement depend page attr  */
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
extern INT     ppcCacheDataUpdate(PVOID  pvAdrs, size_t  stBytes, BOOL  bInv);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

extern VOID    ppcE500MmuInvalidateTLB(VOID);
extern VOID    ppcE500MmuInvalidateTLBEA(addr_t  ulAddr);
/*********************************************************************************************************
** ��������: ppcE500MmuDataStorageAbortType
** ��������: ���ݷ�����ֹ����
** �䡡��  : ulAbortAddr   ��ֹ��ַ
**           bIsWrite      �Ƿ�洢�������쳣
** �䡡��  : ���ݷ�����ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  ppcE500MmuDataStorageAbortType (addr_t  ulAbortAddr, BOOL  bIsWrite)
{
    addr_t              uiEPN      = ulAbortAddr >> LW_CFG_VMM_PAGE_SHIFT;
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + \
                                     uiEPN * sizeof(LW_PTE_TRANSENTRY));
    LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;

    if (pteentry.MAS3_bValid && pteentry.MAS3_uiRPN) {
        if (bIsWrite) {
            if (!pteentry.MAS3_bSuperWrite) {
                return  (LW_VMM_ABORT_TYPE_PERM);
            }
        } else {
            if (!pteentry.MAS3_bSuperRead) {
                return  (LW_VMM_ABORT_TYPE_PERM);
            }
        }
        return  (LW_VMM_ABORT_TYPE_MAP);

    } else {
        return  (LW_VMM_ABORT_TYPE_MAP);
    }
}
/*********************************************************************************************************
** ��������: ppcE500MmuInstStorageAbortType
** ��������: ָ�������ֹ����
** �䡡��  : ulAbortAddr   ��ֹ��ַ
** �䡡��  : ָ�������ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  ppcE500MmuInstStorageAbortType (addr_t  ulAbortAddr)
{
    addr_t              uiEPN      = ulAbortAddr >> LW_CFG_VMM_PAGE_SHIFT;
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + \
                                     uiEPN * sizeof(LW_PTE_TRANSENTRY));
    LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;

    if (pteentry.MAS3_bValid && pteentry.MAS3_uiRPN) {
        if (!pteentry.MAS3_bSuperExec) {
            return  (LW_VMM_ABORT_TYPE_PERM);
        }
        return  (LW_VMM_ABORT_TYPE_MAP);

    } else {
        return  (LW_VMM_ABORT_TYPE_MAP);
    }
}
/*********************************************************************************************************
** ��������: ppcE500MmuEnable
** ��������: ʹ�� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcE500MmuEnable (VOID)
{
    /*
     * E500 ����ʹ�� MMU
     */
}
/*********************************************************************************************************
** ��������: ppcE500MmuDisable
** ��������: ���� MMU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcE500MmuDisable (VOID)
{
    /*
     * E500 ����ʹ�� MMU
     */
}
/*********************************************************************************************************
** ��������: ppcE500MmuBuildPgdesc
** ��������: ����һ��һ�������� (PGD ������)
** �䡡��  : uiBaseAddr              ����ҳ�����ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppcE500MmuBuildPgdesc (UINT32  uiBaseAddr)
{
    return  (uiBaseAddr);                                               /*  һ�����������Ƕ���ҳ�����ַ*/
}
/*********************************************************************************************************
** ��������: ppcE500MmuBuildPtentry
** ��������: ����һ������������ (PTE ������)
** �䡡��  : paBaseAddr              ����ҳ��ַ
**           uiFlag
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppcE500MmuBuildPtentry (phys_addr_t  paBaseAddr,
                                                  ULONG        ulFlag)
{
    LW_PTE_TRANSENTRY   uiDescriptor;
    UINT32              uiRPN;

    uiDescriptor.MAS3_uiValue = 0;
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
    uiDescriptor.MAS7_uiValue = 0;
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        uiRPN = paBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;                    /*  ���� RPN                    */

        uiDescriptor.MAS3_uiRPN = uiRPN & 0xfffff;                      /*  ��� RPN                    */

        if (ulFlag & LW_VMM_FLAG_VALID) {
            uiDescriptor.MAS3_bValid = LW_TRUE;                         /*  ��Ч                        */
        }

        uiDescriptor.MAS3_bSuperRead = LW_TRUE;                         /*  �ɶ�                        */

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            uiDescriptor.MAS3_bSuperWrite = LW_TRUE;                    /*  ��д                        */
        }

        if (ulFlag & LW_VMM_FLAG_EXECABLE) {
            uiDescriptor.MAS3_bSuperExec = LW_TRUE;                     /*  ��ִ��                      */
        }

        if (!(ulFlag & LW_VMM_FLAG_CACHEABLE)) {
            uiDescriptor.MAS3_bUnCache = LW_TRUE;                       /*  ���� Cache                  */
        }

        if ((ulFlag  & LW_VMM_FLAG_CACHEABLE) &&
            !(ulFlag & LW_VMM_FLAG_BUFFERABLE)) {
            uiDescriptor.MAS3_bWT = LW_TRUE;                            /*  д��͸                      */
        }

        if (MMU_MAS2_M) {
            uiDescriptor.MAS3_bMemCoh = LW_TRUE;                        /*  ���һ����                  */
        }

#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
        uiDescriptor.MAS7_uiHigh4RPN = uiRPN >> 20;
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/
    }

    return  (uiDescriptor);
}
/*********************************************************************************************************
** ��������: ppcE500MmuMemInit
** ��������: ��ʼ�� MMU ҳ���ڴ���
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ppcE500MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
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

    pvPteTable = __KHEAP_ALLOC(PTE_TABLE_SIZE);
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

    _G_ulPTETable = (addr_t)pvPteTable;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcE500MmuGlobalInit
** ��������: ���� BSP �� MMU ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ppcE500MmuGlobalInit (CPCHAR  pcMachineName)
{
    MMUCFG_REG  uiMMUCFG;
    MAS4_REG    uiMAS4;
    UINT32      uiHID1;

    /*
     * ���� PID
     */
    uiMMUCFG.MMUCFG_uiValue = ppcE500MmuGetMMUCFG();
    ppcE500MmuSetPID0(0);
    if (uiMMUCFG.MMUCFG_ucNPIDS > 1) {
        ppcE500MmuSetPID1(0);
        if (uiMMUCFG.MMUCFG_ucNPIDS > 2) {
            ppcE500MmuSetPID2(0);
        }
    }

    /*
     * ���� MAS4
     */
    uiMAS4.MAS4_uiValue   = 0;
    uiMAS4.MAS4_bTLBSELD  = 0;
    uiMAS4.MAS4_ucTIDSELD = 0;
    uiMAS4.MAS4_ucTSIZED  = MMU_TRANS_SZ_4K;
    uiMAS4.MAS4_bX0D      = MMU_MAS4_X0D;
    uiMAS4.MAS4_bX1D      = MMU_MAS4_X1D;
    uiMAS4.MAS4_bWD       = LW_FALSE;
    uiMAS4.MAS4_bID       = LW_TRUE;
    uiMAS4.MAS4_bMD       = LW_TRUE;
    uiMAS4.MAS4_bGD       = LW_FALSE;
    uiMAS4.MAS4_bED       = LW_FALSE;

    ppcE500MmuSetMAS4(uiMAS4.MAS4_uiValue);

    /*
     * ʹ�ܵ�ַ�㲥
     */
    if (_G_bHasHID1) {
        uiHID1 = ppcE500GetHID1();
        if (MMU_MAS2_M) {
            uiHID1 |=  ARCH_PPC_HID1_ABE;
        } else {
            uiHID1 &= ~ARCH_PPC_HID1_ABE;
        }
        ppcE500SetHID1(uiHID1);
    }

    /*
     * �� MAS7 �Ĵ���, ��ʹ�� MAS7 �Ĵ����ķ���
     */
    if (_G_bHasMAS7) {
        UINT32  uiHID0;

        uiHID0  = ppcE500GetHID0();
        uiHID0 |= ARCH_PPC_HID0_MAS7EN;
        ppcE500SetHID0(uiHID0);
    }

    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  �� Core0 ��λ Cache         */
        archCacheReset(pcMachineName);                                  /*  ��λ Cache                  */
    }

    ppcE500MmuInvalidateTLB();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcE500MmuPgdOffset
** ��������: ͨ�������ַ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PGD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcE500MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER UINT32              uiPgdNum;
    
    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry +
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcE500MmuPmdOffset
** ��������: ͨ�������ַ���� PMD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PMD �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcE500MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  PowerPC �� PMD ��           */
}
/*********************************************************************************************************
** ��������: ppcE500MmuPteOffset
** ��������: ͨ�������ַ���� PTE ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ��Ӧ�� PTE �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *ppcE500MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;
    REGISTER UINT32              uiPageNum;

    uiTemp = (UINT32)(*p_pmdentry);                                     /*  ���һ��ҳ��������          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp);                         /*  ��ö���ҳ�����ַ          */

    ulAddr &= ~LW_CFG_VMM_PGD_MASK;

    uiPageNum = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry +
                  (uiPageNum * sizeof(LW_PTE_TRANSENTRY)));             /*  ��������ַҳ����������ַ  */
    
    return  (p_pteentry);
}
/*********************************************************************************************************
** ��������: ppcE500MmuPgdIsOk
** ��������: �ж� PGD ����������Ƿ���ȷ
** �䡡��  : pgdentry       PGD ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  ppcE500MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: ppcE500MmuPteIsOk
** ��������: �ж� PTE ����������Ƿ���ȷ
** �䡡��  : pteentry       PTE ��������
** �䡡��  : �Ƿ���ȷ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  ppcE500MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry.MAS3_bValid) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: ppcE500MmuPgdAlloc
** ��������: ���� PGD ��
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ (���� 0 ��ƫ����Ϊ 0 , ��Ҫ����ҳ�����ַ)
** �䡡��  : ���� PGD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcE500MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    REGISTER UINT32              uiPgdNum;
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  ��Чһ��ҳ����              */

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry +
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  ���һ��ҳ����������ַ      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcE500MmuPgdFree
** ��������: �ͷ� PGD ��
** �䡡��  : p_pgdentry     pgd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppcE500MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcE500MmuPmdAlloc
** ��������: ���� PMD ��
** �䡡��  : pmmuctx        mmu ������
**           p_pgdentry     pgd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PMD ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcE500MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PGD_TRANSENTRY  *p_pgdentry,
                                               addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** ��������: ppcE500MmuPmdFree
** ��������: �ͷ� PMD ��
** �䡡��  : p_pmdentry     pmd ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppcE500MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** ��������: ppcE500MmuPteAlloc
** ��������: ���� PTE ��
** �䡡��  : pmmuctx        mmu ������
**           p_pmdentry     pmd ��ڵ�ַ
**           ulAddr         �����ַ
** �䡡��  : ���� PTE ��ַ
** ȫ�ֱ���: 
** ����ģ��: VMM ����û�йر��ж�, ��д CACHE ʱ, ��Ҫ�ֶ����ж�, SylixOS ӳ����ϻ��Զ�����, ����
             ���ﲻ��������.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *ppcE500MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PMD_TRANSENTRY  *p_pmdentry,
                                               addr_t              ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiPgdNum;

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + (uiPgdNum * PTE_BLOCK_SIZE));

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)ppcE500MmuBuildPgdesc((UINT32)p_pteentry); /*  ���ö���ҳ�����ַ  */

    return  (ppcE500MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** ��������: ppcE500MmuPteFree
** ��������: �ͷ� PTE ��
** �䡡��  : p_pteentry     pte ��ڵ�ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppcE500MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    (VOID)p_pteentry;
}
/*********************************************************************************************************
** ��������: ppcE500MmuPtePhysGet
** ��������: ͨ�� PTE ����, ��ѯ�����ַ
** �䡡��  : pteentry           pte ����
**           pulPhysicalAddr    ��õ������ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ppcE500MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    UINT32   uiRPN   = pteentry.MAS3_uiRPN;                             /*  �������ҳ���              */

    *pulPhysicalAddr = uiRPN << LW_CFG_VMM_PAGE_SHIFT;                  /*  ����ҳ�������ַ            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcE500MmuFlagGet
** ��������: ���ָ�������ַ�� SylixOS Ȩ�ޱ�־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
** �䡡��  : SylixOS Ȩ�ޱ�־
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  ppcE500MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcE500MmuPgdOffset(pmmuctx, ulAddr);  /*  ���һ����������ַ      */

    if (ppcE500MmuPgdIsOk(*p_pgdentry)) {                               /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcE500MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  ��ö���������              */

        if (ppcE500MmuPteIsOk(uiDescriptor)) {                          /*  ������������Ч              */
            ULONG    ulFlag = 0;

            if (uiDescriptor.MAS3_bValid) {                             /*  ��Ч                        */
                ulFlag |= LW_VMM_FLAG_VALID;                            /*  ӳ����Ч                    */
            }

            ulFlag |= LW_VMM_FLAG_ACCESS;                               /*  ���Է���                    */
            ulFlag |= LW_VMM_FLAG_GUARDED;                              /*  �����ϸ�Ȩ�޼��            */

            if (uiDescriptor.MAS3_bSuperExec) {
                ulFlag |= LW_VMM_FLAG_EXECABLE;                         /*  ����ִ��                    */
            }

            if (uiDescriptor.MAS3_bSuperWrite) {                        /*  ��д                        */
                ulFlag |= LW_VMM_FLAG_WRITABLE;
            }

            if (!uiDescriptor.MAS3_bUnCache) {                          /*  �� Cache                    */
                ulFlag |= LW_VMM_FLAG_CACHEABLE;

                if (!uiDescriptor.MAS3_bWT) {                           /*  ��д��͸                    */
                    ulFlag |= LW_VMM_FLAG_BUFFERABLE;
                }
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** ��������: ppcE500MmuFlagSet
** ��������: ����ָ�������ַ�� flag ��־
** �䡡��  : pmmuctx        mmu ������
**           ulAddr         �����ַ
**           ulFlag         flag ��־
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��Ҫ������ TLB, ��Ϊ VMM ��������˲���.
*********************************************************************************************************/
static INT  ppcE500MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcE500MmuPgdOffset(pmmuctx, ulAddr);  /*  ���һ����������ַ      */

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  ��Ч��ӳ���ϵ              */
        return  (PX_ERROR);
    }

    if (ppcE500MmuPgdIsOk(*p_pgdentry)) {                               /*  һ����������Ч              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcE500MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  ��ö�����������ַ          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  ��ö���������              */

        if (ppcE500MmuPteIsOk(uiDescriptor)) {                          /*  ������������Ч              */
            UINT32        uiRPN = uiDescriptor.MAS3_uiRPN;              /*  �������ҳ��                */
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
            uiRPN |= uiDescriptor.MAS7_uiHigh4RPN << 20;
#endif                                                                  /*  ����ҳ�������ַ            */
            phys_addr_t   paPhysicalAddr = ((phys_addr_t)uiRPN) << LW_CFG_VMM_PAGE_SHIFT;

            /*
             * �������������������ö���������
             */
            *p_pteentry = ppcE500MmuBuildPtentry(paPhysicalAddr, ulFlag);

#if LW_CFG_CACHE_EN > 0
            ppcCacheDataUpdate((PVOID)p_pteentry,
                               sizeof(LW_PTE_TRANSENTRY), LW_FALSE);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            return  (ERROR_NONE);

        } else {
            return  (PX_ERROR);
        }

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: ppcE500MmuMakeTrans
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
static VOID  ppcE500MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
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
    *p_pteentry = ppcE500MmuBuildPtentry(paPhysicalAddr, ulFlag);

#if LW_CFG_CACHE_EN > 0
    ppcCacheDataUpdate((PVOID)p_pteentry,
                       sizeof(LW_PTE_TRANSENTRY), LW_FALSE);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** ��������: ppcE500MmuMakeCurCtx
** ��������: ���� MMU ��ǰ������
** �䡡��  : pmmuctx        mmu ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppcE500MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ppcSetSPRG2(_G_bHasMAS7);                                           /*  ʹ�� SPRG2 ��¼ bHasMAS7    */
    ppcSetSPRG3(_G_ulPTETable);                                         /*  ʹ�� SPRG3 ��¼ PTETable    */
}
/*********************************************************************************************************
** ��������: ppcE500MmuInvTLB
** ��������: ��Ч��ǰ CPU TLB
** �䡡��  : pmmuctx        mmu ������
**           ulPageAddr     ҳ�������ַ
**           ulPageNum      ҳ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcE500MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > (_G_uiTlbSize >> 1)) {
        ppcE500MmuInvalidateTLB();                                      /*  ȫ����� TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            ppcE500MmuInvalidateTLBEA((addr_t)ulPageAddr);              /*  ���ҳ����� TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** ��������: ppcE500MmuInit
** ��������: MMU ϵͳ��ʼ��
** �䡡��  : pmmuop            MMU ����������
**           pcMachineName     ʹ�õĻ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcE500MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    TLBCFG_REG  uiTLB0CFG;

    /*
     * ʹ���˵�ַ�㲥(HID1[ABE] λ)��, tlbsync ָ����Զ����ͬ��
     */
    pmmuop->MMUOP_ulOption = 0ul;

    pmmuop->MMUOP_pfuncMemInit       = ppcE500MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = ppcE500MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc      = ppcE500MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = ppcE500MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = ppcE500MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = ppcE500MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = ppcE500MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = ppcE500MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk       = ppcE500MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = ppcE500MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = ppcE500MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset     = ppcE500MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = ppcE500MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = ppcE500MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet    = ppcE500MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet       = ppcE500MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = ppcE500MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = ppcE500MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = ppcE500MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = ppcE500MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = ppcE500MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = ppcE500MmuDisable;

    /*
     * ���һ������ʹ�� HID1[ABE] λ
     */
    MMU_MAS2_M = (LW_NCPUS > 1) ? 1 : 0;                                /*  ���һ����λ����            */

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasMAS7 = LW_TRUE;
    } else {
        _G_bHasMAS7 = LW_FALSE;
    }

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasHID1 = LW_FALSE;
    } else {
        _G_bHasHID1 = LW_TRUE;
    }

    /*
     * ��� TLB0 ��Ŀ��
     */
    uiTLB0CFG.TLBCFG_uiValue = ppcE500MmuGetTLB0CFG();
    _G_uiTlbSize = uiTLB0CFG.TLBCFG_usNENTRY;
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU TLB0 size = %d.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, _G_uiTlbSize);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
