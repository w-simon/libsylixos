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
;** 文   件   名: cskyContextAsm.h
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 05 月 11 日
;**
;** 描        述: C-SKY 体系架构上下文处理.
;*********************************************************************************************************/

#ifndef __ARCH_CSKYCONTEXTASM_H
#define __ARCH_CSKYCONTEXTASM_H

#include "arch/csky/arch_regs.h"

;/*********************************************************************************************************
;  保存内核态任务寄存器(参数 A1: ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(SAVE_KERN_REGS)
    STM         R0-R31 , (A1)                                           ;/*  保存 R0 - R31               */

    ST.W        RA , (A1 , XPC)                                         ;/*  RA 代替 PC 保存             */
    
    MFCR        A2 , PSR                                                ;/*  保存 PSR 寄存器             */
    ST.W        A2 , (A1 , XPSR)

    MFLO        A2                                                      ;/*  保存 LO 寄存器              */
    ST.W        A2 , (A1 , XLO)

    MFHI        A2                                                      ;/*  保存 HI 寄存器              */
    ST.W        A2 , (A1 , XHI)
    MACRO_END()

;/*********************************************************************************************************
;  恢复内核态任务寄存器(参数 A1: ARCH_REG_CTX 地址)
;  注意: 目前仅适合 ABIv2
;*********************************************************************************************************/

MACRO_DEF(RESTORE_KERN_REGS)
    LD.W        A0 , (A1 , XLO)                                         ;/*  恢复 LO 寄存器              */
    MTLO        A0

    LD.W        A0 , (A1 , XHI)                                         ;/*  恢复 HI 寄存器              */
    MTHI        A0

    LD.W        A0 , (A1 , XPSR)                                        ;/*  恢复 PSR 寄存器             */
    MTCR        A0 , EPSR

    LD.W        A0 , (A1 , XPC)                                         ;/*  恢复 PC 寄存器              */
    MTCR        A0 , EPC

    LD.W        R0 , (A1 , XGREG(0))                                    ;/*  恢复 R0 - R31               */

    ADDI        A1 , (2 * ARCH_REG_SIZE)
    LDM         R2-R31 , (A1)
    SUBI        A1 , (2 * ARCH_REG_SIZE)
    LD.W        A1 , (A1 , XGREG(1))

    RTE
    MACRO_END()

;/*********************************************************************************************************
;  恢复任务寄存器(参数 A1: ARCH_REG_CTX 地址)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    RESTORE_KERN_REGS
    MACRO_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
