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
** ��   ��   ��: ppcContextE500Asm.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 12 ��
**
** ��        ��: PowerPC E500 ��ϵ�����������л�.
*********************************************************************************************************/

#ifndef __ARCH_PPCCONTEXTE500ASM_H
#define __ARCH_PPCCONTEXTE500ASM_H

#include "arch/ppc/arch_regs.h"

/*********************************************************************************************************
  E500 �쳣�����ı���
  E500 ����ʹ�� MMU, ����û�� ENABLE_MMU ����
*********************************************************************************************************/

MACRO_DEF(E500_EXC_SAVE_REGS)
    STWU    SP  , -STACK_FRAME_SIZE(SP)                                 /*  ���沢���� SP               */

    STW     R0  ,  XR0(SP)                                              /*  ���� R0, R2 - R31           */
    STW     R2  ,  XR2(SP)
    STW     R3  ,  XR3(SP)
    STW     R4  ,  XR4(SP)
    STW     R5  ,  XR5(SP)
    STW     R6  ,  XR6(SP)
    STW     R7  ,  XR7(SP)
    STW     R8  ,  XR8(SP)
    STW     R9  ,  XR9(SP)
    STW     R10 , XR10(SP)
    STW     R11 , XR11(SP)
    STW     R12 , XR12(SP)
    STW     R13 , XR13(SP)
    STW     R14 , XR14(SP)
    STW     R15 , XR15(SP)
    STW     R16 , XR16(SP)
    STW     R17 , XR17(SP)
    STW     R18 , XR18(SP)
    STW     R19 , XR19(SP)
    STW     R20 , XR20(SP)
    STW     R21 , XR21(SP)
    STW     R22 , XR22(SP)
    STW     R23 , XR23(SP)
    STW     R24 , XR24(SP)
    STW     R25 , XR25(SP)
    STW     R26 , XR26(SP)
    STW     R27 , XR27(SP)
    STW     R28 , XR28(SP)
    STW     R29 , XR29(SP)
    STW     R30 , XR30(SP)
    STW     R31 , XR31(SP)

    MFMSR   R0
    ISYNC
    STW     R0  , XMSR(SP)                                              /*  ���� MSR                    */
    SYNC

    MFSPR   R0  , SRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  ���� SRR0                   */
    SYNC

    MFSPR   R0  , SRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  ���� SRR1                   */
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(SP)                                               /*  ���� LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(SP)                                              /*  ���� CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(SP)                                              /*  ���� XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(SP)                                               /*  ���� CR                     */
    SYNC
    MACRO_END()

/*********************************************************************************************************
  E500 �ٽ������쳣�����ı���
  E500 ����ʹ�� MMU, ����û�� ENABLE_MMU ����
*********************************************************************************************************/

MACRO_DEF(E500_CI_EXC_SAVE_REGS)
    STWU    SP  , -STACK_FRAME_SIZE(SP)                                 /*  ���沢���� SP               */

    STW     R0  ,  XR0(SP)                                              /*  ���� R0, R2 - R31           */
    STW     R2  ,  XR2(SP)
    STW     R3  ,  XR3(SP)
    STW     R4  ,  XR4(SP)
    STW     R5  ,  XR5(SP)
    STW     R6  ,  XR6(SP)
    STW     R7  ,  XR7(SP)
    STW     R8  ,  XR8(SP)
    STW     R9  ,  XR9(SP)
    STW     R10 , XR10(SP)
    STW     R11 , XR11(SP)
    STW     R12 , XR12(SP)
    STW     R13 , XR13(SP)
    STW     R14 , XR14(SP)
    STW     R15 , XR15(SP)
    STW     R16 , XR16(SP)
    STW     R17 , XR17(SP)
    STW     R18 , XR18(SP)
    STW     R19 , XR19(SP)
    STW     R20 , XR20(SP)
    STW     R21 , XR21(SP)
    STW     R22 , XR22(SP)
    STW     R23 , XR23(SP)
    STW     R24 , XR24(SP)
    STW     R25 , XR25(SP)
    STW     R26 , XR26(SP)
    STW     R27 , XR27(SP)
    STW     R28 , XR28(SP)
    STW     R29 , XR29(SP)
    STW     R30 , XR30(SP)
    STW     R31 , XR31(SP)

    MFMSR   R0
    ISYNC
    STW     R0  , XMSR(SP)                                              /*  ���� MSR                    */
    SYNC

    MFSPR   R0  , CSRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  ���� CSRR0                  */
    SYNC

    MFSPR   R0  , CSRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  ���� CSRR1                  */
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(SP)                                               /*  ���� LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(SP)                                              /*  ���� CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(SP)                                              /*  ���� XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(SP)                                               /*  ���� CR                     */
    SYNC
    MACRO_END()

/*********************************************************************************************************
  E500 ��������쳣�����ı���
  E500 ����ʹ�� MMU, ����û�� ENABLE_MMU ����
*********************************************************************************************************/

MACRO_DEF(E500_MC_EXC_SAVE_REGS)
    STWU    SP  , -STACK_FRAME_SIZE(SP)                                 /*  ���沢���� SP               */

    STW     R0  ,  XR0(SP)                                              /*  ���� R0, R2 - R31           */
    STW     R2  ,  XR2(SP)
    STW     R3  ,  XR3(SP)
    STW     R4  ,  XR4(SP)
    STW     R5  ,  XR5(SP)
    STW     R6  ,  XR6(SP)
    STW     R7  ,  XR7(SP)
    STW     R8  ,  XR8(SP)
    STW     R9  ,  XR9(SP)
    STW     R10 , XR10(SP)
    STW     R11 , XR11(SP)
    STW     R12 , XR12(SP)
    STW     R13 , XR13(SP)
    STW     R14 , XR14(SP)
    STW     R15 , XR15(SP)
    STW     R16 , XR16(SP)
    STW     R17 , XR17(SP)
    STW     R18 , XR18(SP)
    STW     R19 , XR19(SP)
    STW     R20 , XR20(SP)
    STW     R21 , XR21(SP)
    STW     R22 , XR22(SP)
    STW     R23 , XR23(SP)
    STW     R24 , XR24(SP)
    STW     R25 , XR25(SP)
    STW     R26 , XR26(SP)
    STW     R27 , XR27(SP)
    STW     R28 , XR28(SP)
    STW     R29 , XR29(SP)
    STW     R30 , XR30(SP)
    STW     R31 , XR31(SP)

    MFMSR   R0
    ISYNC
    STW     R0  , XMSR(SP)                                              /*  ���� MSR                    */
    SYNC

    MFSPR   R0  , MCSRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  ���� MCSRR0                 */
    SYNC

    MFSPR   R0  , MCSRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  ���� MCSRR1                 */
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(SP)                                               /*  ���� LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(SP)                                              /*  ���� CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(SP)                                              /*  ���� XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(SP)                                               /*  ���� CR                     */
    SYNC
    MACRO_END()

#endif                                                                  /* __ARCH_PPCCONTEXTE500ASM_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
