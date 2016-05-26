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
** ��   ��   ��: mips32MmuAsm.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 10 �� 12 ��
**
** ��        ��: MIPS32 ��ϵ���� MMU ����.
*********************************************************************************************************/

#ifndef __MIPS32MMUASM_H
#define __MIPS32MMUASM_H

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#include "config/kernel/memory_cfg.h"
#endif

#include "arch/mips/arch_mmu.h"

/*********************************************************************************************************
  TLB refill, 32 bit task
*********************************************************************************************************/

#if LW_CFG_VMM_PAGE_SIZE == (4 * LW_CFG_KB_SIZE)
#define SRL_K1()    SRL     K1 , (PTE_BASE_SIZE + 1)
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define SRL_K1()    SRL     K1 , (PTE_BASE_SIZE + 1 + 2)
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define SRL_K1()    SRL     K1 , (PTE_BASE_SIZE + 1 + 4)
#else
#error You must set 'LW_CFG_VMM_PAGE_SIZE' as 4K, 16K or 64K
#endif                                                                  /*  LW_CFG_VMM_PAGE_SIZE        */

/*********************************************************************************************************
  TLB refill, 32 bit task
*********************************************************************************************************/

#define PTE_BASE_OFFSET     23
#define PTE_BASE_SIZE       9

#define MIPS32_TLB_REFILL_HANDLE()      \
    .set    push;                       \
    .set    noat;                       \
    .set    noreorder;                  \
                                        \
    MFC0    K1 , CP0_CTXT;              \
    EHB;                                \
    MOVE    K0 , K1;                    \
                                        \
    SLL     K1 , PTE_BASE_SIZE;         \
    SRL_K1();                           \
                                        \
    SRL     K0 , PTE_BASE_OFFSET;       \
    SLL     K0 , PTE_BASE_OFFSET;       \
                                        \
    OR      K1 , K1 , K0;               \
                                        \
    LW      K0 , 0(K1);                 \
    LW      K1 , 4(K1);                 \
    MTC0    K0 , CP0_TLBLO0;            \
    MTC0    K1 , CP0_TLBLO1;            \
    EHB;                                \
    TLBWR;                              \
    ERET;                               \
                                        \
    .set    pop

#endif                                                                  /*  __MIPS32MMUASM_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
