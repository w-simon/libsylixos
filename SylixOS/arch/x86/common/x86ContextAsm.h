/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: x86ContextAsm.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 04 ��
**
** ��        ��: x86 ��ϵ�����������л�.
*********************************************************************************************************/

#ifndef __ARCH_X86CONTEXTASM_H
#define __ARCH_X86CONTEXTASM_H

#include "arch/x86/arch_regs.h"
#include "x86Segment.h"

/*********************************************************************************************************
  �����Ļָ�
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

    ADDL    $4 , %ESP                                                   /*  ������ ERROR CODE           */

    IRET                                                                /*  IRET ���ڵ��� CS EIP EFLAGS */
    MACRO_END()

/*********************************************************************************************************
  �����ı���
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
  �쳣/�ж������ı���
*********************************************************************************************************/

MACRO_DEF(INT_SAVE_REGS_HW_ERRNO)
                                                                        /*  EFLAGS CS EIP ERRNO �Ѿ�PUSH*/
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
     * ������ȷ���ں˶�������ֵ
     */
    MOVW    $X86_BUILD_SEGMENT_REG_VALUE(0, 0, X86_SEG_KDATA), %DI

    MOV     %DI , %DS

    MOV     %DI , %ES

    MOV     %DI , %FS

    MOV     %DI , %GS
    /*
     * CS �� SS �������Ѿ�������, ������������
     */
    MACRO_END()

/*********************************************************************************************************
  �쳣/�ж������ı���
*********************************************************************************************************/

MACRO_DEF(INT_SAVE_REGS_FAKE_ERRNO)
    PUSHL   $0                                                          /*  PUSH FAKE ERROR CODE        */

    INT_SAVE_REGS_HW_ERRNO
    MACRO_END()

#endif                                                                  /*  __ARCH_X86CONTEXTASM_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
