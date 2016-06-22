;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: mips32CacheAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 01 日
;**
;** 描        述: MIPS32 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if LW_CFG_CACHE_EN > 0

    EXPORT_LABEL(mips32L1CacheDisable)
    EXPORT_LABEL(mips32L1CacheEnable)

    EXPORT_LABEL(mips32DCacheLineFlush)
    EXPORT_LABEL(mips32DCacheLineClear)
    EXPORT_LABEL(mips32DCacheLineInvalidate)

    EXPORT_LABEL(mips32DCacheIndexClear)

    EXPORT_LABEL(mips32ICacheLineInvalidate)
    EXPORT_LABEL(mips32ICacheIndexInvalidate)

    EXPORT_LABEL(mips32ICacheFill)
    EXPORT_LABEL(mips32ICacheIndexStoreTag)
    EXPORT_LABEL(mips32DCacheIndexStoreTag)

    EXPORT_LABEL(mips32BranchPredictionDisable)
    EXPORT_LABEL(mips32BranchPredictionEnable)
    EXPORT_LABEL(mips32BranchPredictorInvalidate)

    IMPORT_LABEL(mips32ICacheInvalidateAll)
    IMPORT_LABEL(mips32DCacheClearAll)

    WEAK(mips32BranchPredictionDisable)
    WEAK(mips32BranchPredictionEnable)
    WEAK(mips32BranchPredictorInvalidate)

    FILE_BEGIN()

;/*********************************************************************************************************
;  禁能 CACHE
;*********************************************************************************************************/

FUNC_DEF(mips32L1CacheDisable)
    .set    push
    .set    noreorder
    .set    volatile

    MFC0(T0, CP0_CONFIG)
    ANDI    T0 , T0 , M_ConfigK0
    LI      T1 , MIPS_UNCACHED
    BEQ     T0 , T1 , 1f
    NOP

    SUBU    SP , 32
    SW      RA , 28(SP)
    SW      FP , 24(SP)
    MOV     FP , SP

    JAL     mips32DCacheClearAll
    NOP

    JAL     mips32ICacheInvalidateAll
    NOP

    MOV     SP , FP
    LW      RA , 28(SP)
    LW      FP , 24(SP)
    ADDU    SP , 32

    MFC0(T0, CP0_CONFIG)
    AND     T0 , T0 , ~M_ConfigK0
    ORI     T0 , T0 , MIPS_UNCACHED
    MTC0(T0, CP0_CONFIG)

1:
    JR      RA
    NOP

    .set    pop
FUNC_END(mips32L1CacheDisable)

;/*********************************************************************************************************
;  使能 CACHE
;*********************************************************************************************************/

FUNC_DEF(mips32L1CacheEnable)
    .set    push
    .set    noreorder
    .set    volatile

    MFC0(T0, CP0_CONFIG)
    ANDI    T0 , T0 , M_ConfigK0
    LI      T1 , MIPS_CACHABLE_NONCOHERENT
    BEQ     T0 , T1 , 1f
    NOP

    MFC0(T0, CP0_CONFIG)
    AND     T0 , T0 , ~M_ConfigK0
    ORI     T0 , MIPS_CACHABLE_NONCOHERENT
    MTC0(T0, CP0_CONFIG)

1:
    JR      RA
    NOP

    .set    pop
FUNC_END(mips32L1CacheEnable)

;/*********************************************************************************************************
;  CACHE 操作宏
;*********************************************************************************************************/

MACRO_DEF(CACHE_OP op addr)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile
    .set    mips32

    cache   \op , 0(\addr)

    JR      RA
    NOP

    .set    pop
MACRO_END()

;/*********************************************************************************************************
;  回写一个 DCACHE 行
;*********************************************************************************************************/

FUNC_DEF(mips32DCacheLineFlush)
    CACHE_OP    Hit_Writeback_D         A0
FUNC_END(mips32DCacheLineFlush)

;/*********************************************************************************************************
;  回写并无效一个 DCACHE 行
;*********************************************************************************************************/

FUNC_DEF(mips32DCacheLineClear)
    CACHE_OP    Hit_Writeback_Inv_D     A0
FUNC_END(mips32DCacheLineClear)

;/*********************************************************************************************************
;  无效一个 DCACHE 行
;*********************************************************************************************************/

FUNC_DEF(mips32DCacheLineInvalidate)
    CACHE_OP    Hit_Invalidate_D        A0
FUNC_END(mips32DCacheLineInvalidate)

;/*********************************************************************************************************
;  通过 Cache 存储器的地址索引回写并无效一个 DCACHE 行
;*********************************************************************************************************/

FUNC_DEF(mips32DCacheIndexClear)
    CACHE_OP    Index_Writeback_Inv_D   A0
FUNC_END(mips32DCacheIndexClear)

;/*********************************************************************************************************
;  无效一个 ICACHE 行
;*********************************************************************************************************/

FUNC_DEF(mips32ICacheLineInvalidate)
    CACHE_OP    Hit_Invalidate_I        A0
FUNC_END(mips32ICacheLineInvalidate)

;/*********************************************************************************************************
;  通过 Cache 存储器的地址索引回写并无效一个 ICACHE 行
;*********************************************************************************************************/

FUNC_DEF(mips32ICacheIndexInvalidate)
    CACHE_OP    Index_Invalidate_I      A0
FUNC_END(mips32ICacheIndexInvalidate)

;/*********************************************************************************************************
;  FILL I-Cache
;*********************************************************************************************************/

FUNC_DEF(mips32ICacheFill)
    CACHE_OP    Fill_I                  A0
FUNC_END(mips32ICacheFill)

;/*********************************************************************************************************
;  设置 D-Cache 的 TAG
;*********************************************************************************************************/

FUNC_DEF(mips32DCacheIndexStoreTag)
    CACHE_OP    Index_Store_Tag_D       A0
FUNC_END(mips32DCacheIndexStoreTag)

;/*********************************************************************************************************
;  设置 I-Cache 的 TAG
;*********************************************************************************************************/

FUNC_DEF(mips32ICacheIndexStoreTag)
    CACHE_OP    Index_Store_Tag_I       A0
FUNC_END(mips32ICacheIndexStoreTag)

;/*********************************************************************************************************
;  禁能分支预测
;*********************************************************************************************************/

FUNC_DEF(mips32BranchPredictionDisable)
    JR      RA
    NOP
FUNC_END(mips32BranchPredictionDisable)

;/*********************************************************************************************************
;  使能分支预测
;*********************************************************************************************************/

FUNC_DEF(mips32BranchPredictionEnable)
    JR      RA
    NOP
FUNC_END(mips32BranchPredictionEnable)

;/*********************************************************************************************************
;  无效分支预测
;*********************************************************************************************************/

FUNC_DEF(mips32BranchPredictorInvalidate)
    JR      RA
    NOP
FUNC_END(mips32BranchPredictorInvalidate)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
