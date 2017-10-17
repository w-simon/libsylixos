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
;** ��   ��   ��: sparcContextAsm.h
;**
;** ��   ��   ��: Xu.Guizhou (�����)
;**
;** �ļ���������: 2017 �� 05 �� 15 ��
;**
;** ��        ��: SPARC ��ϵ�����������л�.
;*********************************************************************************************************/

#ifndef __ARCH_SPARC_CTX_ASM_H
#define __ARCH_SPARC_CTX_ASM_H

;/*********************************************************************************************************
;  ���� CPU ������
;*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    SUB     %sp  , ASM_REG_STACK_FRAME_SIZE * 2 , %g1                   ;/*  ����ջ�ռ�                  */

    ST      %g1  , [%g1 + REG_GLOBAL(1)]                                ;/*  ����ȫ�ּĴ���(���� %g0)    */
    STD     %g2  , [%g1 + REG_GLOBAL(2)]
    STD     %g4  , [%g1 + REG_GLOBAL(4)]
    STD     %g6  , [%g1 + REG_GLOBAL(6)]

    STD     %l0  , [%g1 + REG_LOCAL(0)]                                 ;/*  ���汾�ؼĴ���              */
    STD     %l2  , [%g1 + REG_LOCAL(2)]
    STD     %l4  , [%g1 + REG_LOCAL(4)]
    STD     %l6  , [%g1 + REG_LOCAL(6)]

    STD     %i0  , [%g1 + REG_INPUT(0)]                                 ;/*  ��������Ĵ���              */
    STD     %i2  , [%g1 + REG_INPUT(2)]
    STD     %i4  , [%g1 + REG_INPUT(4)]
    STD     %i6  , [%g1 + REG_INPUT(6)]

    STD     %o0  , [%g1 + REG_OUTPUT(0)]                                ;/*  ��������Ĵ���              */
    STD     %o2  , [%g1 + REG_OUTPUT(2)]
    STD     %o4  , [%g1 + REG_OUTPUT(4)]
    STD     %o6  , [%g1 + REG_OUTPUT(6)]

    RD      %y   , %l0
    ST      %l0  , [%g1 + REG_Y]                                        ;/*  ���� Y �Ĵ���               */

    ADD     %o7  , 0x8  , %o7                                           ;/*  %o7 = CALL ָ��ĵ�ַ       */
    ST      %o7  , [%g1 + REG_PC]                                       ;/*  ���� PC (��ʱ�۵���һ��ָ��)*/

    ADD     %o7  , 0x4  , %o7                                           ;/*  Next PC = PC + 4            */
    ST      %o7  , [%g1 + REG_NPC]                                      ;/*  ���� Next PC                */

    RD      %psr , %l0
    ST      %l0  , [%g1 + REG_PSR]                                      ;/*  ���� PSR ״̬�Ĵ���         */
    MACRO_END()

;/*********************************************************************************************************
;  ���� CPU ������
;  %g1 Ϊ��Ҫ���صĶ�ջջ��λ��
;*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    RD      %psr , %g3
    AND     %g3  , SPARC_PSR_CWP_MASK , %g3                             ;/*  %g3 = CWP                   */

    LD      [%g1 + REG_PSR] , %l0                                       ;/*  %l0 = ���ָ��� %psr         */
    ANDN    %l0  , PSR_ET   , %l0                                       ;/*  �����쳣                    */
    ANDN    %l0  , SPARC_PSR_CWP_MASK , %l0                             ;/*  ��� CWP ����               */
    OR      %l0  , %g3 , %l0                                            ;/*  ʹ�õ�ǰ�� CWP              */
    WR      %l0  , %psr                                                 ;/*  �ָ� %psr                   */
    NOP
    NOP
    NOP

    ADD     %g3  , 1 , %g3                                              ;/*  %g3 = (CWP + 1) % NWIN      */
    AND     %g3  , LW_CFG_CPU_REG_WIN_NR - 1 , %g3

    MOV     1    , %g4
    SLL     %g4  , %g3 , %g4                                            ;/*  %wim = %g4 = 1 << %g3       */
    WR      %g4  , %wim
    NOP
    NOP
    NOP

    LD      [%g1 + REG_Y] , %g4
    WR      %g4  , %y                                                   ;/*  �ָ� Y �Ĵ���               */

    LDD     [%g1 + REG_GLOBAL(2)] , %g2                                 ;/*  �ָ�ȫ�ּĴ���              */
    LDD     [%g1 + REG_GLOBAL(4)] , %g4
    LDD     [%g1 + REG_GLOBAL(6)] , %g6

    LDD     [%g1 + REG_OUTPUT(0)] , %o0                                 ;/*  �ָ�����Ĵ���              */
    LDD     [%g1 + REG_OUTPUT(2)] , %o2
    LDD     [%g1 + REG_OUTPUT(4)] , %o4
    LDD     [%g1 + REG_OUTPUT(6)] , %o6

    LDD     [%g1 + REG_LOCAL(0)]  , %l0                                 ;/*  �ָ����ؼĴ���              */
    LDD     [%g1 + REG_LOCAL(2)]  , %l2
    LDD     [%g1 + REG_LOCAL(4)]  , %l4
    LDD     [%g1 + REG_LOCAL(6)]  , %l6

    LDD     [%g1 + REG_INPUT(0)]  , %i0                                 ;/*  �ָ�����Ĵ���              */
    LDD     [%g1 + REG_INPUT(2)]  , %i2
    LDD     [%g1 + REG_INPUT(4)]  , %i4
    LDD     [%g1 + REG_INPUT(6)]  , %i6

    SAVE                                                                ;/*  ���뵽һ�� dummy ����       */

    LD      [%g1 + REG_PC]        , %l1                                 ;/*  %l1 �� dummy ����!          */
    LD      [%g1 + REG_NPC]       , %l2                                 ;/*  %l2 �� dummy ����!          */
    LD      [%g1 + REG_GLOBAL(1)] , %g1                                 ;/*  ���ָ� %g1                */

    JMP     %l1
    RETT    %l2
    MACRO_END()

;/*********************************************************************************************************
;  FLUSH ����
;  %g1 %g6 %g7 ���ᱻ�ƻ�
;*********************************************************************************************************/

MACRO_DEF(FLUSH_WINDOWS)
    RD      %psr , %l0
    ANDN    %l0  , PSR_ET , %l0
    WR      %l0  , %psr                                                 ;/*  �ر��쳣                    */
    NOP
    NOP
    NOP

    AND     %l0  , SPARC_PSR_CWP_MASK , %g3                             ;/*  %g3 = CWP                   */

    MOV     1    , %g4                                                  ;/*  %g4 = 1                     */
    SLL     %g4  , %g3 , %g4                                            ;/*  %g4 = WIM mask for CW invalid*/

    RD      %wim , %g2                                                  ;/*  %g2 = wim                   */

123:
    SLL     %g4  , 1   , %g5                                            ;/*  rotate the "wim" left 1     */
    SRL     %g4  , LW_CFG_CPU_REG_WIN_NR - 1 , %g4
    OR      %g4  , %g5 , %g4                                            ;/*  %g4 = wim if we do one restore*/

    ;/*
    ; * If a restore would not underflow, then continue.
    ; */
    ANDCC   %g4  , %g2 , %g0                                            ;/*  Any windows to flush?       */
    BNZ     456f                                                        ;/*  No, then continue           */
    NOP

    RESTORE                                                             ;/*  back one window             */

    ;/*
    ; * Now save the window just as if we overflowed to it.
    ; */
    STD     %l0  , [%sp + SF_LOCAL(0)]
    STD     %l2  , [%sp + SF_LOCAL(2)]
    STD     %l4  , [%sp + SF_LOCAL(4)]
    STD     %l6  , [%sp + SF_LOCAL(6)]

    STD     %i0  , [%sp + SF_INPUT(0)]
    STD     %i2  , [%sp + SF_INPUT(2)]
    STD     %i4  , [%sp + SF_INPUT(4)]
    STD     %i6  , [%sp + SF_INPUT(6)]

    BA      123b
    NOP

456:
    RD      %psr , %l0
    OR      %l0  , PSR_ET  , %l0                                        ;/*  ʹ���쳣                    */
    OR      %l0  , PSR_PIL , %l0                                        ;/*  ���� PSR_PIL ȥ���ж�       */
    WR      %l0  , %psr
    NOP
    NOP
    NOP
    MACRO_END()

#endif                                                                  /*  __ARCH_SPARC_CTX_ASM_H       */
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
