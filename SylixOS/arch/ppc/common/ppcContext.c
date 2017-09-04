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
** ��   ��   ��: ppcContext.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "ppcSpr.h"
#include "arch/ppc/param/ppcParam.h"
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
    ARCH_REG_T          uiMsr;
    ARCH_REG_T          uiSrr1;
    PPC_PARAM          *pParam;

    uiMsr  = ppcGetMSR();                                               /*  ��õ�ǰ MSR ��ֵ           */
    pParam = archKernelParamGet();
    uiSrr1 = uiMsr;

    if (pParam->PP_bGenericPPC32) {                                     /*  ͨ�õ� 32 λ PowerPC ������ */
#define ARCH_PPC_MSR_VEC            0x02000000                          /*  Bit 6 of MSR                */

        uiSrr1 |=  ARCH_PPC_MSR_EE;                                     /*  ʹ���ж�                    */
        uiSrr1 &= ~ARCH_PPC_MSR_PR;                                     /*  ��Ȩģʽ                    */
        uiSrr1 &= ~ARCH_PPC_MSR_FP;                                     /*  ���� FPU                    */
        uiSrr1 &= ~ARCH_PPC_MSR_VEC;                                    /*  ���� ALTIVEC                */
#if LW_CFG_VMM_EN > 0
        uiSrr1 |= ARCH_PPC_MSR_IR | ARCH_PPC_MSR_DR;                    /*  ʹ�� MMU                    */
#else
        uiSrr1 &= ~(ARCH_PPC_MSR_IR | ARCH_PPC_MSR_DR);                 /*  ���� MMU                    */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#undef  ARCH_PPC_MSR_VEC
    } else {                                                            /*  PowerPC E500 ������         */
#define ARCH_PPC_MSR_SPE            0x02000000
#define ARCH_PPC_MSR_IS             0x00000020                          /*  insn address space selector */
#define ARCH_PPC_MSR_DS             0x00000010                          /*  data address space selector */

        uiSrr1 |=  ARCH_PPC_MSR_EE;                                     /*  ʹ���ж�                    */
        uiSrr1 &= ~ARCH_PPC_MSR_SPE;                                    /*  ���� SPE(E6500 ʱΪ ALTIVEC)*/
        uiSrr1 &= ~ARCH_PPC_MSR_FP;                                     /*  ���� FPU(E500mc E5500 E6500)*/
        uiSrr1 &= ~(ARCH_PPC_MSR_IS | ARCH_PPC_MSR_DS);                 /*  ʹ�õ�ַ�ռ� 0              */

#undef  ARCH_PPC_MSR_SPE
#undef  ARCH_PPC_MSR_IS
#undef  ARCH_PPC_MSR_DS
    }

    if ((addr_t)pstkTop & 0x7) {                                        /*  ��֤��ջ�� CPU SP 8 �ֽڶ���*/
        pstkTop = (PLW_STACK)((addr_t)pstkTop - 4);                     /*  ��͵�ַ�ƽ� 4 �ֽ�         */
    }

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiFp     = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_uiLr     = (ARCH_REG_T)LW_NULL;

    pregctx->REG_uiR0   = 0;
    pregctx->REG_uiSp   = (ARCH_REG_T)pfpctx;                           /*  R1 == SP ��ȫ��ջ��� SP    */
    pregctx->REG_uiR2   = 2;
    pregctx->REG_uiR3   = (ARCH_REG_T)pvArg;                            /*  R3 ���ڲ�������             */
    pregctx->REG_uiR4   = 4;
    pregctx->REG_uiR5   = 5;
    pregctx->REG_uiR6   = 6;
    pregctx->REG_uiR7   = 7;
    pregctx->REG_uiR8   = 8;
    pregctx->REG_uiR9   = 9;
    pregctx->REG_uiR10  = 10;
    pregctx->REG_uiR11  = 11;
    pregctx->REG_uiR12  = 12;
    pregctx->REG_uiR13  = 13;
    pregctx->REG_uiR14  = 14;
    pregctx->REG_uiR15  = 15;
    pregctx->REG_uiR16  = 16;
    pregctx->REG_uiR17  = 17;
    pregctx->REG_uiR18  = 18;
    pregctx->REG_uiR19  = 19;
    pregctx->REG_uiR20  = 20;
    pregctx->REG_uiR21  = 21;
    pregctx->REG_uiR22  = 22;
    pregctx->REG_uiR23  = 23;
    pregctx->REG_uiR24  = 24;
    pregctx->REG_uiR25  = 25;
    pregctx->REG_uiR26  = 26;
    pregctx->REG_uiR27  = 27;
    pregctx->REG_uiR28  = 28;
    pregctx->REG_uiR29  = 29;
    pregctx->REG_uiR30  = 30;
    pregctx->REG_uiFp   = pfpctx->FP_uiFp;

    pregctx->REG_uiLr   = (ARCH_REG_T)pfuncTask;
    pregctx->REG_uiSrr0 = (ARCH_REG_T)pfuncTask;                        /*  RFI ���ص� ��ַ             */
    pregctx->REG_uiSrr1 = uiSrr1;                                       /*  RFI ��� MSR                */
    pregctx->REG_uiMsr  = uiMsr;                                        /*  ��ʱ�� MSR                  */
    pregctx->REG_uiCtr  = 0;
    pregctx->REG_uiXer  = 0;
    pregctx->REG_uiCr   = 0;

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
    ARCH_FP_CTX       *pfpctx      = (ARCH_FP_CTX *)((PCHAR)pregctxDest + sizeof(ARCH_REG_CTX));

    /*
     *  �� ARCH_FP_CTX ������, ģ����һ��
     *  push {fp, lr}
     *  add  fp, sp, #4
     */
    pfpctx->FP_uiFp = pregctxSrc->REG_uiFp;
    pfpctx->FP_uiLr = pregctxSrc->REG_uiLr;

    pregctxDest->REG_uiFp = (ARCH_REG_T)&pfpctx->FP_uiLr;
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
    
#if	CPU_STK_GROWTH == 0
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
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    if (iFd >= 0) {
        fdprintf(iFd, "\n");

        fdprintf(iFd, "R0  = 0x%08x  ", pregctx->REG_uiR0);
        fdprintf(iFd, "R1  = 0x%08x\n", pregctx->REG_uiSp);             /*  R1 == SP �쳣ʱ�� SP        */

        fdprintf(iFd, "R2  = 0x%08x  ", pregctx->REG_uiR2);
        fdprintf(iFd, "R3  = 0x%08x\n", pregctx->REG_uiR3);

        fdprintf(iFd, "R4  = 0x%08x  ", pregctx->REG_uiR4);
        fdprintf(iFd, "R5  = 0x%08x\n", pregctx->REG_uiR5);

        fdprintf(iFd, "R6  = 0x%08x  ", pregctx->REG_uiR6);
        fdprintf(iFd, "R7  = 0x%08x\n", pregctx->REG_uiR7);

        fdprintf(iFd, "R8  = 0x%08x  ", pregctx->REG_uiR8);
        fdprintf(iFd, "R9  = 0x%08x\n", pregctx->REG_uiR9);

        fdprintf(iFd, "R10 = 0x%08x  ", pregctx->REG_uiR10);
        fdprintf(iFd, "R11 = 0x%08x\n", pregctx->REG_uiR11);

        fdprintf(iFd, "R12 = 0x%08x  ", pregctx->REG_uiR12);
        fdprintf(iFd, "R13 = 0x%08x\n", pregctx->REG_uiR13);

        fdprintf(iFd, "R14 = 0x%08x  ", pregctx->REG_uiR14);
        fdprintf(iFd, "R15 = 0x%08x\n", pregctx->REG_uiR15);

        fdprintf(iFd, "R16 = 0x%08x  ", pregctx->REG_uiR16);
        fdprintf(iFd, "R17 = 0x%08x\n", pregctx->REG_uiR17);

        fdprintf(iFd, "R18 = 0x%08x  ", pregctx->REG_uiR18);
        fdprintf(iFd, "R19 = 0x%08x\n", pregctx->REG_uiR19);

        fdprintf(iFd, "R20 = 0x%08x  ", pregctx->REG_uiR20);
        fdprintf(iFd, "R21 = 0x%08x\n", pregctx->REG_uiR21);

        fdprintf(iFd, "R22 = 0x%08x  ", pregctx->REG_uiR22);
        fdprintf(iFd, "R23 = 0x%08x\n", pregctx->REG_uiR23);

        fdprintf(iFd, "R24 = 0x%08x  ", pregctx->REG_uiR24);
        fdprintf(iFd, "R25 = 0x%08x\n", pregctx->REG_uiR25);

        fdprintf(iFd, "R26 = 0x%08x  ", pregctx->REG_uiR26);
        fdprintf(iFd, "R27 = 0x%08x\n", pregctx->REG_uiR27);

        fdprintf(iFd, "R28 = 0x%08x  ", pregctx->REG_uiR28);
        fdprintf(iFd, "R29 = 0x%08x\n", pregctx->REG_uiR29);

        fdprintf(iFd, "R30 = 0x%08x  ", pregctx->REG_uiR30);
        fdprintf(iFd, "R31 = 0x%08x\n", pregctx->REG_uiFp);             /*  R31 == FP                   */

        fdprintf(iFd, "FP  = 0x%08x  ", pregctx->REG_uiFp);
        fdprintf(iFd, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);           /*  �쳣ѹջ��� SP             */

        fdprintf(iFd, "LR  = 0x%08x  ", pregctx->REG_uiLr);
        fdprintf(iFd, "PC  = 0x%08x\n", pregctx->REG_uiSrr0);           /*  �쳣ʱ�� PC                 */

        fdprintf(iFd, "MSR = 0x%08x  ", pregctx->REG_uiSrr1);           /*  �쳣ʱ�� MSR                */
        fdprintf(iFd, "CR  = 0x%08x\n", pregctx->REG_uiCr);

        fdprintf(iFd, "CTR = 0x%08x  ", pregctx->REG_uiCtr);
        fdprintf(iFd, "XER = 0x%08x\n", pregctx->REG_uiXer);

    } else {
        archTaskCtxPrint(pstkTop);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: archTaskCtxPrint
** ��������: ֱ�Ӵ�ӡ����������
** �䡡��  : pstkTop    ��ջջ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTaskCtxPrint (PLW_STACK  pstkTop)
{
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    _PrintFormat("\n");

    _PrintFormat("R0  = 0x%08x  ",   pregctx->REG_uiR0);
    _PrintFormat("R1  = 0x%08x\r\n", pregctx->REG_uiSp);                /*  R1 == SP �쳣ʱ�� SP        */

    _PrintFormat("R2  = 0x%08x  ",   pregctx->REG_uiR2);
    _PrintFormat("R3  = 0x%08x\r\n", pregctx->REG_uiR3);

    _PrintFormat("R4  = 0x%08x  ",   pregctx->REG_uiR4);
    _PrintFormat("R5  = 0x%08x\r\n", pregctx->REG_uiR5);

    _PrintFormat("R6  = 0x%08x  ",   pregctx->REG_uiR6);
    _PrintFormat("R7  = 0x%08x\r\n", pregctx->REG_uiR7);

    _PrintFormat("R8  = 0x%08x  ",   pregctx->REG_uiR8);
    _PrintFormat("R9  = 0x%08x\r\n", pregctx->REG_uiR9);

    _PrintFormat("R10 = 0x%08x  ",   pregctx->REG_uiR10);
    _PrintFormat("R11 = 0x%08x\r\n", pregctx->REG_uiR11);

    _PrintFormat("R12 = 0x%08x  ",   pregctx->REG_uiR12);
    _PrintFormat("R13 = 0x%08x\r\n", pregctx->REG_uiR13);

    _PrintFormat("R14 = 0x%08x  ",   pregctx->REG_uiR14);
    _PrintFormat("R15 = 0x%08x\r\n", pregctx->REG_uiR15);

    _PrintFormat("R16 = 0x%08x  ",   pregctx->REG_uiR16);
    _PrintFormat("R17 = 0x%08x\r\n", pregctx->REG_uiR17);

    _PrintFormat("R18 = 0x%08x  ",   pregctx->REG_uiR18);
    _PrintFormat("R19 = 0x%08x\r\n", pregctx->REG_uiR19);

    _PrintFormat("R20 = 0x%08x  ",   pregctx->REG_uiR20);
    _PrintFormat("R21 = 0x%08x\r\n", pregctx->REG_uiR21);

    _PrintFormat("R22 = 0x%08x  ",   pregctx->REG_uiR22);
    _PrintFormat("R23 = 0x%08x\r\n", pregctx->REG_uiR23);

    _PrintFormat("R24 = 0x%08x  ",   pregctx->REG_uiR24);
    _PrintFormat("R25 = 0x%08x\r\n", pregctx->REG_uiR25);

    _PrintFormat("R26 = 0x%08x  ",   pregctx->REG_uiR26);
    _PrintFormat("R27 = 0x%08x\r\n", pregctx->REG_uiR27);

    _PrintFormat("R28 = 0x%08x  ",   pregctx->REG_uiR28);
    _PrintFormat("R29 = 0x%08x\r\n", pregctx->REG_uiR29);

    _PrintFormat("R30 = 0x%08x  ",   pregctx->REG_uiR30);
    _PrintFormat("R31 = 0x%08x\r\n", pregctx->REG_uiFp);                /*  R31 == FP                   */

    _PrintFormat("FP  = 0x%08x  ",   pregctx->REG_uiFp);
    _PrintFormat("SP  = 0x%08x\r\n", (ARCH_REG_T)pstkTop);              /*  �쳣ѹջ��� SP             */

    _PrintFormat("LR  = 0x%08x  ",   pregctx->REG_uiLr);
    _PrintFormat("PC  = 0x%08x\r\n", pregctx->REG_uiSrr0);              /*  �쳣ʱ�� PC                 */

    _PrintFormat("MSR = 0x%08x  ",   pregctx->REG_uiSrr1);              /*  �쳣ʱ�� MSR                */
    _PrintFormat("CR  = 0x%08x\r\n", pregctx->REG_uiCr);

    _PrintFormat("CTR = 0x%08x  ",   pregctx->REG_uiCtr);
    _PrintFormat("XER = 0x%08x\r\n", pregctx->REG_uiXer);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
