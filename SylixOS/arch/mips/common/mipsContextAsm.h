;/*********************************************************************************************************
;**
;**                                    �й������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: mipsContextAsm.h
;**
;** ��   ��   ��: Ryan.Xin (�Ž���)
;**
;** �ļ���������: 2015 �� 09 �� 01 ��
;**
;** ��        ��: MIPS ��ϵ���������Ĵ���.
;*********************************************************************************************************/

#ifndef __MIPSCONTEXTASM_H
#define __MIPSCONTEXTASM_H

#include "arch/mips/arch_regs.h"

;/*********************************************************************************************************
;  ���� $0 - $31 �Ĵ���
;*********************************************************************************************************/

MACRO_DEF(SAVE_GREGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SW      $0  , 0  * 4(SP)
    SW      $1  , 1  * 4(SP)
    SW      $2  , 2  * 4(SP)
    SW      $3  , 3  * 4(SP)
    SW      $4  , 4  * 4(SP)
    SW      $5  , 5  * 4(SP)
    SW      $6  , 6  * 4(SP)
    SW      $7  , 7  * 4(SP)
    SW      $8  , 8  * 4(SP)
    SW      $9  , 9  * 4(SP)
    SW      $10 , 10 * 4(SP)
    SW      $11 , 11 * 4(SP)
    SW      $12 , 12 * 4(SP)
    SW      $13 , 13 * 4(SP)
    SW      $14 , 14 * 4(SP)
    SW      $15 , 15 * 4(SP)
    SW      $16 , 16 * 4(SP)
    SW      $17 , 17 * 4(SP)
    SW      $18 , 18 * 4(SP)
    SW      $19 , 19 * 4(SP)
    SW      $20 , 20 * 4(SP)
    SW      $21 , 21 * 4(SP)
    SW      $22 , 22 * 4(SP)
    SW      $23 , 23 * 4(SP)
    SW      $24 , 24 * 4(SP)
    SW      $25 , 25 * 4(SP)
    ;/*
    ; * $26 $27 �� K0 K1
    ; */
    SW      $28 , 28 * 4(SP)
    SW      $29 , 29 * 4(SP)
    SW      $30 , 30 * 4(SP)
    SW      $31 , 31 * 4(SP)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  �ָ� $0 - $31 �Ĵ���
;*********************************************************************************************************/

MACRO_DEF(RESTORE_GREGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    ;/*
    ; * $0 �̶�Ϊ 0
    ; */
    LW      $1  , 1  * 4(SP)
    LW      $2  , 2  * 4(SP)
    LW      $3  , 3  * 4(SP)
    LW      $4  , 4  * 4(SP)
    LW      $5  , 5  * 4(SP)
    LW      $6  , 6  * 4(SP)
    LW      $7  , 7  * 4(SP)
    LW      $8  , 8  * 4(SP)
    ;/*
    ; * $9 �� T1(�����ָ�)
    ; */
    LW      $10 , 10 * 4(SP)
    LW      $11 , 11 * 4(SP)
    LW      $12 , 12 * 4(SP)
    LW      $13 , 13 * 4(SP)
    LW      $14 , 14 * 4(SP)
    LW      $15 , 15 * 4(SP)
    LW      $16 , 16 * 4(SP)
    LW      $17 , 17 * 4(SP)
    LW      $18 , 18 * 4(SP)
    LW      $19 , 19 * 4(SP)
    LW      $20 , 20 * 4(SP)
    LW      $21 , 21 * 4(SP)
    LW      $22 , 22 * 4(SP)
    LW      $23 , 23 * 4(SP)
    LW      $24 , 24 * 4(SP)
    LW      $25 , 25 * 4(SP)
    ;/*
    ; * $26 $27 �� K0 K1
    ; */
    LW      $28 , 28 * 4(SP)
    ;/*
    ; * $29 �� SP
    ; */
    LW      $30 , 30 * 4(SP)
    LW      $31 , 31 * 4(SP)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  ����������
;*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SUBU    SP , STK_CTX_SIZE                                           ;/*  �޸� SP                     */

    SW      ZERO , 0(SP)
    SW      ZERO , STK_OFFSET_LAST(SP)
    SYNC
    ;/*
    ; * ���治���ٷ��� TLB �����쳣
    ; */

    SAVE_GREGS                                                          ;/*  ���� $0 - $31 �Ĵ���        */

    SW      RA , STK_OFFSET_EPC(SP)                                     ;/*  RA ���� EPC ����            */

    MFC0(T1, CP0_STATUS)                                                ;/*  ���� STATUS �Ĵ���          */
    SW      T1 , STK_OFFSET_SR(SP)

    MFC0(T1, CP0_BADVADDR)                                              ;/*  ���� BADVADDR �Ĵ���        */
    SW      T1 , STK_OFFSET_BADVADDR(SP)

    MFC0(T1, CP0_CAUSE)                                                 ;/*  ���� CAUSE �Ĵ���           */
    SW      T1 , STK_OFFSET_CAUSE(SP)

    MFLO    T1                                                          ;/*  ���� LO �Ĵ���              */
    SW      T1 , STK_OFFSET_LO(SP)

    MFHI    T1                                                          ;/*  ���� HI �Ĵ���              */
    SW      T1 , STK_OFFSET_HI(SP)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  �ָ�������
;*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    LW      ZERO , 0(SP)
    LW      ZERO , STK_OFFSET_LAST(SP)
    SYNC
    ;/*
    ; * ���治���ٷ��� TLB �����쳣
    ; */

    RESTORE_GREGS                                                       ;/*  �ָ� $0 - $31 �Ĵ���        */

    LW      T1 , STK_OFFSET_LO(SP)                                      ;/*  �ָ� LO �Ĵ���              */
    MTLO    T1

    LW      T1 , STK_OFFSET_HI(SP)                                      ;/*  �ָ� HI �Ĵ���              */
    MTHI    T1

    LW      T1 , STK_OFFSET_CAUSE(SP)                                   ;/*  �ָ� CAUSE �Ĵ���           */
    MTC0(T1, CP0_CAUSE)

    LW      T1 , STK_OFFSET_BADVADDR(SP)                                ;/*  �ָ� BADVADDR �Ĵ���        */
    MTC0(T1, CP0_BADVADDR)

    LW      T1 , REG_T1 * 4(SP)                                         ;/*  �ָ� T1 �Ĵ���              */

    LW      K0 , STK_OFFSET_SR(SP)                                      ;/*  �ָ� SR  �Ĵ���             */
    LW      K1 , STK_OFFSET_EPC(SP)                                     ;/*  �ָ� EPC �Ĵ���             */

    ADDU    SP , STK_CTX_SIZE                                           ;/*  �޸� SP                     */

    ORI     K0 , K0 , M_StatusEXL                                       ;/*  ͨ������ EXL λ             */

    MTC0    K1 , CP0_EPC                                                ;/*  �ָ� EPC �Ĵ���             */
    MTC0    K0 , CP0_STATUS                                             ;/*  �����ں�ģʽ�������ж�      */
    EHB
    ERET                                                                ;/*  ��� EXL λ������           */
    NOP

    .set    pop
    MACRO_END()

#endif                                                                  /*  __MIPSCONTEXTASM_H          */
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
