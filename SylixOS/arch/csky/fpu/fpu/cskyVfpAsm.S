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
;** 文   件   名: cskyVfpAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 05 月 14 日
;**
;** 描        述: C-SKY 体系架构 VFP 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0
#include "arch/csky/arch_float.h"
#include "arch/csky/inc/cskyregs.h"

    FILE_BEGIN()

    EXPORT_LABEL(cskyVfpInit)
    EXPORT_LABEL(cskyVfpEnable)
    EXPORT_LABEL(cskyVfpDisable)
    EXPORT_LABEL(cskyVfpIsEnable)
    EXPORT_LABEL(cskyVfpSave)
    EXPORT_LABEL(cskyVfpRestore)

;/*********************************************************************************************************
;  宏定义
;*********************************************************************************************************/

#define FCR_ENABLE      (IDE_STAT | UFE_STAT | OFE_STAT | DZE_STAT | IOE_STAT)

MACRO_DEF(FPU_REG_RESTORE fr, tmp, base, offsetl, offseth)
    LD.W        \tmp , (\base , \offsetl)
    FMTVRL      \fr  , \tmp
    LD.W        \tmp , (\base , \offseth)
    FMTVRH      \fr  , \tmp
    MACRO_END()

MACRO_DEF(FPU_REG_SAVE fr, tmp, base, offsetl, offseth)
    FMFVRL      \tmp  , \fr
    ST.W        \tmp , (\base , \offsetl)
    FMFVRH      \tmp  , \fr
    ST.W        \tmp , (\base , \offseth)
    MACRO_END()

;/*********************************************************************************************************
;  使能 FPU 
;*********************************************************************************************************/

MACRO_DEF(ENABLE_FPU)
    MFCR        A1 , CR<1, 2>
    ORI         A1 , A1 , FCR_ENABLE
    MTCR        A1 , CR<1, 2>
    MACRO_END()

;/*********************************************************************************************************
;  初始化 FPU
;*********************************************************************************************************/

FUNC_DEF(cskyVfpInit)
    MFCR        A1 , CR<1, 2>                                           ;/*  记录原 FPU 状态             */

    ENABLE_FPU                                                          ;/*  使能 FPU                    */
        
    MOVI        A0 , 0

    FMTVRL      FR0 , A0                                                ;/*  清零 FPU 寄存器             */
    FMTVRH      FR0 , A0
    FMTVRL      FR1 , A0
    FMTVRH      FR1 , A0
    FMTVRL      FR2 , A0
    FMTVRH      FR2 , A0
    FMTVRL      FR3 , A0
    FMTVRH      FR3 , A0
    FMTVRL      FR4 , A0
    FMTVRH      FR4 , A0
    FMTVRL      FR5 , A0
    FMTVRH      FR5 , A0
    FMTVRL      FR6 , A0
    FMTVRH      FR6 , A0
    FMTVRL      FR7 , A0
    FMTVRH      FR7 , A0
    FMTVRL      FR8 , A0
    FMTVRH      FR8 , A0
    FMTVRL      FR9 , A0
    FMTVRH      FR9 , A0
    FMTVRL      FR10, A0
    FMTVRH      FR10, A0
    FMTVRL      FR11, A0
    FMTVRH      FR11, A0
    FMTVRL      FR12, A0
    FMTVRH      FR12, A0
    FMTVRL      FR13, A0
    FMTVRH      FR13, A0
    FMTVRL      FR14, A0
    FMTVRH      FR14, A0
    FMTVRL      FR15, A0
    FMTVRH      FR15, A0
     
    MTCR        A1 , CR<1, 2>                                           ;/*  恢复原 FPU 开关状态         */
    RTS
    FUNC_END(cskyVfpInit)

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

FUNC_DEF(cskyVfpEnable)
    ENABLE_FPU                                                          ;/*  使能 FPU                    */
    RTS
    FUNC_END(cskyVfpEnable)

;/*********************************************************************************************************
;  禁能 FPU
;*********************************************************************************************************/

FUNC_DEF(cskyVfpDisable)
    MFCR        A0 , CR<1, 2>
    MOVI        A1 , FCR_ENABLE
    ANDN        A0 , A1
    MTCR        A0 , CR<1, 2>
    RTS
    FUNC_END(cskyVfpDisable)

;/*********************************************************************************************************
;  判断 FPU 是否使能
;*********************************************************************************************************/

FUNC_DEF(cskyVfpIsEnable)
    MFCR        A1 , CR<1, 2>
    ANDI        A1 , A1 , FCR_ENABLE
    BEZ         A1 , 1f
    MOVI        A0 , 1
1:
    MOVI        A0 , 0
    RTS
    FUNC_END(cskyVfpIsEnable)

;/*********************************************************************************************************
;  保存 FPU 寄存器
;*********************************************************************************************************/

FUNC_DEF(cskyVfpSave)
    ENABLE_FPU                                                          ;/*  使能 FPU                    */

    FPU_REG_SAVE    FR0 ,  A1,  A0,  FPU_OFFSET_REG_LO(0),  FPU_OFFSET_REG_HI(0)
    FPU_REG_SAVE    FR1 ,  A1,  A0,  FPU_OFFSET_REG_LO(1),  FPU_OFFSET_REG_HI(1)
    FPU_REG_SAVE    FR2 ,  A1,  A0,  FPU_OFFSET_REG_LO(2),  FPU_OFFSET_REG_HI(2)
    FPU_REG_SAVE    FR3 ,  A1,  A0,  FPU_OFFSET_REG_LO(3),  FPU_OFFSET_REG_HI(3)
    FPU_REG_SAVE    FR4 ,  A1,  A0,  FPU_OFFSET_REG_LO(4),  FPU_OFFSET_REG_HI(4)
    FPU_REG_SAVE    FR5 ,  A1,  A0,  FPU_OFFSET_REG_LO(5),  FPU_OFFSET_REG_HI(5)
    FPU_REG_SAVE    FR6 ,  A1,  A0,  FPU_OFFSET_REG_LO(6),  FPU_OFFSET_REG_HI(6)
    FPU_REG_SAVE    FR7 ,  A1,  A0,  FPU_OFFSET_REG_LO(7),  FPU_OFFSET_REG_HI(7)
    FPU_REG_SAVE    FR8 ,  A1,  A0,  FPU_OFFSET_REG_LO(8),  FPU_OFFSET_REG_HI(8)
    FPU_REG_SAVE    FR9 ,  A1,  A0,  FPU_OFFSET_REG_LO(9),  FPU_OFFSET_REG_HI(9)
    FPU_REG_SAVE    FR10,  A1,  A0,  FPU_OFFSET_REG_LO(10), FPU_OFFSET_REG_HI(10)
    FPU_REG_SAVE    FR11,  A1,  A0,  FPU_OFFSET_REG_LO(11), FPU_OFFSET_REG_HI(11)
    FPU_REG_SAVE    FR12,  A1,  A0,  FPU_OFFSET_REG_LO(12), FPU_OFFSET_REG_HI(12)
    FPU_REG_SAVE    FR13,  A1,  A0,  FPU_OFFSET_REG_LO(13), FPU_OFFSET_REG_HI(13)
    FPU_REG_SAVE    FR14,  A1,  A0,  FPU_OFFSET_REG_LO(14), FPU_OFFSET_REG_HI(14)
    FPU_REG_SAVE    FR15,  A1,  A0,  FPU_OFFSET_REG_LO(15), FPU_OFFSET_REG_HI(15)

    MFCR        A1  , CR<1, 2>
    ST.W        A1  , (A0 , FPU_OFFSET_FCR)                             ;/*  保存 FCR 状态寄存器         */
    
    MFCR        A1  , CR<2, 2>
    ST.W        A1  , (A0 , FPU_OFFSET_FESR)                            ;/*  保存 FECR 状态寄存器        */

    RTS
    FUNC_END(cskyVfpSave)

;/*********************************************************************************************************
;  恢复 FPU 寄存器
;*********************************************************************************************************/

FUNC_DEF(cskyVfpRestore)
    ENABLE_FPU                                                          ;/*  使能 FPU                    */
   
    LD.W        A1  , (A0 , FPU_OFFSET_FESR)                            ;/*  恢复 FECR 状态寄存器        */
    MTCR        A1  , CR<2, 2>
        
    FPU_REG_RESTORE FR0 ,  A1,  A0,  FPU_OFFSET_REG_LO(0),  FPU_OFFSET_REG_HI(0)
    FPU_REG_RESTORE FR1 ,  A1,  A0,  FPU_OFFSET_REG_LO(1),  FPU_OFFSET_REG_HI(1)
    FPU_REG_RESTORE FR2 ,  A1,  A0,  FPU_OFFSET_REG_LO(2),  FPU_OFFSET_REG_HI(2)
    FPU_REG_RESTORE FR3 ,  A1,  A0,  FPU_OFFSET_REG_LO(3),  FPU_OFFSET_REG_HI(3)
    FPU_REG_RESTORE FR4 ,  A1,  A0,  FPU_OFFSET_REG_LO(4),  FPU_OFFSET_REG_HI(4)
    FPU_REG_RESTORE FR5 ,  A1,  A0,  FPU_OFFSET_REG_LO(5),  FPU_OFFSET_REG_HI(5)
    FPU_REG_RESTORE FR6 ,  A1,  A0,  FPU_OFFSET_REG_LO(6),  FPU_OFFSET_REG_HI(6)
    FPU_REG_RESTORE FR7 ,  A1,  A0,  FPU_OFFSET_REG_LO(7),  FPU_OFFSET_REG_HI(7)
    FPU_REG_RESTORE FR8 ,  A1,  A0,  FPU_OFFSET_REG_LO(8),  FPU_OFFSET_REG_HI(8)
    FPU_REG_RESTORE FR9 ,  A1,  A0,  FPU_OFFSET_REG_LO(9),  FPU_OFFSET_REG_HI(9)
    FPU_REG_RESTORE FR10,  A1,  A0,  FPU_OFFSET_REG_LO(10), FPU_OFFSET_REG_HI(10)
    FPU_REG_RESTORE FR11,  A1,  A0,  FPU_OFFSET_REG_LO(11), FPU_OFFSET_REG_HI(11)
    FPU_REG_RESTORE FR12,  A1,  A0,  FPU_OFFSET_REG_LO(12), FPU_OFFSET_REG_HI(12)
    FPU_REG_RESTORE FR13,  A1,  A0,  FPU_OFFSET_REG_LO(13), FPU_OFFSET_REG_HI(13)
    FPU_REG_RESTORE FR14,  A1,  A0,  FPU_OFFSET_REG_LO(14), FPU_OFFSET_REG_HI(14)
    FPU_REG_RESTORE FR15,  A1,  A0,  FPU_OFFSET_REG_LO(15), FPU_OFFSET_REG_HI(15)
    
    LD.W        A1  , (A0 , FPU_OFFSET_FCR)                             ;/*  恢复 FCR 状态寄存器         */
    MTCR        A1  , CR<1, 2>

    RTS
    FUNC_END(cskyVfpRestore)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
