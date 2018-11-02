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
;** 文   件   名: cskyContextAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 05 月 11 日
;**
;** 描        述: C-SKY 体系架构上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>
#include "cskyContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_SMP_EN > 0
    IMPORT_LABEL(_SchedSafeStack)
#endif
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif

    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
    EXPORT_LABEL(archTaskCtxCopy)
#if LW_CFG_COROUTINE_EN > 0
    EXPORT_LABEL(archCrtCtxSwitch)
#endif
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    LD.W        A1 , (A0 , 0)                                           ;/*  获取当前 TCB 的 REG_CTX 地址*/
    
    RESTORE_REGS                                                        ;/*  恢复所有寄存器              */
    FUNC_END(archTaskCtxStart)

;/*********************************************************************************************************
;  线程切换
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    LD.W        A1 , (A0 , 0)                                           ;/*  获取当前 TCB 的 REG_CTX 地址*/

    SAVE_REGS                                                           ;/*  保存所有寄存器              */

#if LW_CFG_SMP_EN > 0
    SUBI        SP , ARCH_REG_SIZE
    ST.W        A0 , (SP)
    BSR         _SchedSafeStack                                         ;/*  _SchedSafeStack();          */
    LD.W        A2 , (SP)                                               ;/*  当前 CPU 信息出栈           */
    MOV         SP , A0
    MOV         A0 , A2
#endif

    SUBI        SP , ARCH_REG_SIZE
    ST.W        A0 , (SP)                                               ;/*  当前 CPU 信息入栈           */
    BSR         _SchedSwp                                               ;/*  _SchedSwp();                */   
    LD.W        A0 , (SP)                                               ;/*  当前 CPU 信息出栈           */
    ADDI        SP , ARCH_REG_SIZE
    
    LD.W        A1 , (A0 , 0)                                           ;/*  获取当前 TCB 的 REG_CTX 地址*/
    
    RESTORE_REGS                                                        ;/*  恢复所有寄存器              */
    FUNC_END(archTaskCtxSwitch)

;/*********************************************************************************************************
;  协程切换
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    LD.W        A1 , (A0 , (2 * PTRSIZE))                               ;/*  当前 CCB 的 REG_CTX 地址    */
    
    SAVE_REGS                                                           ;/*  保存所有寄存器              */
    
    SUBI        SP , ARCH_REG_SIZE
    ST.W        A0 , (SP)                                               ;/*  当前 CPU 信息入栈           */
    BSR         _SchedCrSwp                                             ;/*  _SchedCrSwp();              */    
    LD.W        A0 , (SP)                                               ;/*  当前 CPU 信息出栈           */
    ADDI        SP , ARCH_REG_SIZE
    
    LD.W        A1 , (A0 , (2 * PTRSIZE))                               ;/*  当前 CCB 的 REG_CTX 地址    */

    RESTORE_REGS                                                        ;/*  恢复所有寄存器              */    
    FUNC_END(archCrtCtxSwitch)
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    LD.W        A1 , (A0 , 0)                                           ;/*  获取当前 TCB 的 REG_CTX 地址*/

    RESTORE_REGS                                                        ;/*  恢复所有寄存器              */
    FUNC_END(archIntCtxLoad)

;/*********************************************************************************************************
;  信号上下文返回
;  参数为 ARCH_REG_CTX 指针
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOV         A1 , A0
    
    RESTORE_REGS                                                        ;/*  恢复所有寄存器              */
    FUNC_END(archSigCtxLoad)

;/*********************************************************************************************************
;  拷贝任务上下文
;  参数 A0 为目的 ARCH_REG_CTX 指针, A1 为源 ARCH_REG_CTX 指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxCopy)
    LD.W        A2 , (A1 , (0 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (0 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (1 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (1 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (2 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (2 * ARCH_REG_SIZE))

    LD.W        A2 , (A1 , (3 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (3 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (4 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (4 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (5 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (5 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (6 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (6 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (7 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (7 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (8 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (8 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (9 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (9 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (10 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (10 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (11 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (11 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (12 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (12 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (13 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (13 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (14 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (14 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (15 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (15 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (16 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (16 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (17 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (17 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (18 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (18 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (19 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (19 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (20 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (20 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (21 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (21 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (22 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (22 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (23 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (23 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (24 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (24 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (25 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (25 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (26 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (26 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (27 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (27 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (28 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (28 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (29 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (29 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (30 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (30 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (31 * ARCH_REG_SIZE))
    ST.W        A2 , (A0 , (31 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (32 * ARCH_REG_SIZE))                        ;/*   PC                         */
    ST.W        A2 , (A0 , (32 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (33 * ARCH_REG_SIZE))                        ;/*   PSR                        */
    ST.W        A2 , (A0 , (33 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (34 * ARCH_REG_SIZE))                        ;/*   LO                         */
    ST.W        A2 , (A0 , (34 * ARCH_REG_SIZE))
    
    LD.W        A2 , (A1 , (35 * ARCH_REG_SIZE))                        ;/*   HI                         */
    ST.W        A2 , (A0 , (35 * ARCH_REG_SIZE))

    LD.W        A2 , (A1 , (36 * ARCH_REG_SIZE))                        ;/*   MEH                        */
    ST.W        A2 , (A0 , (36 * ARCH_REG_SIZE))

    RTS
    FUNC_END(archTaskCtxCopy)
    
    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
