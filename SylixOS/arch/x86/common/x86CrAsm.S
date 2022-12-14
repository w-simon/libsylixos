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
;** 文   件   名: x86CrAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 04 日
;**
;** 描        述: x86 体系构架处理器 CR 寄存器读写.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

    EXPORT_LABEL(x86Cr0Get)
    EXPORT_LABEL(x86Cr0Set)
    EXPORT_LABEL(x86Cr1Get)
    EXPORT_LABEL(x86Cr1Set)
    EXPORT_LABEL(x86Cr2Get)
    EXPORT_LABEL(x86Cr2Set)
    EXPORT_LABEL(x86Cr3Get)
    EXPORT_LABEL(x86Cr3Set)
    EXPORT_LABEL(x86Cr4Get)
    EXPORT_LABEL(x86Cr4Set)

;/*********************************************************************************************************
;  CR0 寄存器设置和获取
;*********************************************************************************************************/

FUNC_DEF(x86Cr0Get)
    MOVL    %CR0 , %EAX
    RET
    FUNC_END(x86Cr0Get)

FUNC_DEF(x86Cr0Set)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    MOVL    %EAX , %CR0
    RET
    FUNC_END(x86Cr0Set)

;/*********************************************************************************************************
;  CR1 寄存器设置和获取
;*********************************************************************************************************/

FUNC_DEF(x86Cr1Get)
    MOVL    %CR1 , %EAX
    RET
    FUNC_END(x86Cr1Get)

FUNC_DEF(x86Cr1Set)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    MOVL    %EAX , %CR1
    RET
    FUNC_END(x86Cr1Set)

;/*********************************************************************************************************
;  CR2 寄存器设置和获取
;*********************************************************************************************************/

FUNC_DEF(x86Cr2Get)
    MOVL    %CR2 , %EAX
    RET
    FUNC_END(x86Cr2Get)

FUNC_DEF(x86Cr2Set)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    MOVL    %EAX , %CR2
    RET
    FUNC_END(x86Cr2Set)

;/*********************************************************************************************************
;  CR3 寄存器设置和获取
;*********************************************************************************************************/

FUNC_DEF(x86Cr3Get)
    MOVL    %CR3 , %EAX
    RET
    FUNC_END(x86Cr3Get)

FUNC_DEF(x86Cr3Set)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    MOVL    %EAX , %CR3
    RET
    FUNC_END(x86Cr3Set)

;/*********************************************************************************************************
;  CR4 寄存器设置和获取
;*********************************************************************************************************/

FUNC_DEF(x86Cr4Get)
    MOVL    %CR4 , %EAX
    RET
    FUNC_END(x86Cr4Get)

FUNC_DEF(x86Cr4Set)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    MOVL    %EAX , %CR4
    RET
    FUNC_END(x86Cr4Set)

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
