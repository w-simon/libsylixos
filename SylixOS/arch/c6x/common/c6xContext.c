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
** ��   ��   ��: c6xContext.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 03 �� 17 ��
**
** ��        ��: c6x ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
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

    /*
     * TI c6x DSP SP Ϊ�ն�ջ�ݼ�
     * The stack grows toward zero. The SP points to the word above the topmost allocated word;
     * that is, the word at *(SP+4) is allocated, but *SP is not.
     */
    if (((addr_t)pstkTop & 0x7) == 0) {                                 /*  ��֤��ջ�� CPU SP 8 �ֽڶ���*/
        pstkTop = (PLW_STACK)((addr_t)pstkTop - 4);                     /*  ��͵�ַ�ƽ� 4 �ֽ�         */
    }

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiRetAddr = 0;
    pfpctx->FP_uiArg     = 0;

    archTaskCtxInit(&pregctx->REG_uiIlc);

    pregctx->REG_uiA4  = (UINT32)pvArg;                                 /*  A4 �Ĵ������ݲ���           */
    pregctx->REG_uiA10 = 0xaaaaaa00 + 10;
    pregctx->REG_uiA11 = 0xaaaaaa00 + 11;
    pregctx->REG_uiA12 = 0xaaaaaa00 + 12;
    pregctx->REG_uiA13 = 0xaaaaaa00 + 13;
    pregctx->REG_uiA14 = 0xaaaaaa00 + 14;
    pregctx->REG_uiA15 = (UINT32)0;                                     /*  FP                          */

    pregctx->REG_uiB3  = (UINT32)pfuncTask;                             /*  B3 �Ĵ������淵�ص�ַ       */

    pregctx->REG_uiB10 = 0xbbbbbb00 + 10;
    pregctx->REG_uiB11 = 0xbbbbbb00 + 11;
    pregctx->REG_uiB12 = 0xbbbbbb00 + 12;
    pregctx->REG_uiB13 = 0xbbbbbb00 + 13;
    pregctx->REG_uiB14 = archDataPointerGet();                          /*  B14 ������ָ�� DP           */

    pregctx->REG_uiReserved0 = 0xcccccc00 + 0;
    pregctx->REG_uiReserved1 = 0xcccccc00 + 1;
    pregctx->REG_uiReserved2 = 0xcccccc00 + 2;

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
    (VOID)pstkDest;
    (VOID)pstkSrc;

    /*
     * c6x ��ʹ��ջ֡���� backtrace ����
     */
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
VOID  archTaskRegsGet (PLW_CLASS_TCB  ptcb, ARCH_REG_IRQ_CTX  *pdestregctx, ARCH_REG_T *pregSp)
{
    ARCH_REG_T  regSp = (ARCH_REG_T)ptcb->TCB_pstkStackNow;

    if (ptcb->TCB_ulContextType) {
        *pdestregctx = *(ARCH_REG_IRQ_CTX *)ptcb->TCB_pstkStackNow;
        regSp       += sizeof(ARCH_REG_IRQ_CTX);
        *pregSp      = regSp;

    } else {
        ARCH_REG_CTX  *pregctx = (ARCH_REG_CTX *)ptcb->TCB_pstkStackNow;

        lib_bzero(pdestregctx, sizeof(*pdestregctx));
        pdestregctx->REG_uiCsr    = pregctx->REG_uiCsr;
        pdestregctx->REG_uiAmr    = pregctx->REG_uiAmr;
        pdestregctx->REG_uiGfpgfr = pregctx->REG_uiGfpgfr;
        pdestregctx->REG_uiB3     = pregctx->REG_uiB3;
        pdestregctx->REG_uiIrp    = pregctx->REG_uiB3;
        pdestregctx->REG_uiFmcr   = pregctx->REG_uiFmcr;
        pdestregctx->REG_uiFaucr  = pregctx->REG_uiFaucr;
        pdestregctx->REG_uiFadcr  = pregctx->REG_uiFadcr;
        pdestregctx->REG_uiSsr    = pregctx->REG_uiSsr;
        pdestregctx->REG_uiRilc   = pregctx->REG_uiRilc;
        pdestregctx->REG_uiItsr   = pregctx->REG_uiItsr;
        pdestregctx->REG_uiGplyb  = pregctx->REG_uiGplyb;
        pdestregctx->REG_uiGplya  = pregctx->REG_uiGplya;
        pdestregctx->REG_uiIlc    = pregctx->REG_uiIlc;
        pdestregctx->REG_uiA13    = pregctx->REG_uiA13;
        pdestregctx->REG_uiA12    = pregctx->REG_uiA12;
        pdestregctx->REG_uiB13    = pregctx->REG_uiB13;
        pdestregctx->REG_uiB12    = pregctx->REG_uiB12;
        pdestregctx->REG_uiA11    = pregctx->REG_uiA11;
        pdestregctx->REG_uiA10    = pregctx->REG_uiA10;
        pdestregctx->REG_uiB11    = pregctx->REG_uiB11;
        pdestregctx->REG_uiB10    = pregctx->REG_uiB10;
        pdestregctx->REG_uiA15    = pregctx->REG_uiA15;
        pdestregctx->REG_uiA14    = pregctx->REG_uiA14;

        pdestregctx->REG_uiB14    = pregctx->REG_uiB14;
        pdestregctx->REG_uiA4     = pregctx->REG_uiA4;

        regSp   += sizeof(ARCH_REG_CTX);
        *pregSp  = regSp;
    }
}
/*********************************************************************************************************
** ��������: archTaskRegsSet
** ��������: ͨ��ջ��ָ�����üĴ����� (��ջ�ṹ)
** �䡡��  : pstkTop        ��ջ����
**           pregctx        �Ĵ�����
** �䡡��  : �Ĵ����ṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTaskRegsSet (PLW_CLASS_TCB  ptcb, const ARCH_REG_IRQ_CTX  *pregctx)
{
    if (ptcb->TCB_ulContextType) {
        ARCH_REG_IRQ_CTX  *pdestregctx = (ARCH_REG_IRQ_CTX *)ptcb->TCB_pstkStackNow;
        *pdestregctx = *pregctx;

    } else {
        ARCH_REG_CTX  *pdestregctx = (ARCH_REG_CTX *)ptcb->TCB_pstkStackNow;

        pdestregctx->REG_uiCsr    = pregctx->REG_uiCsr;
        pdestregctx->REG_uiAmr    = pregctx->REG_uiAmr;
        pdestregctx->REG_uiGfpgfr = pregctx->REG_uiGfpgfr;
        pdestregctx->REG_uiB3     = pregctx->REG_uiIrp;
        pdestregctx->REG_uiFmcr   = pregctx->REG_uiFmcr;
        pdestregctx->REG_uiFaucr  = pregctx->REG_uiFaucr;
        pdestregctx->REG_uiFadcr  = pregctx->REG_uiFadcr;
        pdestregctx->REG_uiSsr    = pregctx->REG_uiSsr;
        pdestregctx->REG_uiRilc   = pregctx->REG_uiRilc;
        pdestregctx->REG_uiItsr   = pregctx->REG_uiItsr;
        pdestregctx->REG_uiGplyb  = pregctx->REG_uiGplyb;
        pdestregctx->REG_uiGplya  = pregctx->REG_uiGplya;
        pdestregctx->REG_uiIlc    = pregctx->REG_uiIlc;
        pdestregctx->REG_uiA13    = pregctx->REG_uiA13;
        pdestregctx->REG_uiA12    = pregctx->REG_uiA12;
        pdestregctx->REG_uiB13    = pregctx->REG_uiB13;
        pdestregctx->REG_uiB12    = pregctx->REG_uiB12;
        pdestregctx->REG_uiA11    = pregctx->REG_uiA11;
        pdestregctx->REG_uiA10    = pregctx->REG_uiA10;
        pdestregctx->REG_uiB11    = pregctx->REG_uiB11;
        pdestregctx->REG_uiB10    = pregctx->REG_uiB10;
        pdestregctx->REG_uiA15    = pregctx->REG_uiA15;
        pdestregctx->REG_uiA14    = pregctx->REG_uiA14;

        pdestregctx->REG_uiB14    = pregctx->REG_uiB14;
        pdestregctx->REG_uiA4     = pregctx->REG_uiA4;
    }
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
    ARCH_REG_IRQ_CTX    *pregctx = (ARCH_REG_IRQ_CTX *)pstkTop;

    if (iFd >= 0) {
        fdprintf(iFd, "\n");

        fdprintf(iFd, "A0    = 0x%08x  ", pregctx->REG_uiA0);
        fdprintf(iFd, "A1    = 0x%08x\n", pregctx->REG_uiA1);
        fdprintf(iFd, "A2    = 0x%08x  ", pregctx->REG_uiA2);
        fdprintf(iFd, "A3    = 0x%08x\n", pregctx->REG_uiA3);
        fdprintf(iFd, "A4    = 0x%08x  ", pregctx->REG_uiA4);
        fdprintf(iFd, "A5    = 0x%08x\n", pregctx->REG_uiA5);
        fdprintf(iFd, "A6    = 0x%08x  ", pregctx->REG_uiA6);
        fdprintf(iFd, "A7    = 0x%08x\n", pregctx->REG_uiA7);
        fdprintf(iFd, "A8    = 0x%08x  ", pregctx->REG_uiA8);
        fdprintf(iFd, "A9    = 0x%08x\n", pregctx->REG_uiA9);
        fdprintf(iFd, "A10   = 0x%08x  ", pregctx->REG_uiA10);
        fdprintf(iFd, "A11   = 0x%08x\n", pregctx->REG_uiA11);
        fdprintf(iFd, "A12   = 0x%08x  ", pregctx->REG_uiA12);
        fdprintf(iFd, "A13   = 0x%08x\n", pregctx->REG_uiA13);
        fdprintf(iFd, "A14   = 0x%08x  ", pregctx->REG_uiA14);
        fdprintf(iFd, "A15   = 0x%08x\n", pregctx->REG_uiA15);
        fdprintf(iFd, "A16   = 0x%08x  ", pregctx->REG_uiA16);
        fdprintf(iFd, "A17   = 0x%08x\n", pregctx->REG_uiA17);
        fdprintf(iFd, "A18   = 0x%08x  ", pregctx->REG_uiA18);
        fdprintf(iFd, "A19   = 0x%08x\n", pregctx->REG_uiA19);
        fdprintf(iFd, "A20   = 0x%08x  ", pregctx->REG_uiA20);
        fdprintf(iFd, "A21   = 0x%08x\n", pregctx->REG_uiA21);
        fdprintf(iFd, "A22   = 0x%08x  ", pregctx->REG_uiA22);
        fdprintf(iFd, "A23   = 0x%08x\n", pregctx->REG_uiA23);
        fdprintf(iFd, "A24   = 0x%08x  ", pregctx->REG_uiA24);
        fdprintf(iFd, "A25   = 0x%08x\n", pregctx->REG_uiA25);
        fdprintf(iFd, "A26   = 0x%08x  ", pregctx->REG_uiA26);
        fdprintf(iFd, "A27   = 0x%08x\n", pregctx->REG_uiA27);
        fdprintf(iFd, "A28   = 0x%08x  ", pregctx->REG_uiA28);
        fdprintf(iFd, "A29   = 0x%08x\n", pregctx->REG_uiA29);
        fdprintf(iFd, "A30   = 0x%08x  ", pregctx->REG_uiA30);
        fdprintf(iFd, "A31   = 0x%08x\n", pregctx->REG_uiA31);

        fdprintf(iFd, "B0    = 0x%08x  ", pregctx->REG_uiB0);
        fdprintf(iFd, "B1    = 0x%08x\n", pregctx->REG_uiB1);
        fdprintf(iFd, "B2    = 0x%08x  ", pregctx->REG_uiB2);
        fdprintf(iFd, "B3    = 0x%08x\n", pregctx->REG_uiB3);
        fdprintf(iFd, "B4    = 0x%08x  ", pregctx->REG_uiB4);
        fdprintf(iFd, "B5    = 0x%08x\n", pregctx->REG_uiB5);
        fdprintf(iFd, "B6    = 0x%08x  ", pregctx->REG_uiB6);
        fdprintf(iFd, "B7    = 0x%08x\n", pregctx->REG_uiB7);
        fdprintf(iFd, "B8    = 0x%08x  ", pregctx->REG_uiB8);
        fdprintf(iFd, "B9    = 0x%08x\n", pregctx->REG_uiB9);
        fdprintf(iFd, "B10   = 0x%08x  ", pregctx->REG_uiB10);
        fdprintf(iFd, "B11   = 0x%08x\n", pregctx->REG_uiB11);
        fdprintf(iFd, "B12   = 0x%08x  ", pregctx->REG_uiB12);
        fdprintf(iFd, "B13   = 0x%08x\n", pregctx->REG_uiB13);
        fdprintf(iFd, "B14   = 0x%08x  ", pregctx->REG_uiB14);
        fdprintf(iFd, "B15   = 0x%08x\n", (ARCH_REG_T)pstkTop);
        fdprintf(iFd, "B16   = 0x%08x  ", pregctx->REG_uiB16);
        fdprintf(iFd, "B17   = 0x%08x\n", pregctx->REG_uiB17);
        fdprintf(iFd, "B18   = 0x%08x  ", pregctx->REG_uiB18);
        fdprintf(iFd, "B19   = 0x%08x\n", pregctx->REG_uiB19);
        fdprintf(iFd, "B20   = 0x%08x  ", pregctx->REG_uiB20);
        fdprintf(iFd, "B21   = 0x%08x\n", pregctx->REG_uiB21);
        fdprintf(iFd, "B22   = 0x%08x  ", pregctx->REG_uiB22);
        fdprintf(iFd, "B23   = 0x%08x\n", pregctx->REG_uiB23);
        fdprintf(iFd, "B24   = 0x%08x  ", pregctx->REG_uiB24);
        fdprintf(iFd, "B25   = 0x%08x\n", pregctx->REG_uiB25);
        fdprintf(iFd, "B26   = 0x%08x  ", pregctx->REG_uiB26);
        fdprintf(iFd, "B27   = 0x%08x\n", pregctx->REG_uiB27);
        fdprintf(iFd, "B28   = 0x%08x  ", pregctx->REG_uiB28);
        fdprintf(iFd, "B29   = 0x%08x\n", pregctx->REG_uiB29);
        fdprintf(iFd, "B30   = 0x%08x  ", pregctx->REG_uiB30);
        fdprintf(iFd, "B31   = 0x%08x\n", pregctx->REG_uiB31);

        fdprintf(iFd, "CSR   = 0x%08x  ", pregctx->REG_uiCsr);
        fdprintf(iFd, "AMR   = 0x%08x\n", pregctx->REG_uiAmr);

        fdprintf(iFd, "IRP   = 0x%08x  ", pregctx->REG_uiIrp);
        fdprintf(iFd, "FMCR  = 0x%08x\n", pregctx->REG_uiFmcr);

        fdprintf(iFd, "FAUCR = 0x%08x  ", pregctx->REG_uiFaucr);
        fdprintf(iFd, "FADCR = 0x%08x\n", pregctx->REG_uiFadcr);

        fdprintf(iFd, "SSR   = 0x%08x  ", pregctx->REG_uiSsr);
        fdprintf(iFd, "ILC   = 0x%08x\n", pregctx->REG_uiIlc);

        fdprintf(iFd, "RILC  = 0x%08x  ", pregctx->REG_uiRilc);
        fdprintf(iFd, "ITSR  = 0x%08x\n", pregctx->REG_uiItsr);

        fdprintf(iFd, "GPLYA = 0x%08x  ", pregctx->REG_uiGplya);
        fdprintf(iFd, "GPLYB = 0x%08x\n", pregctx->REG_uiGplya);

        fdprintf(iFd, "GFPGFR= 0x%08x  ", pregctx->REG_uiGfpgfr);
        fdprintf(iFd, "SP    = 0x%08x\n", (ARCH_REG_T)pstkTop);         /*  �쳣ѹջ��� SP             */

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
    ARCH_REG_IRQ_CTX    *pregctx = (ARCH_REG_IRQ_CTX *)pstkTop;

    _PrintFormat("\r\n");

    _PrintFormat("A0    = 0x%08x  ",   pregctx->REG_uiA0);
    _PrintFormat("A1    = 0x%08x\r\n", pregctx->REG_uiA1);
    _PrintFormat("A2    = 0x%08x  ",   pregctx->REG_uiA2);
    _PrintFormat("A3    = 0x%08x\r\n", pregctx->REG_uiA3);
    _PrintFormat("A4    = 0x%08x  ",   pregctx->REG_uiA4);
    _PrintFormat("A5    = 0x%08x\r\n", pregctx->REG_uiA5);
    _PrintFormat("A6    = 0x%08x  ",   pregctx->REG_uiA6);
    _PrintFormat("A7    = 0x%08x\r\n", pregctx->REG_uiA7);
    _PrintFormat("A8    = 0x%08x  ",   pregctx->REG_uiA8);
    _PrintFormat("A9    = 0x%08x\r\n", pregctx->REG_uiA9);
    _PrintFormat("A10   = 0x%08x  ",   pregctx->REG_uiA10);
    _PrintFormat("A11   = 0x%08x\r\n", pregctx->REG_uiA11);
    _PrintFormat("A12   = 0x%08x  ",   pregctx->REG_uiA12);
    _PrintFormat("A13   = 0x%08x\r\n", pregctx->REG_uiA13);
    _PrintFormat("A14   = 0x%08x  ",   pregctx->REG_uiA14);
    _PrintFormat("A15   = 0x%08x\r\n", pregctx->REG_uiA15);
    _PrintFormat("A16   = 0x%08x  ",   pregctx->REG_uiA16);
    _PrintFormat("A17   = 0x%08x\r\n", pregctx->REG_uiA17);
    _PrintFormat("A18   = 0x%08x  ",   pregctx->REG_uiA18);
    _PrintFormat("A19   = 0x%08x\r\n", pregctx->REG_uiA19);
    _PrintFormat("A20   = 0x%08x  ",   pregctx->REG_uiA20);
    _PrintFormat("A21   = 0x%08x\r\n", pregctx->REG_uiA21);
    _PrintFormat("A22   = 0x%08x  ",   pregctx->REG_uiA22);
    _PrintFormat("A23   = 0x%08x\r\n", pregctx->REG_uiA23);
    _PrintFormat("A24   = 0x%08x  ",   pregctx->REG_uiA24);
    _PrintFormat("A25   = 0x%08x\r\n", pregctx->REG_uiA25);
    _PrintFormat("A26   = 0x%08x  ",   pregctx->REG_uiA26);
    _PrintFormat("A27   = 0x%08x\r\n", pregctx->REG_uiA27);
    _PrintFormat("A28   = 0x%08x  ",   pregctx->REG_uiA28);
    _PrintFormat("A29   = 0x%08x\r\n", pregctx->REG_uiA29);
    _PrintFormat("A30   = 0x%08x  ",   pregctx->REG_uiA30);
    _PrintFormat("A31   = 0x%08x\r\n", pregctx->REG_uiA31);

    _PrintFormat("B0    = 0x%08x  ",   pregctx->REG_uiB0);
    _PrintFormat("B1    = 0x%08x\r\n", pregctx->REG_uiB1);
    _PrintFormat("B2    = 0x%08x  ",   pregctx->REG_uiB2);
    _PrintFormat("B3    = 0x%08x\r\n", pregctx->REG_uiB3);
    _PrintFormat("B4    = 0x%08x  ",   pregctx->REG_uiB4);
    _PrintFormat("B5    = 0x%08x\r\n", pregctx->REG_uiB5);
    _PrintFormat("B6    = 0x%08x  ",   pregctx->REG_uiB6);
    _PrintFormat("B7    = 0x%08x\r\n", pregctx->REG_uiB7);
    _PrintFormat("B8    = 0x%08x  ",   pregctx->REG_uiB8);
    _PrintFormat("B9    = 0x%08x\r\n", pregctx->REG_uiB9);
    _PrintFormat("B10   = 0x%08x  ",   pregctx->REG_uiB10);
    _PrintFormat("B11   = 0x%08x\r\n", pregctx->REG_uiB11);
    _PrintFormat("B12   = 0x%08x  ",   pregctx->REG_uiB12);
    _PrintFormat("B13   = 0x%08x\r\n", pregctx->REG_uiB13);
    _PrintFormat("B14   = 0x%08x  ",   pregctx->REG_uiB14);
    _PrintFormat("B15   = 0x%08x\r\n", (ARCH_REG_T)pstkTop);
    _PrintFormat("B16   = 0x%08x  ",   pregctx->REG_uiB16);
    _PrintFormat("B17   = 0x%08x\r\n", pregctx->REG_uiB17);
    _PrintFormat("B18   = 0x%08x  ",   pregctx->REG_uiB18);
    _PrintFormat("B19   = 0x%08x\r\n", pregctx->REG_uiB19);
    _PrintFormat("B20   = 0x%08x  ",   pregctx->REG_uiB20);
    _PrintFormat("B21   = 0x%08x\r\n", pregctx->REG_uiB21);
    _PrintFormat("B22   = 0x%08x  ",   pregctx->REG_uiB22);
    _PrintFormat("B23   = 0x%08x\r\n", pregctx->REG_uiB23);
    _PrintFormat("B24   = 0x%08x  ",   pregctx->REG_uiB24);
    _PrintFormat("B25   = 0x%08x\r\n", pregctx->REG_uiB25);
    _PrintFormat("B26   = 0x%08x  ",   pregctx->REG_uiB26);
    _PrintFormat("B27   = 0x%08x\r\n", pregctx->REG_uiB27);
    _PrintFormat("B28   = 0x%08x  ",   pregctx->REG_uiB28);
    _PrintFormat("B29   = 0x%08x\r\n", pregctx->REG_uiB29);
    _PrintFormat("B30   = 0x%08x  ",   pregctx->REG_uiB30);
    _PrintFormat("B31   = 0x%08x\r\n", pregctx->REG_uiB31);

    _PrintFormat("CSR   = 0x%08x  ",   pregctx->REG_uiCsr);
    _PrintFormat("AMR   = 0x%08x\r\n", pregctx->REG_uiAmr);

    _PrintFormat("IRP   = 0x%08x  ",   pregctx->REG_uiIrp);
    _PrintFormat("FMCR  = 0x%08x\r\n", pregctx->REG_uiFmcr);

    _PrintFormat("FAUCR = 0x%08x  ",   pregctx->REG_uiFaucr);
    _PrintFormat("FADCR = 0x%08x\r\n", pregctx->REG_uiFadcr);

    _PrintFormat("SSR   = 0x%08x  ",   pregctx->REG_uiSsr);
    _PrintFormat("ILC   = 0x%08x\r\n", pregctx->REG_uiIlc);

    _PrintFormat("RILC  = 0x%08x  ",   pregctx->REG_uiRilc);
    _PrintFormat("ITSR  = 0x%08x\r\n", pregctx->REG_uiItsr);

    _PrintFormat("GPLYA = 0x%08x  ",   pregctx->REG_uiGplya);
    _PrintFormat("GPLYB = 0x%08x\r\n", pregctx->REG_uiGplya);

    _PrintFormat("GFPGFR= 0x%08x  ",   pregctx->REG_uiGfpgfr);
    _PrintFormat("SP    = 0x%08x\r\n", (ARCH_REG_T)pstkTop);            /*  �쳣ѹջ��� SP             */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
