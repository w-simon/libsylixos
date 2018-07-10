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
;** 文   件   名: riscvMpCoreAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2018 年 03 月 20 日
;**
;** 描        述: RISC-V 体系构架多核接口驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "arch/riscv/arch_regs.h"

#if LW_CFG_SMP_EN > 0

    FILE_BEGIN()
    
    IMPORT_LABEL(bspMpInt)
    
    EXPORT_LABEL(riscvSpinLock)
    EXPORT_LABEL(riscvSpinTryLock)
    EXPORT_LABEL(riscvSpinUnlock)
    EXPORT_LABEL(archMpCur)
    EXPORT_LABEL(archMpInt)
    
    WEAK(riscvSpinLock)
    WEAK(riscvSpinTryLock)
    WEAK(riscvSpinUnlock)
    WEAK(archMpCur)
    WEAK(archMpInt)

;/*********************************************************************************************************
;  自旋锁
;*********************************************************************************************************/

FUNC_DEF(riscvSpinLock)
    LI              T0 , 1
LINE_LABEL(again)
    AMOSWAP.W.AQ    T0 , T0 , (A0)
    BNEZ            T0 , again
    RET
    FUNC_END(riscvSpinLock)

FUNC_DEF(riscvSpinTryLock)
    LI              T0 , 1
    AMOSWAP.W.AQ    T0 , T0 , (A0)
    MOVE            RV0 , T0
    RET
    FUNC_END(riscvSpinTryLock)

FUNC_DEF(riscvSpinUnlock)
    AMOSWAP.W.RL    ZERO , ZERO , (A0)
    RET
    FUNC_END(riscvSpinUnlock)

;/*********************************************************************************************************
;  获得当前核 ID
;*********************************************************************************************************/

FUNC_DEF(archMpCur)
    CSRR    T0  , XSCRATCH
    REG_L   RV0 , CPUID_OFFSET(T0)
    RET
    FUNC_END(archMpCur)

;/*********************************************************************************************************
;  产生一次核间中断
;*********************************************************************************************************/

FUNC_DEF(archMpInt)
    J       bspMpInt
    FUNC_END(archMpInt)
    
    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
