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
;** 文   件   名: arm64ContextAsm.S
;**
;** 创   建   人: Wang.Xuan (王翾)
;**
;** 文件创建日期: 2018 年 06 月 23 日
;**
;** 描        述: ARM64 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>

#include "arm64ContextAsm.h"

    FILE_BEGIN()

;/*********************************************************************************************************
;  ARM 体系构架相关异常处理函数
;*********************************************************************************************************/

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
;  参数为当前 CPU 控制块, 即 X0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    MOV     X1 , SP
    BIC     X1 , X1 , #0xF                                              ;/*  SP 向下 16 字节对齐         */
    MOV     SP , X1
    MSR     TPIDR_EL1, X1                                               ;/*  设置异常临时栈(使用启动栈)  */

    LDR     X18, [X0]                                                   ;/*  获取当前 TCB 的 REG_CTX 地址*/

LINE_LABEL(archTaskCtxLoad)
    RESTORE_SMALL_REG_CTX                                               ;/*  恢复小寄存器上下文          */
    FUNC_END()

;/*********************************************************************************************************
;  线程切换 
;  参数为当前 CPU 控制块, 即 X0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    LDR     X18, [X0]                                                   ;/*  获取当前 TCB 的 REG_CTX 地址*/
    
    SAVE_SMALL_REG_CTX                                                  ;/*  保存小寄存器上下文          */

    MOV     X9 , X0                                                     ;/*  X9  暂存 X0                 */

#if LW_CFG_SMP_EN > 0    
    BL      _SchedSafeStack                                             ;/*  _SchedSafeStack();          */
    MOV     SP , X0                                                     ;/*  恢复 X0                     */
    MOV     X0 , X9
#endif

    BL      _SchedSwp                                                   ;/*  _SchedSwp();                */

    LDR     X18, [X9]                                                   ;/*  获取当前 TCB 的 REG_CTX 地址*/
    LDR     X9 , [X18, #CTX_TYPE_OFFSET]                                ;/*  获得上下文类型              */
    CMP     X9 , #0
    B.NE    _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  恢复大寄存器上下文          */

LINE_LABEL(_RestoreSmallCtx)
    RESTORE_SMALL_REG_CTX                                               ;/*  恢复小寄存器上下文          */
    FUNC_END()

;/*********************************************************************************************************
;  协程切换 
;  参数为当前 CPU 控制块, 即 X0 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    LDR     X18 , [X0]                                                  ;/*  获取当前 CCB 的 REG_CTX 地址*/
    SAVE_SMALL_REG_CTX                                                  ;/*  保存小寄存器上下文          */

    MOV     X9  , X0                                                    ;/*  X9 暂存 X0                  */

    BL      _SchedSwp                                                   ;/*  _SchedSwp();                */

    LDR     X18 , [X9]                                                  ;/*  获取当前 CCB 的 REG_CTX 地址*/
    RESTORE_SMALL_REG_CTX                                               ;/*  恢复小寄存器上下文          */
    FUNC_END()
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载 
;  参数为当前 CPU 控制块, 即 X0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    LDR     X18 , [X0]                                                  ;/*  获取当前 TCB 的 REG_CTX 地址*/
    LDR     X9  , [X18, #CTX_TYPE_OFFSET]                               ;/*  获得上下文类型              */
    CMP     X9  , #0
    B.NE    _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  恢复大寄存器上下文          */
    FUNC_END()

;/*********************************************************************************************************
;  信号上下文返回
;  参数为 ARCH_REG_CTX 指针
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOV     X18 , X0
    LDR     X9  , [X18, #CTX_TYPE_OFFSET]                               ;/*  获得上下文类型              */
    CMP     X9  , #0
    B.NE    _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  恢复大寄存器上下文          */
    FUNC_END()

;/*********************************************************************************************************
;  拷贝任务上下文
;  参数 X0 为目的 ARCH_REG_CTX 指针, X1 为源 ARCH_REG_CTX 指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxCopy)
    LDR     X9 ,       [X1 , #CTX_TYPE_OFFSET]
    STR     X9 ,       [X1 , #CTX_TYPE_OFFSET]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(0)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(0)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(2)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(2)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(4)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(4)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(6)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(6)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(8)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(8)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(10)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(10)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(12)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(12)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(14)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(14)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(16)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(16)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(18)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(18)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(20)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(20)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(22)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(22)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(24)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(24)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(26)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(26)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(28)]
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(28)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(30)]                         ;/*  LR、PC 寄存器               */
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(30)]

    LDP     X9 , X10 , [X1 , #XGREG_OFFSET(32)]                         ;/*  SP、PSTATE 寄存器           */
    STP     X9 , X10 , [X0 , #XGREG_OFFSET(32)]
    
    RET
    FUNC_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
