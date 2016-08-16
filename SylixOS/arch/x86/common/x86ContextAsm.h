/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86ContextAsm.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 04 日
**
** 描        述: x86 体系构架上下文切换.
*********************************************************************************************************/

#ifndef __ARCH_X86CONTEXTASM_H
#define __ARCH_X86CONTEXTASM_H

#include "arch/x86/arch_regs.h"
#include "x86Segment.h"

/*********************************************************************************************************
  上下文恢复
*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    POPW    %GS
    POPW    %FS
    POPW    %ES
    POPW    %DS
    POPW    %SS

    ADDL    $2 , %ESP                                                   /*  PAD                         */

    POPL    %EAX
    POPL    %EBX
    POPL    %ECX
    POPL    %EDX

    POPL    %ESI
    POPL    %EDI

    POPL    %EBP

    ADDL    $4 , %ESP                                                   /*  不弹出 ERROR CODE           */

    IRET                                                                /*  IRET 等于弹出 CS EIP EFLAGS */
    MACRO_END()

/*********************************************************************************************************
  上下文保存
*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    PUSHF                                                               /*  PUSH EFLAGS                 */

    PUSHL   %CS

    PUSHL   $archResumePc                                               /*  PUSH EIP                    */

    PUSHL   $0                                                          /*  PUSH ERROR CODE             */

    PUSHL   %EBP

    PUSHL   %EDI
    PUSHL   %ESI

    PUSHL   %EDX
    PUSHL   %ECX
    PUSHL   %EBX
    PUSHL   %EAX

    SUBL    $2 , %ESP                                                   /*  PAD                         */

    PUSHW   %SS
    PUSHW   %DS
    PUSHW   %ES
    PUSHW   %FS
    PUSHW   %GS
    MACRO_END()

/*********************************************************************************************************
  异常/中断上下文保存
*********************************************************************************************************/

MACRO_DEF(INT_SAVE_REGS_HW_ERRNO)
                                                                        /*  EFLAGS CS EIP ERRNO 已经PUSH*/
    PUSHL   %EBP
    MOVL    %ESP , %EBP

    PUSHL   %EDI
    PUSHL   %ESI

    PUSHL   %EDX
    PUSHL   %ECX
    PUSHL   %EBX
    PUSHL   %EAX

    SUBL    $2 , %ESP                                                   /*  PAD                         */

    PUSHW   %SS
    PUSHW   %DS
    PUSHW   %ES
    PUSHW   %FS
    PUSHW   %GS

    /*
     * 设置正确的内核段描述符值
     */
    MOVW    $X86_BUILD_SEGMENT_REG_VALUE(0, 0, X86_SEG_KDATA), %DI

    MOV     %DI , %DS

    MOV     %DI , %ES

    MOV     %DI , %FS

    MOV     %DI , %GS
    /*
     * CS 和 SS 处理器已经加载完, 不用再设置了
     */
    MACRO_END()

/*********************************************************************************************************
  异常/中断上下文保存
*********************************************************************************************************/

MACRO_DEF(INT_SAVE_REGS_FAKE_ERRNO)
    PUSHL   $0                                                          /*  PUSH FAKE ERROR CODE        */

    INT_SAVE_REGS_HW_ERRNO
    MACRO_END()

#endif                                                                  /*  __ARCH_X86CONTEXTASM_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
