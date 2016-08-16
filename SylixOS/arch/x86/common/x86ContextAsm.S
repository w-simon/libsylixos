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
;** 文   件   名: x86ContextAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 04 日
;**
;** 描        述: x86 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include "x86ContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif

    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
    EXPORT_LABEL(archCrtCtxSwitch)
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)

;/*********************************************************************************************************
;  archResumePc
;*********************************************************************************************************/

FUNC_DEF(archResumePc)
    RET
    FUNC_END(archResumePc)

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 X86_SP_ARG1 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    MOVL    X86_SP_ARG1(%ESP) , %EDX                                    ;/*  EDX = 当前 CPU 控制块指针   */

    MOVL    0(%EDX) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */

    MOVL    0(%EAX) , %ESP                                              ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archTaskCtxStart)

;/*********************************************************************************************************
;  线程切换 
;  参数为当前 CPU 控制块, 即 X86_SP_ARG1 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    MOVL    (X86_SP_ARG1 + ARCH_REG_CTX_WORD_SIZE * 4)(%ESP) , %EDX     ;/*  EDX = 当前 CPU 控制块指针   */
    MOVL    0(%EDX) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    %ESP , 0(%EAX)                                              ;/*  保存 ESP                    */

    PUSHL   %EDX                                                        ;/*  保存当前 CPU 控制块指针     */
    PUSHL   %EDX                                                        ;/*  参数 = 当前 CPU 控制块指针  */
    CALL    _SchedSwp                                                   ;/*  _SchedSwp();                */
    ADDL    $4 , %ESP
    POPL    %EDX                                                        ;/*  EDX = 当前 CPU 控制块指针   */

    MOVL    0(%EDX) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    0(%EAX) , %ESP                                              ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archTaskCtxSwitch)

;/*********************************************************************************************************
;  协程切换 
;  参数为当前 CPU 控制块, 即 X86_SP_ARG1 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    MOVL    (X86_SP_ARG1 + ARCH_REG_CTX_WORD_SIZE * 4)(%ESP) , %EDX     ;/*  EDX = 当前 CPU 控制块指针   */
    MOVL    0(%EDX) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    %ESP , 0(%EAX)                                              ;/*  保存 ESP                    */

    PUSHL   %EDX                                                        ;/*  保存当前 CPU 控制块指针     */
    PUSHL   %EDX                                                        ;/*  参数 = 当前 CPU 控制块指针  */
    CALL    _SchedCrSwp                                                 ;/*  _SchedCrSwp();              */
    ADDL    $4 , %ESP
    POPL    %EDX                                                        ;/*  EDX = 当前 CPU 控制块指针   */

    MOVL    0(%EDX) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    0(%EAX) , %ESP                                              ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archCrtCtxSwitch)
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载 
;  参数为当前 CPU 控制块, 即 X86_SP_ARG1 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    PUSHL   X86_SP_ARG1(%ESP)                                           ;/*  保存当前 CPU 控制块指针     */
    PUSHL   (X86_SP_ARG1 + 4)(%ESP)                                     ;/*  参数 = 当前 CPU 控制块指针  */
    CALL    _SchedSwp                                                   ;/*  _SchedSwp();                */
    ADDL    $4 , %ESP
    POPL    %EDX                                                        ;/*  EDX = 当前 CPU 控制块指针   */

    MOVL    0(%EDX) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    0(%EAX) , %ESP                                              ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archIntCtxLoad)

;/*********************************************************************************************************
;  信号上下文返回
;  参数为需要返回的堆栈栈顶位置
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOVL    X86_SP_ARG1(%ESP) , %EDX                                    ;/*  EDX = 传入的参数            */

    MOVL    %EDX , %ESP                                                 ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archSigCtxLoad)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
