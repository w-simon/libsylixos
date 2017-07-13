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
** ��   ��   ��: x64ContextAsm.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 06 �� 06 ��
**
** ��        ��: x86-64 ��ϵ�����������л�.
*********************************************************************************************************/

#ifndef __ARCH_X64CONTEXTASM_H
#define __ARCH_X64CONTEXTASM_H

/*********************************************************************************************************
  ͷ�ļ�
*********************************************************************************************************/

#include "arch/x86/arch_regs.h"
#include "../x86Segment.h"

/*********************************************************************************************************
    +-----------------+
    |       SS(8)     |
    +-----------------+
    |       RSP       |
    +-----------------+
    |     RFLAGS      |
    +-----------------+
    |       CS(8)     |
    +-----------------+
    |       RIP       |
    +-----------------+
    |      ERROR      |
    +-----------------+
    |       R15       |
    +-----------------+
    |       R14       |
    +-----------------+
    |       R13       |
    +-----------------+
    |       R12       |
    +-----------------+
    |       R11       |
    +-----------------+
    |       R10       |
    +-----------------+
    |       R9        |
    +-----------------+
    |       R8        |
    +-----------------+
    |       RBP       |
    +-----------------+
    |       RDI       |
    +-----------------+
    |       RSI       |
    +-----------------+
    |       RDX       |
    +-----------------+
    |       RCX       |
    +-----------------+
    |       RBX       |
    +-----------------+
    |       RAX       |
    +-----------------+
    |       DS(2)     |
    +-----------------+
    |       ES(2)     |
    +-----------------+
    |       FS(2)     |
    +-----------------+
    |       GS(2)     |
    +-----------------+

*********************************************************************************************************/

#define __SAVE_SEG_REG  0                                               /*  ������μĴ���              */

/*********************************************************************************************************
  �����Ļָ�
*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
#if __SAVE_SEG_REG > 0
    MOVW    ARCH_REG_OFFSET_GS(%RSP)  , %GS
    MOVW    ARCH_REG_OFFSET_FS(%RSP)  , %FS
    MOVW    ARCH_REG_OFFSET_ES(%RSP)  , %ES
    MOVW    ARCH_REG_OFFSET_DS(%RSP)  , %DS
#endif                                                                  /*  __SAVE_SEG_REG > 0          */

    MOV     ARCH_REG_OFFSET_RAX(%RSP) , %RAX
    MOV     ARCH_REG_OFFSET_RBX(%RSP) , %RBX
    MOV     ARCH_REG_OFFSET_RCX(%RSP) , %RCX
    MOV     ARCH_REG_OFFSET_RDX(%RSP) , %RDX
    MOV     ARCH_REG_OFFSET_RSI(%RSP) , %RSI
    MOV     ARCH_REG_OFFSET_RDI(%RSP) , %RDI
    MOV     ARCH_REG_OFFSET_RBP(%RSP) , %RBP

    MOV     ARCH_REG_OFFSET_R8(%RSP)  , %R8
    MOV     ARCH_REG_OFFSET_R9(%RSP)  , %R9
    MOV     ARCH_REG_OFFSET_R10(%RSP) , %R10
    MOV     ARCH_REG_OFFSET_R11(%RSP) , %R11
    MOV     ARCH_REG_OFFSET_R12(%RSP) , %R12
    MOV     ARCH_REG_OFFSET_R13(%RSP) , %R13
    MOV     ARCH_REG_OFFSET_R14(%RSP) , %R14
    MOV     ARCH_REG_OFFSET_R15(%RSP) , %R15

    ADD     $ARCH_REG_OFFSET_RIP , %RSP                                 /*  ������ ERROR CODE           */

    IRETQ                                                               /*  IRETQ �ᵯ�� CS RIP RFLAGS  */
                                                                        /*  SS RSP                      */
    MACRO_END()

/*********************************************************************************************************
  �����ı���
*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    MOV     %RSP , %RAX                                                 /*  �ݴ� RSP                    */
    AND     $~15 , %RSP                                                 /*  RSP ���� 16 �ֽڶ���        */

    MOV     %SS  , %RCX
    PUSH    %RCX                                                        /*  PUSH SS                     */
    PUSH    %RAX                                                        /*  PUSH OLD RSP                */
    PUSHFQ                                                              /*  PUSH RFLAGS                 */

    SUB     $((ARCH_REG_CTX_WORD_SIZE - 3) * 8) , %RSP

#if __SAVE_SEG_REG > 0
    MOVW    %GS ,  ARCH_REG_OFFSET_GS(%RSP)
    MOVW    %FS ,  ARCH_REG_OFFSET_FS(%RSP)
    MOVW    %ES ,  ARCH_REG_OFFSET_ES(%RSP)
    MOVW    %DS ,  ARCH_REG_OFFSET_DS(%RSP)
#endif                                                                  /*  __SAVE_SEG_REG > 0          */

    MOV     %RAX , ARCH_REG_OFFSET_RAX(%RSP)
    MOV     %RBX , ARCH_REG_OFFSET_RBX(%RSP)
    MOV     %RCX , ARCH_REG_OFFSET_RCX(%RSP)
    MOV     %RDX , ARCH_REG_OFFSET_RDX(%RSP)
    MOV     %RSI , ARCH_REG_OFFSET_RSI(%RSP)
    MOV     %RDI , ARCH_REG_OFFSET_RDI(%RSP)
    MOV     %RBP , ARCH_REG_OFFSET_RBP(%RSP)

    MOV     %R8  , ARCH_REG_OFFSET_R8(%RSP)
    MOV     %R9  , ARCH_REG_OFFSET_R9(%RSP)
    MOV     %R10 , ARCH_REG_OFFSET_R10(%RSP)
    MOV     %R11 , ARCH_REG_OFFSET_R11(%RSP)
    MOV     %R12 , ARCH_REG_OFFSET_R12(%RSP)
    MOV     %R13 , ARCH_REG_OFFSET_R13(%RSP)
    MOV     %R14 , ARCH_REG_OFFSET_R14(%RSP)
    MOV     %R15 , ARCH_REG_OFFSET_R15(%RSP)

    MOV     $0   , %RAX
    MOV     %RAX , ARCH_REG_OFFSET_ERROR(%RSP)
    MOV     $archResumePc , %RAX
    MOV     %RAX , ARCH_REG_OFFSET_RIP(%RSP)
    MOVW    %CS  , ARCH_REG_OFFSET_CS(%RSP)
    MACRO_END()

/*********************************************************************************************************
  �쳣/�ж������ı���
*********************************************************************************************************/

MACRO_DEF(INT_SAVE_REGS_HW_ERRNO)
    CLI
                                                                        /*  SS RSP RFLAGS CS RIP ERRNO  */
                                                                        /*  �Ѿ� PUSH                   */
    SUB     $((ARCH_REG_CTX_WORD_SIZE - 6) * 8) , %RSP

#if __SAVE_SEG_REG > 0
    MOVW    %GS ,  ARCH_REG_OFFSET_GS(%RSP)
    MOVW    %FS ,  ARCH_REG_OFFSET_FS(%RSP)
    MOVW    %ES ,  ARCH_REG_OFFSET_ES(%RSP)
    MOVW    %DS ,  ARCH_REG_OFFSET_DS(%RSP)
#endif                                                                  /*  __SAVE_SEG_REG > 0          */

    MOV     %RAX , ARCH_REG_OFFSET_RAX(%RSP)
    MOV     %RBX , ARCH_REG_OFFSET_RBX(%RSP)
    MOV     %RCX , ARCH_REG_OFFSET_RCX(%RSP)
    MOV     %RDX , ARCH_REG_OFFSET_RDX(%RSP)
    MOV     %RSI , ARCH_REG_OFFSET_RSI(%RSP)
    MOV     %RDI , ARCH_REG_OFFSET_RDI(%RSP)
    MOV     %RBP , ARCH_REG_OFFSET_RBP(%RSP)

    MOV     %R8  , ARCH_REG_OFFSET_R8(%RSP)
    MOV     %R9  , ARCH_REG_OFFSET_R9(%RSP)
    MOV     %R10 , ARCH_REG_OFFSET_R10(%RSP)
    MOV     %R11 , ARCH_REG_OFFSET_R11(%RSP)
    MOV     %R12 , ARCH_REG_OFFSET_R12(%RSP)
    MOV     %R13 , ARCH_REG_OFFSET_R13(%RSP)
    MOV     %R14 , ARCH_REG_OFFSET_R14(%RSP)
    MOV     %R15 , ARCH_REG_OFFSET_R15(%RSP)
    MACRO_END()

/*********************************************************************************************************
  �쳣/�ж������ı���
*********************************************************************************************************/

MACRO_DEF(INT_SAVE_REGS_FAKE_ERRNO)
    CLI

    PUSH    $0                                                          /*  PUSH FAKE ERROR CODE        */

    INT_SAVE_REGS_HW_ERRNO
    MACRO_END()

#endif                                                                  /*  __ARCH_X64CONTEXTASM_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
