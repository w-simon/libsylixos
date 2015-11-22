;/**********************************************************************************************************
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
;** 文   件   名: mipsVfp32Asm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 11 月 17 日
;**
;** 描        述: MIPS 体系架构 VFP32 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()

    EXPORT_LABEL(mipsVfp32Sid)
    EXPORT_LABEL(mipsVfp32Enable)
    EXPORT_LABEL(mipsVfp32Disable)
    EXPORT_LABEL(mipsVfp32IsEnable)
    EXPORT_LABEL(mipsVfp32Save16)
    EXPORT_LABEL(mipsVfp32Restore16)
    EXPORT_LABEL(mipsVfp32Save32)
    EXPORT_LABEL(mipsVfp32Restore32)

;/*********************************************************************************************************
;  浮点运算器基本操作函数(mipsVfp32Sid 函数先保留)
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Sid)
    LA      V0, 0
    JR      RA
    NOP
FUNC_END(mipsVfp32Sid)

FUNC_DEF(mipsVfp32Enable)
    MFC0(T0, CP0_STATUS)
    MOV     T1, T0
    AND     T1, M_StatusCU1
    BEQ     T1, 1, 2f
    NOP
    OR      T0, M_StatusCU1
    MTC0(T0, CP0_STATUS)
2:
    JR      RA
    NOP
FUNC_END(mipsVfp32Enable)

FUNC_DEF(mipsVfp32Disable)
    MFC0(T0, CP0_STATUS)
    MOV     T1, T0
    AND     T1, M_StatusCU1
    BEQ     T1, 0, 2f
    NOP
    AND     T0, ~M_StatusCU1
    MTC0(T0, CP0_STATUS)
2:
    JR      RA
    NOP
FUNC_END(mipsVfp32Disable)

FUNC_DEF(mipsVfp32IsEnable)
    MFC0    T0, CP0_STATUS
    MOV     V0, ZERO                                                    ;/*  OK return value             */
    AND     T0, M_StatusCU1                                             ;/*  coprocessor 1 enabled       */
    BNE     T0, ZERO, 2f
    LI      V0, 1
2:
    JR      RA
    NOP
FUNC_END(mipsVfp32IsEnable)

;/*********************************************************************************************************
;  VFP-D16
;   +-----------+
;   | freg[31]  |    + 0x80
;   |  ...      |
;   | freg[2]   |    + 0x0C
;   | freg[1]   |    + 0x08
;   | freg[0]   |    + 0x04
;   | fpscr     | <-- cpu_fpu_context ( = a0 )
;   +-----------+
;*********************************************************************************************************/

;/*********************************************************************************************************
;  MIPSVFP 保存/恢复寄存器 32Bit MIPS 使用偶数的寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Save16)
    CFC1(T0,  CP1_STATUS)
    SW      T0,     0x00(A0)
    SDC1    FP0,    0x04(A0)
    SDC1    FP2,    0x0c(A0)
    SDC1    FP4,    0x14(A0)
    SDC1    FP6,    0x1c(A0)
    SDC1    FP8,    0x24(A0)
    SDC1    FP10,   0x2c(A0)
    SDC1    FP12,   0x34(A0)
    SDC1    FP14,   0x3c(A0)
    SDC1    FP16,   0x44(A0)
    SDC1    FP18,   0x4c(A0)
    SDC1    FP20,   0x54(A0)
    SDC1    FP22,   0x5c(A0)
    SDC1    FP24,   0x64(A0)
    SDC1    FP26,   0x6c(A0)
    SDC1    FP28,   0x74(A0)
    SDC1    FP30,   0x7c(A0)
    JR      RA
    LI      V0,     0
FUNC_END(mipsVfp32Save16)

FUNC_DEF(mipsVfp32Restore16)
    LW      T0,     0X00(A0)
    LDC1    FP0,    0x04(A0)
    LDC1    FP2,    0x0c(A0)
    LDC1    FP4,    0x14(A0)
    LDC1    FP6,    0x1c(A0)
    LDC1    FP8,    0x24(A0)
    LDC1    FP10,   0x2c(A0)
    LDC1    FP12,   0x34(A0)
    LDC1    FP14,   0x3c(A0)
    LDC1    FP16,   0x44(A0)
    LDC1    FP18,   0x4c(A0)
    LDC1    FP20,   0x54(A0)
    LDC1    FP22,   0x5c(A0)
    LDC1    FP24,   0x64(A0)
    LDC1    FP26,   0x6c(A0)
    LDC1    FP28,   0x74(A0)
    LDC1    FP30,   0x7c(A0)
    CTC1(T0,     CP1_STATUS)                                            ;/*  restore fpp status reg      */
    JR      RA
    LI      V0,     0
FUNC_END(mipsVfp32Restore16)

;/*********************************************************************************************************
;  MIPSVFP32 保存/恢复寄存器 64Bit MIPS 使用32的寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Save32)
    JR      RA
    NOP
FUNC_END(mipsVfp32Save32)

FUNC_DEF(mipsVfp32Restore32)
    JR      RA
    NOP
FUNC_END(mipsVfp32Restore32)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
