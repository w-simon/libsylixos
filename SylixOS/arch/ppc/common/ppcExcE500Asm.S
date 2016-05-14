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
#include "arch/ppc/arch_e500.h"
#include "ppcContextAsm.h"
#include "ppcContextE500Asm.h"

    FILE_BEGIN()

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterGetNesting)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)

    IMPORT_LABEL(archE500CriticalInputExceptionHandle)
    IMPORT_LABEL(archE500MachineCheckExceptionHandle)
    IMPORT_LABEL(archE500DataStorageExceptionHandle)
    IMPORT_LABEL(archE500InstructionStorageExceptionHandle)
    IMPORT_LABEL(bspIntHandle)
    IMPORT_LABEL(archE500AlignmentExceptionHandle)
    IMPORT_LABEL(archE500ProgramExceptionHandle)
    IMPORT_LABEL(archE500SystemCallHandle)
    IMPORT_LABEL(archE500DecrementerInterruptHandle)
    IMPORT_LABEL(archE500TimerInterruptHandle)
    IMPORT_LABEL(archE500WatchdogInterruptHandle)
#if LW_CFG_VMM_EN > 0
    IMPORT_LABEL(archE500MmuDataTlbErrorHandle)
    IMPORT_LABEL(archE500MmuInstructionTlbErrorHandle)
#else
    IMPORT_LABEL(archE500DataTLBErrorHandle)
    IMPORT_LABEL(archE500InstructionTLBErrorHandle)
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    IMPORT_LABEL(archE500DebugInterruptHandle)
    IMPORT_LABEL(archE500FpuUnavailableExceptionHandle)

    EXPORT_LABEL(archE500CriticalInputExceptionEntry)
    EXPORT_LABEL(archE500MachineCheckExceptionEntry)
    EXPORT_LABEL(archE500DataStorageExceptionEntry)
    EXPORT_LABEL(archE500InstructionStorageExceptionEntry)
    EXPORT_LABEL(archE500ExternalInterruptEntry)
    EXPORT_LABEL(archE500AlignmentExceptionEntry)
    EXPORT_LABEL(archE500ProgramExceptionEntry)
    EXPORT_LABEL(archE500SystemCallEntry)
    EXPORT_LABEL(archE500DecrementerInterruptEntry)
    EXPORT_LABEL(archE500TimerInterruptEntry)
    EXPORT_LABEL(archE500WatchdogInterruptEntry)
    EXPORT_LABEL(archE500DataTLBErrorEntry)
    EXPORT_LABEL(archE500InstructionTLBErrorEntry)
    EXPORT_LABEL(archE500DebugInterruptEntry)
    EXPORT_LABEL(archE500FpuUnavailableExceptionEntry)

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
;  临界输入异常入口宏
;*********************************************************************************************************/

MACRO_DEF(E500_CI_IRQ_ENTRY, handle)
    E500_CI_EXC_SAVE_REGS                                               ;/*  所有寄存器入栈              */

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
    MFSPR   R3 , CSRR0                                                  ;/*  传递返回地址                */
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
;  机器检查异常入口宏
;*********************************************************************************************************/

MACRO_DEF(E500_MC_IRQ_ENTRY, handle)
    E500_MC_EXC_SAVE_REGS                                               ;/*  所有寄存器入栈              */

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
    MFSPR   R3 , MCSRR0                                                 ;/*  传递返回地址                */
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
;  临界输入异常入口(Vector IVOR0)
;*********************************************************************************************************/

FUNC_DEF(archE500CriticalInputExceptionEntry)
    E500_CI_IRQ_ENTRY   archE500CriticalInputExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  机器检查异常入口(Vector IVOR1)
;*********************************************************************************************************/

FUNC_DEF(archE500MachineCheckExceptionEntry)
    E500_MC_IRQ_ENTRY   archE500MachineCheckExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  数据存储异常入口(Vector IVOR2)
;*********************************************************************************************************/

FUNC_DEF(archE500DataStorageExceptionEntry)
    E500_IRQ_ENTRY      archE500DataStorageExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  指令访问异常入口(Vector IVOR3)
;*********************************************************************************************************/

FUNC_DEF(archE500InstructionStorageExceptionEntry)
    E500_IRQ_ENTRY      archE500InstructionStorageExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  外部中断入口(Vector IVOR4)
;*********************************************************************************************************/

FUNC_DEF(archE500ExternalInterruptEntry)
    E500_IRQ_ENTRY      bspIntHandle
    FUNC_END()

;/*********************************************************************************************************
;  非对齐异常入口(Vector IVOR5)
;*********************************************************************************************************/

FUNC_DEF(archE500AlignmentExceptionEntry)
    E500_IRQ_ENTRY      archE500AlignmentExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  程序异常入口(Vector IVOR6)
;*********************************************************************************************************/

FUNC_DEF(archE500ProgramExceptionEntry)
    E500_IRQ_ENTRY      archE500ProgramExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  系统调用入口(Vector IVOR8)
;*********************************************************************************************************/

FUNC_DEF(archE500SystemCallEntry)
    E500_IRQ_ENTRY      archE500SystemCallHandle
    FUNC_END()

;/*********************************************************************************************************
;  Decrementer 异常入口(Vector IVOR10)
;*********************************************************************************************************/

FUNC_DEF(archE500DecrementerInterruptEntry)
    E500_IRQ_ENTRY      archE500DecrementerInterruptHandle
    FUNC_END()

;/*********************************************************************************************************
;  Fixed-interval timer 异常入口(Vector IVOR11)
;*********************************************************************************************************/

FUNC_DEF(archE500TimerInterruptEntry)
    E500_IRQ_ENTRY      archE500TimerInterruptHandle
    FUNC_END()

;/*********************************************************************************************************
;  Watchdog timer 异常入口(Vector IVOR12)
;*********************************************************************************************************/

FUNC_DEF(archE500WatchdogInterruptEntry)
    E500_CI_IRQ_ENTRY   archE500WatchdogInterruptHandle
    FUNC_END()

;/*********************************************************************************************************
;  数据 TLB 错误入口(Vector IVOR13)
;*********************************************************************************************************/

FUNC_DEF(archE500DataTLBErrorEntry)
#if LW_CFG_VMM_EN > 0
    E500_IRQ_ENTRY      archE500MmuDataTlbErrorHandle
#else
    E500_IRQ_ENTRY      archE500DataTLBErrorHandle
#endif                                                                  /*  LW_CFG_VMM_EN > 0            */
    FUNC_END()

;/*********************************************************************************************************
;  指令 TLB 错误入口(Vector IVOR14)
;*********************************************************************************************************/

FUNC_DEF(archE500InstructionTLBErrorEntry)
#if LW_CFG_VMM_EN > 0
    E500_IRQ_ENTRY      archE500MmuInstructionTlbErrorHandle
#else
    E500_IRQ_ENTRY      archE500InstructionTLBErrorHandle
#endif                                                                  /*  LW_CFG_VMM_EN > 0            */
    FUNC_END()

;/*********************************************************************************************************
;  Debug 异常入口(Vector IVOR15)
;*********************************************************************************************************/

FUNC_DEF(archE500DebugInterruptEntry)
    E500_CI_IRQ_ENTRY   archE500DebugInterruptHandle
    FUNC_END()

;/*********************************************************************************************************
;  FPU 不可用入口(Vector IVOR32)
;*********************************************************************************************************/

FUNC_DEF(archE500FpuUnavailableExceptionEntry)
    E500_IRQ_ENTRY      archFpuUnavailableExceptionHandle
    FUNC_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
