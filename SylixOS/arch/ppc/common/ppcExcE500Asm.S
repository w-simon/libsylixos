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
;** 文   件   名: ppcExcE500Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 05 月 04 日
;**
;** 描        述: PowerPC E500 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "ppcContextAsm.h"
#include "arch/ppc/arch_e500.h"

    FILE_BEGIN()

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterGetNesting)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)

    IMPORT_LABEL(bspIntHandle)

    IMPORT_LABEL(archE500DataStorageExceptionHandle)
    IMPORT_LABEL(archE500InstructionStorageExceptionHandle)
    IMPORT_LABEL(archE500AlignmentExceptionHandle)

    IMPORT_LABEL(archE500MmuDataTlbMissHandle)
    IMPORT_LABEL(archE500MmuInstructionTlbMissHandle)

    IMPORT_LABEL(archProgramExceptionHandle)
    IMPORT_LABEL(archFpuUnavailableExceptionHandle)
    IMPORT_LABEL(archSystemCallHandle)
    IMPORT_LABEL(archDecrementerInterruptHandle)
    IMPORT_LABEL(archTraceHandle)

    EXPORT_LABEL(archE500DataStorageExceptionEntry)
    EXPORT_LABEL(archE500InstructionStorageExceptionEntry)

    EXPORT_LABEL(archE500MmuDataTlbMissEntry)
    EXPORT_LABEL(archE500MmuInstructionTlbMissEntry)

    EXPORT_LABEL(archE500ExternalInterruptEntry)
    EXPORT_LABEL(archE500AlignmentExceptionEntry)
    EXPORT_LABEL(archE500ProgramExceptionEntry)
    EXPORT_LABEL(archE500FpuUnavailableExceptionEntry)
    EXPORT_LABEL(archE500SystemCallEntry)
    EXPORT_LABEL(archE500DecrementerInterruptEntry)
    EXPORT_LABEL(archE500TraceEntry)

;/*********************************************************************************************************
;  中断入口宏
;*********************************************************************************************************/

MACRO_DEF(E500_IRQ_ENTRY, handle)
    E500_EXC_SAVE_REGS                                                  ;/*  所有寄存器入栈              */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    BL      API_InterEnter                                              ;/*  获取中断层数放到返回值 V0   */

    CMPWI   CR0, R3 , 1
    BNE     CR0, 1f

    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */
    STW     SP , 0(R3)                                                  ;/*  current stack = SP          */

    BL      API_InterStackBaseGet                                       ;/*  获得当前 CPU 中断堆栈栈顶   */
    OR      SP , R3 , R3

1:
    MFSPR   R3 , SRR0                                                   ;/*  传递返回地址                */
    ISYNC
    BL      \handle                                                     ;/*  调用处理函数                */

    ;/*
    ; *  if (API_InterGetNesting() == 1) {
    ; *      SP = current stack;
    ; *  }
    ; */
    BL      API_InterGetNesting

    CMPWI   CR0, R3 , 1
    BNE     CR0, 2f

    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */

    LWZ     SP , 0(R3)                                                  ;/*  SP = current stack          */

2:
    BL      API_InterExit

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    MACRO_END()

;/*********************************************************************************************************
;  数据存储异常入口(Vector 0x300)
;*********************************************************************************************************/

FUNC_DEF(archE500DataStorageExceptionEntry)
    E500_IRQ_ENTRY   archE500DataStorageExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  指令访问异常入口(Vector 0x400)
;*********************************************************************************************************/

FUNC_DEF(archE500InstructionStorageExceptionEntry)
    E500_IRQ_ENTRY   archE500InstructionStorageExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  外部中断入口(Vector 0x500)
;*********************************************************************************************************/

FUNC_DEF(archE500ExternalInterruptEntry)
    E500_IRQ_ENTRY   bspIntHandle
    FUNC_END()

;/*********************************************************************************************************
;  非对齐异常入口(Vector 0x600)
;*********************************************************************************************************/

FUNC_DEF(archE500AlignmentExceptionEntry)
    E500_IRQ_ENTRY   archE500AlignmentExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  程序异常入口(Vector 0x700)
;*********************************************************************************************************/

FUNC_DEF(archE500ProgramExceptionEntry)
    E500_IRQ_ENTRY   archProgramExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  FPU 不可用入口(Vector 0x800)
;*********************************************************************************************************/

FUNC_DEF(archE500FpuUnavailableExceptionEntry)
    E500_IRQ_ENTRY   archFpuUnavailableExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  Decrementer 异常入口(Vector 0x900)
;*********************************************************************************************************/

FUNC_DEF(archE500DecrementerInterruptEntry)
    E500_IRQ_ENTRY   archDecrementerInterruptHandle
    FUNC_END()

;/*********************************************************************************************************
;  系统调用入口(Vector 0xC00)
;*********************************************************************************************************/

FUNC_DEF(archE500SystemCallEntry)
    E500_IRQ_ENTRY   archSystemCallHandle
    FUNC_END()

;/*********************************************************************************************************
;  Trace 入口(Vector 0xD00)
;*********************************************************************************************************/

FUNC_DEF(archE500TraceEntry)
    E500_IRQ_ENTRY   archTraceHandle
    FUNC_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
