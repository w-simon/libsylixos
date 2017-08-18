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
;** ��   ��   ��: c6xContextAsm.S
;**
;** ��   ��   ��: Jiao.JinXing (������)
;**
;** �ļ���������: 2017 �� 03 �� 17 ��
;**
;** ��        ��: c6x ��ϵ�����������л�.
;*********************************************************************************************************/

    .include "c6xContextAsmInc.asm"

    .ref    _SchedSwp
    .ref    _SchedCrSwp

    .global archTaskCtxInit
    .global archTaskCtxStart
    .global archTaskCtxSwitch
    .global archCrtCtxSwitch
    .global archIntCtxLoad
    .global archSigCtxLoad

    .sect .text

;/*********************************************************************************************************
;  ��ʼ�������������е����⹦�ܼĴ�������
;*********************************************************************************************************/

archTaskCtxInit: .asmfunc
    MVC     ILC  , B16
    STW     B16  , *A4--                                                ;/*  ILC                         */
 || MVC     GPLYA, B17

    STW     B17  , *A4--                                                ;/*  GPLYA                       */
 || MVC     GPLYB, B18

    STW     B18  , *A4--                                                ;/*  GPLYB                       */
 || MVC     ITSR , B19

    STW     B19  , *A4--                                                ;/*  ITSR                        */
 || MVC     RILC , B20

    STW     B20  , *A4--                                                ;/*  RILC                        */
 || MVC     SSR  , B21

    .if (.TMS320C6740)
    STW     B21  , *A4--                                                ;/*  SSR                         */
 || MVC     FADCR, B22

    STW     B22  , *A4--                                                ;/*  FADCR                       */
 || MVC     FAUCR, B23

    STW     B23  , *A4--                                                ;/*  FAUCR                       */
 || MVC     FMCR , B24
    .else
    STW     B21  , *A4--                                                ;/*  SSR                         */
    STW     B22  , *A4--                                                ;/*  FADCR                       */
    STW     B23  , *A4--                                                ;/*  FAUCR                       */
    .endif

    STW     B24  , *A4--                                                ;/*  FMCR                        */

    ADDK    -4   , A4                                                   ;/*  Align 64bit                 */

    MVC     GFPGFR, B7

    STW     B3   , *A4--                                                ;/*  B3                          */
 || MVC     AMR  , B8

    STW     B7   , *A4--                                                ;/*  GFPGFR                      */
 || MVC     CSR  , B9

    STW     B8   , *A4--                                                ;/*  AMR                         */

    B       B3
 || STW     B9   , *A4--                                                ;/*  CSR                         */

    NOP     5
    .endasmfunc

;/*********************************************************************************************************
;  ��ǰ CPU ��ʼ���ж�����ִ��
;  ����(A4 �Ĵ���)Ϊ��ǰ CPU ���ƿ�
;*********************************************************************************************************/

archTaskCtxStart: .asmfunc
    LDW     *+A4(0) , A3                                                ;/*  Load new tcb                */
    NOP     4
    LDW     *+A3(0) , B15                                               ;/*  Load new context SP         */
    NOP     4
    RESTORE_SMALL_CTX                                                   ;/*  Restore small context       */
    .endasmfunc

;/*********************************************************************************************************
;  �߳��л� 
;  ����(A4 �Ĵ���)Ϊ��ǰ CPU ���ƿ�
;*********************************************************************************************************/

archTaskCtxSwitch: .asmfunc
    SAVE_SMALL_CTX                                                      ;/*  Save small context          */

    LDW     *+A4(0) , A3                                                ;/*  Load current tcb            */
    NOP     4

    STW     B15 , *+A3(0)                                               ;/*  Save current context SP     */
 || MVK     0   , A2
    STW     A2  , *+A3(4)                                               ;/*  Set small context flag      */
 || MV      A4  , A10                                                   ;/*  Save A4 to A10              */
    B       _SchedSwp                                                   ;/*  CALL _SchedSwp()            */
 || AND     ~7  , B15 , B15
    ADDKPC  _SchedSwpRet , B3 , 4
_SchedSwpRet:

    LDW     *+A10(0) , A3                                               ;/*  Load new tcb                */
    NOP     4

    LDW     *+A3(4) , A2                                                ;/*  Load context flag           */
    LDW     *+A3(0) , B15                                               ;/*  Load new context SP         */
    NOP     3
    [A2]    BNOP  _RestoreBigCtx , 5

    RESTORE_SMALL_CTX                                                   ;/*  Restore small context       */

_RestoreBigCtx:
    RESTORE_BIG_CTX                                                     ;/*  Restore big context         */
    .endasmfunc

;/*********************************************************************************************************
;  Э���л� 
;  ����(A4 �Ĵ���)Ϊ��ǰ CPU ���ƿ�
;*********************************************************************************************************/

archCrtCtxSwitch: .asmfunc
    SAVE_SMALL_CTX                                                      ;/*  Save big context            */

    LDW     *+A4(8) , A3                                                ;/*  Load current crcb           */
    NOP     4

    STW     B15 , *+A3(0)                                               ;/*  Save current context SP     */
 || MV      A4  , A10                                                   ;/*  Save A4 to A10              */
    B       _SchedCrSwp                                                 ;/*  CALL _SchedCrSwp()          */
 || AND     ~7  , B15 , B15
    ADDKPC  _SchedCrSwpRet , B3 , 4
_SchedCrSwpRet:

    LDW     *+A10(8) , A3                                               ;/*  Load new crcb               */
    NOP     4

    LDW     *+A3(0) , B15                                               ;/*  Load new context SP         */
    NOP     4

    RESTORE_SMALL_CTX                                                   ;/*  Restore big context         */
    .endasmfunc

;/*********************************************************************************************************
;  �жϷ���ʱ, �߳�װ��
;  ����(A4 �Ĵ���)Ϊ��ǰ CPU ���ƿ�
;*********************************************************************************************************/

archIntCtxLoad: .asmfunc
    LDW     *+A4(0) , A3                                                ;/*  Load new tcb                */
    NOP     4

    LDW     *+A3(4) , A2                                                ;/*  Load context flag           */
    LDW     *+A3(0) , B15                                               ;/*  Load new context SP         */
    NOP     3
    [A2]    BNOP  _RestoreBigCtx2 , 5

    RESTORE_SMALL_CTX                                                   ;/*  Restore small context       */

_RestoreBigCtx2:
    RESTORE_BIG_CTX                                                     ;/*  Restore big context         */
    .endasmfunc

;/*********************************************************************************************************
;  �ź������ķ���
;  ����1(A4 �Ĵ���)Ϊ��Ҫ���صĶ�ջջ��λ��
;  ����2(B4 �Ĵ���)Ϊ����������
;*********************************************************************************************************/

archSigCtxLoad: .asmfunc
    MV      A4 , B15                                                    ;/*  Load new context SP         */
    MV      B4 , A2
    [A2]    BNOP  _RestoreBigCtx3 , 5

    RESTORE_SMALL_CTX                                                   ;/*  Restore small context       */

_RestoreBigCtx3:
    RESTORE_BIG_CTX                                                     ;/*  Restore big context         */
    .endasmfunc

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
