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
;** 文   件   名: arm64ExcAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 07 月 05 日
;**
;** 描        述: ARM64 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "arch/arm64/arch_regs.h"
#include "arm64ContextAsm.h"

    FILE_BEGIN()

;/*********************************************************************************************************
;  ARM 体系构架相关异常处理函数
;*********************************************************************************************************/

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)

    IMPORT_LABEL(bspIntHandle)
    IMPORT_LABEL(archSyncExcHandle)
    IMPORT_LABEL(archInvalidExcHandle)

    EXPORT_LABEL(archEL1IrqEntry)
    EXPORT_LABEL(archEL1SyncExcEntry)
    EXPORT_LABEL(archEL1SyncInvalidEntry)
    EXPORT_LABEL(archEL1IrqInvalidEntry)
    EXPORT_LABEL(archEL1FiqInvalidEntry)
    EXPORT_LABEL(archEL1ErrInvalidEntry)

;/*********************************************************************************************************
;  中断入口
;*********************************************************************************************************/

FUNC_DEF(archEL1IrqEntry)
    ;/*
    ; * 使用异常临时栈, 并在异常临时栈开辟临时上下文保存区, 将 volatile 寄存器保存到临时上下文保存区
    ; * SP 指向异常临时栈
    ; */
    EXC_SAVE_VOLATILE

    BL      API_InterEnter

    ;/*
    ; * 如果不是第一次进入中断, 跳转
    ; */
    CMP     X0 , #1
    BNE     1f

    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */

    ;/*
    ; * 拷贝 volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_SAVE_NON_VOLATILE

    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 SP
    ; */
    BL      API_InterStackBaseGet
    MOV     SP , X0

2:
    ;/*
    ; * bspIntHandle()
    ; */
    BL      bspIntHandle

    ;/*
    ; * API_InterExit()
    ; * 如果没有发生中断嵌套, 则 API_InterExit 会调用 archIntCtxLoad 函数
    ; */
    BL      API_InterExit

    ;/*
    ; * 来到这里, 说明发生了中断嵌套
    ; */
    MOV     X18 , SP
    RESTORE_BIG_REG_CTX                                                 ;/*  恢复所有寄存器              */

1:
    ;/*
    ; * 不是第一次进入中断
    ; */
    LDR     X0  , [SP, #XSP_OFFSET]                                    ;/*  获取异常前 SP               */
    SUB     X0  , X0 , ARCH_REG_CTX_SIZE                               ;/*  在异常堆栈开辟上下文保存区  */

    ;/*
    ; * 拷贝 volatile 寄存器到异常堆栈里的上下文保存区
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到异常堆栈里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     SP , X0                                                    ;/*  使用异常堆栈                */
    B       2b
    FUNC_END()
    
;/*********************************************************************************************************
;  同步异常入口
;*********************************************************************************************************/

FUNC_DEF(archEL1SyncExcEntry)
    ;/*
    ; * 使用异常临时栈, 并在异常临时栈开辟临时上下文保存区, 将 volatile 寄存器保存到临时上下文保存区
    ; * SP 指向异常临时栈
    ; */
    EXC_SAVE_VOLATILE

    BL      API_InterEnter
            
    ;/*
    ; * 如果不是第一次进入中断, 跳转
    ; */
    CMP     X0 , #1
    BNE     1f
    
    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */

    ;/*
    ; * 拷贝 volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     X18, X0
    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 SP
    ; */
    BL      API_InterStackBaseGet
    MOV     SP , X0

    MOV     X0 , X18
2:
    ;/*
    ; * archSyncExcHandle()
    ; */
    MRS     X1 , ESR_EL1    
    BL      archSyncExcHandle

    ;/*
    ; * API_InterExit()
    ; * 如果没有发生中断嵌套, 则 API_InterExit 会调用 archIntCtxLoad 函数
    ; */
    BL      API_InterExit

    ;/*
    ; * 来到这里, 说明发生了中断嵌套
    ; */
    MOV     X18 , SP
    RESTORE_BIG_REG_CTX                                                 ;/*  恢复所有寄存器              */

1:
    ;/*
    ; * 不是第一次进入中断
    ; */
    LDR     X0  , [SP, #XSP_OFFSET]                                    ;/*  获取异常前 SP               */
    SUB     X0  , X0 , ARCH_REG_CTX_SIZE                               ;/*  在异常堆栈开辟上下文保存区  */

    ;/*
    ; * 拷贝 volatile 寄存器到异常堆栈里的上下文保存区
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到异常堆栈里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     SP , X0                                                    ;/*  使用异常堆栈                */
    B       2b
    FUNC_END()

;/*********************************************************************************************************
;  Invalid 异常入口
;*********************************************************************************************************/

MACRO_DEF(ARCH_INVALID_EXC_ENTRY type)
    ;/*
    ; * 使用异常临时栈, 并在异常临时栈开辟临时上下文保存区, 将 volatile 寄存器保存到临时上下文保存区
    ; * SP 指向异常临时栈
    ; */
    EXC_SAVE_VOLATILE

    BL      API_InterEnter

    ;/*
    ; * 如果不是第一次进入中断, 跳转
    ; */
    CMP     X0 , #1
    BNE     1f

    ;/*
    ; * 获得当前任务 TCB 的 ARCH_REG_CTX 地址
    ; */
    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */

    ;/*
    ; * 拷贝 volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到当前任务 TCB 的 ARCH_REG_CTX 里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     X18, X0

    ;/*
    ; * 第一次进入中断: 获得当前 CPU 中断堆栈栈顶, 并设置 SP
    ; */
    BL      API_InterStackBaseGet
    MOV     SP , X0

    MOV     X0 , X18
2:
    ;/*
    ; * archInvalidExcHandle()
    ; */
    MRS     X1 , ESR_EL1
    MOV     X2 , \type
    BL      archInvalidExcHandle

    ;/*
    ; * 来到这里, 说明发生了中断嵌套
    ; */
    MOV     X18 , SP
    RESTORE_BIG_REG_CTX                                                 ;/*  恢复所有寄存器              */

1:
    ;/*
    ; * 不是第一次进入中断
    ; */
    LDR     X0  , [SP, #XSP_OFFSET]                                    ;/*  获取异常前 SP               */
    SUB     X0  , X0 , ARCH_REG_CTX_SIZE                               ;/*  在异常堆栈开辟上下文保存区  */

    ;/*
    ; * 拷贝 volatile 寄存器到异常堆栈里的上下文保存区
    ; */
    EXC_COPY_VOLATILE

    ;/*
    ; * 保存 non volatile 寄存器到异常堆栈里
    ; */
    EXC_SAVE_NON_VOLATILE

    MOV     SP , X0                                                    ;/*  使用异常堆栈                */
    B       2b
    MACRO_END()

FUNC_DEF(archEL1SyncInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL1_SYN_INVALID
    FUNC_END()

FUNC_DEF(archEL1IrqInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL1_IRQ_INVALID
    FUNC_END()

FUNC_DEF(archEL1FiqInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL1_FIQ_INVALID
    FUNC_END()

FUNC_DEF(archEL1ErrInvalidEntry)
    ARCH_INVALID_EXC_ENTRY      EL1_ERR_INVALID
    FUNC_END()

    FILE_END()
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

