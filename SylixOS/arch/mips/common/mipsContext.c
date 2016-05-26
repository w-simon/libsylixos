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
** ��   ��   ��: mipsContext.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
** ��������: archTaskCtxCreate
** ��������: ��������������
** �䡡��  : pfuncTask      �������
**           pvArg          ��ڲ���
**           pstkTop        ��ʼ����ջ���
**           ulOpt          ���񴴽�ѡ��
** �䡡��  : ��ʼ����ջ������
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��ջ�Ӹߵ�ַ��͵�ַ����.
*********************************************************************************************************/
PLW_STACK  archTaskCtxCreate (PTHREAD_START_ROUTINE  pfuncTask,
                              PVOID                  pvArg,
                              PLW_STACK              pstkTop,
                              ULONG                  ulOpt)
{
    ARCH_REG_CTX       *pregctx;
    ARCH_FP_CTX        *pfpctx;
    UINT32              uiCP0Status;
    UINT32              uiGP;
    INT                 i;

    uiCP0Status  = mipsCp0StatusRead();                                 /*  ��õ�ǰ�� CP0 STATUS �Ĵ���*/
    uiCP0Status |= bspIntInitEnableStatus() | M_StatusIE;               /*  ʹ���ж�                    */

    MIPS_EXEC_INS("addi   %0, " MIPS_GP ", 0" : "=r"(uiGP));

    if ((addr_t)pstkTop & 0x7) {                                        /*  ��֤��ջ�� CPU SP 8 �ֽڶ���*/
        pstkTop = (PLW_STACK)((addr_t)pstkTop - 4);                     /*  ��͵�ַ�ƽ� 4 �ֽ�         */
    }

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiFP = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_uiRA = (ARCH_REG_T)LW_NULL;

    for (i = 0; i < 32; i++) {
        pregctx->REG_uiReg[i] = i;
    }

    pregctx->REG_uiReg[REG_A0] = (ARCH_REG_T)pvArg;
    pregctx->REG_uiReg[REG_GP] = (ARCH_REG_T)uiGP;
    pregctx->REG_uiReg[REG_FP] = (ARCH_REG_T)pfpctx->FP_uiFP;
    pregctx->REG_uiReg[REG_RA] = (ARCH_REG_T)pfuncTask;

    pregctx->REG_uiCP0Status   = (ARCH_REG_T)uiCP0Status;
    pregctx->REG_uiCP0Cause    = (ARCH_REG_T)0x0;
    pregctx->REG_uiCP0DataLO   = (ARCH_REG_T)0x0;
    pregctx->REG_uiCP0DataHI   = (ARCH_REG_T)0x0;
    pregctx->REG_uiCP0EPC      = (ARCH_REG_T)pfuncTask;

    return  ((PLW_STACK)pregctx);
}
/*********************************************************************************************************
** ��������: archTaskCtxSetFp
** ��������: ��������������ջ֡ (���� backtrace ����, ������� backtrace ����ļ�)
** �䡡��  : pstkDest  Ŀ�� stack frame
**           pstkSrc   Դ�� stack frame
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTaskCtxSetFp (PLW_STACK  pstkDest, PLW_STACK  pstkSrc)
{
    ARCH_REG_CTX      *pregctxDest = (ARCH_REG_CTX *)pstkDest;
    ARCH_REG_CTX      *pregctxSrc  = (ARCH_REG_CTX *)pstkSrc;
    ARCH_FP_CTX       *pfpctx      = (ARCH_FP_CTX  *)((PCHAR)pregctxDest + sizeof(ARCH_REG_CTX));

    /*
     *  �� ARCH_FP_CTX ������, ģ����һ��
     *  push {fp, lr}
     *  add  fp, sp, #4
     */
    pfpctx->FP_uiFP = pregctxSrc->REG_uiReg[REG_FP];
    pfpctx->FP_uiRA = pregctxSrc->REG_uiReg[REG_RA];

    pregctxDest->REG_uiReg[REG_FP] = (ARCH_REG_T)&pfpctx->FP_uiRA;
}
/*********************************************************************************************************
** ��������: archTaskRegsGet
** ��������: ͨ��ջ��ָ���ȡ�Ĵ����� (��ջ�ṹ)
** �䡡��  : pstkTop        ��ջ����
**           pregSp         SP ָ��
** �䡡��  : �Ĵ����ṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ARCH_REG_CTX  *archTaskRegsGet (PLW_STACK  pstkTop, ARCH_REG_T *pregSp)
{
    ARCH_REG_T  regSp = (ARCH_REG_T)pstkTop;

#if CPU_STK_GROWTH == 0
    regSp -= sizeof(ARCH_REG_CTX);
#else
    regSp += sizeof(ARCH_REG_CTX);
#endif

    *pregSp = regSp;

    return  ((ARCH_REG_CTX *)pstkTop);
}
/*********************************************************************************************************
** ��������: archTaskRegsSet
** ��������: ͨ��ջ��ָ���ȡ�Ĵ����� (��ջ�ṹ)
** �䡡��  : pstkTop        ��ջ����
**           pregctx        �Ĵ�����
** �䡡��  : �Ĵ����ṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTaskRegsSet (PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx)
{
    *(ARCH_REG_CTX *)pstkTop = *pregctx;
}
/*********************************************************************************************************
** ��������: archTaskCtxShow
** ��������: ��ӡ����������
** �䡡��  : iFd        �ļ�������
             pstkTop    ��ջջ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

VOID  archTaskCtxShow (INT  iFd, PLW_STACK  pstkTop)
{
    UINT32              uiCP0Status;
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    fdprintf(iFd, "\n");

    fdprintf(iFd, "SP      = 0x%08x\n", (ARCH_REG_T)pstkTop);
    fdprintf(iFd, "EPC     = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0EPC);

    fdprintf(iFd, "R01(AT) = 0x%08x\n", pregctx->REG_uiReg[REG_AT]);
    fdprintf(iFd, "R02(V0) = 0x%08x\n", pregctx->REG_uiReg[REG_V0]);
    fdprintf(iFd, "R03(V1) = 0x%08x\n", pregctx->REG_uiReg[REG_V1]);
    fdprintf(iFd, "R04(A0) = 0x%08x\n", pregctx->REG_uiReg[REG_A0]);
    fdprintf(iFd, "R05(A1) = 0x%08x\n", pregctx->REG_uiReg[REG_A1]);
    fdprintf(iFd, "R06(A2) = 0x%08x\n", pregctx->REG_uiReg[REG_A2]);
    fdprintf(iFd, "R07(A3) = 0x%08x\n", pregctx->REG_uiReg[REG_A3]);
    fdprintf(iFd, "R08(T0) = 0x%08x\n", pregctx->REG_uiReg[REG_T0]);
    fdprintf(iFd, "R09(T1) = 0x%08x\n", pregctx->REG_uiReg[REG_T1]);
    fdprintf(iFd, "R10(T2) = 0x%08x\n", pregctx->REG_uiReg[REG_T2]);
    fdprintf(iFd, "R11(T3) = 0x%08x\n", pregctx->REG_uiReg[REG_T3]);
    fdprintf(iFd, "R12(T4) = 0x%08x\n", pregctx->REG_uiReg[REG_T4]);
    fdprintf(iFd, "R13(T5) = 0x%08x\n", pregctx->REG_uiReg[REG_T5]);
    fdprintf(iFd, "R14(T6) = 0x%08x\n", pregctx->REG_uiReg[REG_T6]);
    fdprintf(iFd, "R15(T7) = 0x%08x\n", pregctx->REG_uiReg[REG_T7]);
    fdprintf(iFd, "R16(S0) = 0x%08x\n", pregctx->REG_uiReg[REG_S0]);
    fdprintf(iFd, "R17(S1) = 0x%08x\n", pregctx->REG_uiReg[REG_S1]);
    fdprintf(iFd, "R18(S2) = 0x%08x\n", pregctx->REG_uiReg[REG_S2]);
    fdprintf(iFd, "R19(S3) = 0x%08x\n", pregctx->REG_uiReg[REG_S3]);
    fdprintf(iFd, "R20(S4) = 0x%08x\n", pregctx->REG_uiReg[REG_S4]);
    fdprintf(iFd, "R21(S5) = 0x%08x\n", pregctx->REG_uiReg[REG_S5]);
    fdprintf(iFd, "R22(S6) = 0x%08x\n", pregctx->REG_uiReg[REG_S6]);
    fdprintf(iFd, "R23(S7) = 0x%08x\n", pregctx->REG_uiReg[REG_S7]);
    fdprintf(iFd, "R24(T8) = 0x%08x\n", pregctx->REG_uiReg[REG_T8]);
    fdprintf(iFd, "R25(T9) = 0x%08x\n", pregctx->REG_uiReg[REG_T9]);
    fdprintf(iFd, "R28(GP) = 0x%08x\n", pregctx->REG_uiReg[REG_GP]);
    fdprintf(iFd, "R30(FP) = 0x%08x\n", pregctx->REG_uiReg[REG_FP]);
    fdprintf(iFd, "R31(RA) = 0x%08x\n", pregctx->REG_uiReg[REG_RA]);

    uiCP0Status = pregctx->REG_uiCP0Status;

    fdprintf(iFd, "CP0 Status Register:\n");
    fdprintf(iFd, "CU3 = %d  ", (uiCP0Status & M_StatusCU3) >> S_StatusCU3);
    fdprintf(iFd, "CU2 = %d\n", (uiCP0Status & M_StatusCU2) >> S_StatusCU2);
    fdprintf(iFd, "CU1 = %d  ", (uiCP0Status & M_StatusCU1) >> S_StatusCU1);
    fdprintf(iFd, "CU0 = %d\n", (uiCP0Status & M_StatusCU0) >> S_StatusCU0);
    fdprintf(iFd, "RP  = %d  ", (uiCP0Status & M_StatusRP)  >> S_StatusRP);
    fdprintf(iFd, "FR  = %d\n", (uiCP0Status & M_StatusFR)  >> S_StatusFR);
    fdprintf(iFd, "RE  = %d  ", (uiCP0Status & M_StatusRE)  >> S_StatusRE);
    fdprintf(iFd, "MX  = %d\n", (uiCP0Status & M_StatusMX)  >> S_StatusMX);
    fdprintf(iFd, "PX  = %d  ", (uiCP0Status & M_StatusPX)  >> S_StatusPX);
    fdprintf(iFd, "BEV = %d\n", (uiCP0Status & M_StatusBEV) >> S_StatusBEV);
    fdprintf(iFd, "TS  = %d  ", (uiCP0Status & M_StatusTS)  >> S_StatusTS);
    fdprintf(iFd, "SR  = %d\n", (uiCP0Status & M_StatusSR)  >> S_StatusSR);
    fdprintf(iFd, "NMI = %d  ", (uiCP0Status & M_StatusNMI) >> S_StatusNMI);
    fdprintf(iFd, "IM7 = %d\n", (uiCP0Status & M_StatusIM7) >> S_StatusIM7);
    fdprintf(iFd, "IM6 = %d  ", (uiCP0Status & M_StatusIM6) >> S_StatusIM6);
    fdprintf(iFd, "IM5 = %d\n", (uiCP0Status & M_StatusIM5) >> S_StatusIM5);
    fdprintf(iFd, "IM4 = %d  ", (uiCP0Status & M_StatusIM4) >> S_StatusIM4);
    fdprintf(iFd, "IM3 = %d\n", (uiCP0Status & M_StatusIM3) >> S_StatusIM3);
    fdprintf(iFd, "IM2 = %d  ", (uiCP0Status & M_StatusIM2) >> S_StatusIM2);
    fdprintf(iFd, "IM1 = %d\n", (uiCP0Status & M_StatusIM1) >> S_StatusIM1);
    fdprintf(iFd, "IM0 = %d  ", (uiCP0Status & M_StatusIM0) >> S_StatusIM0);
    fdprintf(iFd, "KX  = %d\n", (uiCP0Status & M_StatusKX)  >> S_StatusKX);
    fdprintf(iFd, "SX  = %d  ", (uiCP0Status & M_StatusSX)  >> S_StatusSX);
    fdprintf(iFd, "UX  = %d\n", (uiCP0Status & M_StatusUX)  >> S_StatusUX);
    fdprintf(iFd, "KSU = %d  ", (uiCP0Status & M_StatusKSU) >> S_StatusKSU);
    fdprintf(iFd, "UM  = %d\n", (uiCP0Status & M_StatusUM)  >> S_StatusUM);
    fdprintf(iFd, "SM  = %d  ", (uiCP0Status & M_StatusSM)  >> S_StatusSM);
    fdprintf(iFd, "ERL = %d\n", (uiCP0Status & M_StatusERL) >> S_StatusERL);
    fdprintf(iFd, "EXL = %d  ", (uiCP0Status & M_StatusEXL) >> S_StatusEXL);
    fdprintf(iFd, "IE  = %d\n", (uiCP0Status & M_StatusIE)  >> S_StatusIE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
