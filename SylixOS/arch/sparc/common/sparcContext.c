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
** ��   ��   ��: sparcContext.c
**
** ��   ��   ��: Xu.Guizhou (�����)
**
** �ļ���������: 2017 �� 05 �� 15 ��
**
** ��        ��: SPARC ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "sparcLib.h"
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
    ARCH_REG_CTX      *pregctx;
    ARCH_FP_CTX       *pfpctx;
    ARCH_REG_T         uiPsr;

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, 8);                        /*  ��֤��ջ�� SP 8 �ֽڶ���    */

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pregctx->REG_uiGlobal[0]  = 0x00000000;                             /*  Init global regs            */
    pregctx->REG_uiGlobal[1]  = 0x01010101;
    pregctx->REG_uiGlobal[2]  = 0x02020202;
    pregctx->REG_uiGlobal[3]  = 0x03030303;
    pregctx->REG_uiGlobal[4]  = 0x04040404;
    pregctx->REG_uiGlobal[5]  = 0x05050505;
    pregctx->REG_uiGlobal[6]  = 0x06060606;
    pregctx->REG_uiGlobal[7]  = 0x07070707;

    pregctx->REG_uiOutput[0]  = (ARCH_REG_T)pvArg;                      /*  Init output regs            */
    pregctx->REG_uiOutput[1]  = 0x11111111;
    pregctx->REG_uiOutput[2]  = 0x12121212;
    pregctx->REG_uiOutput[3]  = 0x13131313;
    pregctx->REG_uiOutput[4]  = 0x14141414;
    pregctx->REG_uiOutput[5]  = 0x15151515;
    pregctx->REG_uiSp         = (ARCH_REG_T)pregctx;
    pregctx->REG_uiOutput[7]  = 0x00000000;

    pregctx->REG_uiLocal[0]   = 0x20202020;                             /*  Init local regs             */
    pregctx->REG_uiLocal[1]   = 0x21212121;
    pregctx->REG_uiLocal[2]   = 0x22222222;
    pregctx->REG_uiLocal[3]   = 0x23131313;
    pregctx->REG_uiLocal[4]   = 0x24242424;
    pregctx->REG_uiLocal[5]   = 0x25252525;
    pregctx->REG_uiLocal[6]   = 0x26262626;
    pregctx->REG_uiLocal[7]   = 0x27272727;

    pregctx->REG_uiInput[0]   = 0x30303030;                             /*  Init input regs             */
    pregctx->REG_uiInput[1]   = 0x31313131;
    pregctx->REG_uiInput[2]   = 0x32323232;
    pregctx->REG_uiInput[3]   = 0x33333333;
    pregctx->REG_uiInput[4]   = 0x34343434;
    pregctx->REG_uiInput[5]   = 0x35353535;
    pregctx->REG_uiFp         = 0x00000000;
    pregctx->REG_uiInput[7]   = 0x00000000;

    uiPsr  = archPsrGet();
    uiPsr &= ~PSR_PIL;                                                  /*  ʹ���ж�                    */
    uiPsr &= ~PSR_CWP;                                                  /*  ʹ�ô��� 0                  */
    uiPsr |= PSR_S | PSR_PS;                                            /*  ��Ȩģʽ                    */
    uiPsr |= PSR_ET;                                                    /*  ʹ���쳣                    */
    uiPsr &= ~PSR_EF;                                                   /*  ���ܸ���                    */
    uiPsr &= ~PSR_EC;                                                   /*  ����Э������                */
    pregctx->REG_uiPsr = uiPsr;
    pregctx->REG_uiWim = 0x2;                                           /*  Window 1  (all empty)       */
    pregctx->REG_uiPc  = (ARCH_REG_T)pfuncTask;                         /*  PC                          */
    pregctx->REG_uiNPc = pregctx->REG_uiPc + 4;                         /*  Next PC                     */
    pregctx->REG_uiY   = 0x00000000;                                    /*  Y �Ĵ���                    */

    pfpctx->FP_uiFp       = 0x00000000;
    pfpctx->FP_uiRetAddr  = 0x00000000;

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

    pregctxDest->REG_uiFp       = (ARCH_REG_T)&pregctxSrc->REG_uiLocal[0];
    pregctxDest->REG_uiInput[7] = pregctxSrc->REG_uiPc;
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
    ARCH_REG_CTX *pregDest = (ARCH_REG_CTX *)pstkTop;

    pregDest->REG_uiGlobal[0] = pregctx->REG_uiGlobal[0];
    pregDest->REG_uiGlobal[1] = pregctx->REG_uiGlobal[1];
    pregDest->REG_uiGlobal[2] = pregctx->REG_uiGlobal[2];
    pregDest->REG_uiGlobal[3] = pregctx->REG_uiGlobal[3];
    pregDest->REG_uiGlobal[4] = pregctx->REG_uiGlobal[4];
    pregDest->REG_uiGlobal[5] = pregctx->REG_uiGlobal[5];
    pregDest->REG_uiGlobal[6] = pregctx->REG_uiGlobal[6];
    pregDest->REG_uiGlobal[7] = pregctx->REG_uiGlobal[7];

    pregDest->REG_uiOutput[0] = pregctx->REG_uiOutput[0];
    pregDest->REG_uiOutput[1] = pregctx->REG_uiOutput[1];
    pregDest->REG_uiOutput[2] = pregctx->REG_uiOutput[2];
    pregDest->REG_uiOutput[3] = pregctx->REG_uiOutput[3];
    pregDest->REG_uiOutput[4] = pregctx->REG_uiOutput[4];
    pregDest->REG_uiOutput[5] = pregctx->REG_uiOutput[5];
    /*
     * %o6(SP)������
     */
    pregDest->REG_uiOutput[7] = pregctx->REG_uiOutput[7];

    pregDest->REG_uiLocal[0]  = pregctx->REG_uiLocal[0];
    pregDest->REG_uiLocal[1]  = pregctx->REG_uiLocal[1];
    pregDest->REG_uiLocal[2]  = pregctx->REG_uiLocal[2];
    pregDest->REG_uiLocal[3]  = pregctx->REG_uiLocal[3];
    pregDest->REG_uiLocal[4]  = pregctx->REG_uiLocal[4];
    pregDest->REG_uiLocal[5]  = pregctx->REG_uiLocal[5];
    pregDest->REG_uiLocal[6]  = pregctx->REG_uiLocal[6];
    pregDest->REG_uiLocal[7]  = pregctx->REG_uiLocal[7];

    pregDest->REG_uiInput[0]  = pregctx->REG_uiInput[0];
    pregDest->REG_uiInput[1]  = pregctx->REG_uiInput[1];
    pregDest->REG_uiInput[2]  = pregctx->REG_uiInput[2];
    pregDest->REG_uiInput[3]  = pregctx->REG_uiInput[3];
    pregDest->REG_uiInput[4]  = pregctx->REG_uiInput[4];
    pregDest->REG_uiInput[5]  = pregctx->REG_uiInput[5];
    /*
     * %i6(FP)������
     */
    pregDest->REG_uiInput[7]  = pregctx->REG_uiInput[7];

    pregDest->REG_uiPsr       = pregctx->REG_uiPsr;
    pregDest->REG_uiTbr       = pregctx->REG_uiTbr;
    pregDest->REG_uiPc        = pregctx->REG_uiPc;
    pregDest->REG_uiNPc       = pregctx->REG_uiNPc;
    pregDest->REG_uiWim       = pregctx->REG_uiWim;
    pregDest->REG_uiY         = pregctx->REG_uiY;
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

        fdprintf(iFd, "g0  = 0x%08x  ", pregctx->REG_uiGlobal[0]);
        fdprintf(iFd, "g1  = 0x%08x\n", pregctx->REG_uiGlobal[1]);
        fdprintf(iFd, "g2  = 0x%08x  ", pregctx->REG_uiGlobal[2]);
        fdprintf(iFd, "g3  = 0x%08x\n", pregctx->REG_uiGlobal[3]);
        fdprintf(iFd, "g4  = 0x%08x  ", pregctx->REG_uiGlobal[4]);
        fdprintf(iFd, "g5  = 0x%08x\n", pregctx->REG_uiGlobal[5]);
        fdprintf(iFd, "g6  = 0x%08x  ", pregctx->REG_uiGlobal[6]);
        fdprintf(iFd, "g7  = 0x%08x\n", pregctx->REG_uiGlobal[7]);

        fdprintf(iFd, "o0  = 0x%08x  ", pregctx->REG_uiOutput[0]);
        fdprintf(iFd, "o1  = 0x%08x\n", pregctx->REG_uiOutput[1]);
        fdprintf(iFd, "o2  = 0x%08x  ", pregctx->REG_uiOutput[2]);
        fdprintf(iFd, "o3  = 0x%08x\n", pregctx->REG_uiOutput[3]);
        fdprintf(iFd, "o4  = 0x%08x  ", pregctx->REG_uiOutput[4]);
        fdprintf(iFd, "o5  = 0x%08x\n", pregctx->REG_uiOutput[5]);
        fdprintf(iFd, "o6  = 0x%08x  ", pregctx->REG_uiOutput[6]);      /*  �쳣ѹջǰ�� SP             */
        fdprintf(iFd, "o7  = 0x%08x\n", pregctx->REG_uiOutput[7]);

        fdprintf(iFd, "l0  = 0x%08x  ", pregctx->REG_uiLocal[0]);
        fdprintf(iFd, "l1  = 0x%08x\n", pregctx->REG_uiLocal[1]);
        fdprintf(iFd, "l2  = 0x%08x  ", pregctx->REG_uiLocal[2]);
        fdprintf(iFd, "l3  = 0x%08x\n", pregctx->REG_uiLocal[3]);
        fdprintf(iFd, "l4  = 0x%08x  ", pregctx->REG_uiLocal[4]);
        fdprintf(iFd, "l5  = 0x%08x\n", pregctx->REG_uiLocal[5]);
        fdprintf(iFd, "l6  = 0x%08x  ", pregctx->REG_uiLocal[6]);
        fdprintf(iFd, "l7  = 0x%08x\n", pregctx->REG_uiLocal[7]);

        fdprintf(iFd, "i0  = 0x%08x  ", pregctx->REG_uiInput[0]);
        fdprintf(iFd, "i1  = 0x%08x\n", pregctx->REG_uiInput[1]);
        fdprintf(iFd, "i2  = 0x%08x  ", pregctx->REG_uiInput[2]);
        fdprintf(iFd, "i3  = 0x%08x\n", pregctx->REG_uiInput[3]);
        fdprintf(iFd, "i4  = 0x%08x  ", pregctx->REG_uiInput[4]);
        fdprintf(iFd, "i5  = 0x%08x\n", pregctx->REG_uiInput[5]);
        fdprintf(iFd, "i6  = 0x%08x  ", pregctx->REG_uiInput[6]);       /*  FP                          */
        fdprintf(iFd, "i7  = 0x%08x\n", pregctx->REG_uiInput[7]);

        fdprintf(iFd, "PSR = 0x%08x  ", pregctx->REG_uiPsr);
        fdprintf(iFd, "Y   = 0x%08x\n", pregctx->REG_uiY);
        fdprintf(iFd, "PC  = 0x%08x  ", pregctx->REG_uiPc);
        fdprintf(iFd, "NPC = 0x%08x\n", pregctx->REG_uiNPc);

        fdprintf(iFd, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);           /*  �쳣ѹջ��� SP             */

    } else {
        archTaskCtxPrint(LW_NULL, 0, pstkTop);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: archTaskCtxPrint
** ��������: ֱ�Ӵ�ӡ����������
** �䡡��  : pvBuffer   �ڴ滺���� (NULL, ��ʾֱ�Ӵ�ӡ)
**           stSize     �����С
**           pstkTop    ��ջջ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTaskCtxPrint (PVOID  pvBuffer, size_t  stSize, PLW_STACK  pstkTop)
{
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    if (pvBuffer && stSize) {
        size_t  stOft = 0;

        stOft = bnprintf(pvBuffer, stSize, stOft, "g0  = 0x%08x  ", pregctx->REG_uiGlobal[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g1  = 0x%08x\n", pregctx->REG_uiGlobal[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g2  = 0x%08x  ", pregctx->REG_uiGlobal[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g3  = 0x%08x\n", pregctx->REG_uiGlobal[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g4  = 0x%08x  ", pregctx->REG_uiGlobal[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g5  = 0x%08x\n", pregctx->REG_uiGlobal[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g6  = 0x%08x  ", pregctx->REG_uiGlobal[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g7  = 0x%08x\n", pregctx->REG_uiGlobal[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "o0  = 0x%08x  ", pregctx->REG_uiOutput[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o1  = 0x%08x\n", pregctx->REG_uiOutput[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o2  = 0x%08x  ", pregctx->REG_uiOutput[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o3  = 0x%08x\n", pregctx->REG_uiOutput[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o4  = 0x%08x  ", pregctx->REG_uiOutput[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o5  = 0x%08x\n", pregctx->REG_uiOutput[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o6  = 0x%08x  ", pregctx->REG_uiOutput[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o7  = 0x%08x\n", pregctx->REG_uiOutput[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "l0  = 0x%08x  ", pregctx->REG_uiLocal[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l1  = 0x%08x\n", pregctx->REG_uiLocal[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l2  = 0x%08x  ", pregctx->REG_uiLocal[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l3  = 0x%08x\n", pregctx->REG_uiLocal[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l4  = 0x%08x  ", pregctx->REG_uiLocal[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l5  = 0x%08x\n", pregctx->REG_uiLocal[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l6  = 0x%08x  ", pregctx->REG_uiLocal[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l7  = 0x%08x\n", pregctx->REG_uiLocal[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "i0  = 0x%08x  ", pregctx->REG_uiInput[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i1  = 0x%08x\n", pregctx->REG_uiInput[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i2  = 0x%08x  ", pregctx->REG_uiInput[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i3  = 0x%08x\n", pregctx->REG_uiInput[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i4  = 0x%08x  ", pregctx->REG_uiInput[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i5  = 0x%08x\n", pregctx->REG_uiInput[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i6  = 0x%08x  ", pregctx->REG_uiInput[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i7  = 0x%08x\n", pregctx->REG_uiInput[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "PSR = 0x%08x  ", pregctx->REG_uiPsr);
        stOft = bnprintf(pvBuffer, stSize, stOft, "Y   = 0x%08x\n", pregctx->REG_uiY);
        stOft = bnprintf(pvBuffer, stSize, stOft, "PC  = 0x%08x  ", pregctx->REG_uiPc);
        stOft = bnprintf(pvBuffer, stSize, stOft, "NPC = 0x%08x\n", pregctx->REG_uiNPc);

        stOft = bnprintf(pvBuffer, stSize, stOft, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);

    } else {
        _PrintFormat(", pregctx");

        _PrintFormat("g0  = 0x%08x  ",   pregctx->REG_uiGlobal[0]);
        _PrintFormat("g1  = 0x%08x\r\n", pregctx->REG_uiGlobal[1]);
        _PrintFormat("g2  = 0x%08x  ",   pregctx->REG_uiGlobal[2]);
        _PrintFormat("g3  = 0x%08x\r\n", pregctx->REG_uiGlobal[3]);
        _PrintFormat("g4  = 0x%08x  ",   pregctx->REG_uiGlobal[4]);
        _PrintFormat("g5  = 0x%08x\r\n", pregctx->REG_uiGlobal[5]);
        _PrintFormat("g6  = 0x%08x  ",   pregctx->REG_uiGlobal[6]);
        _PrintFormat("g7  = 0x%08x\r\n", pregctx->REG_uiGlobal[7]);

        _PrintFormat("o0  = 0x%08x  ",   pregctx->REG_uiOutput[0]);
        _PrintFormat("o1  = 0x%08x\r\n", pregctx->REG_uiOutput[1]);
        _PrintFormat("o2  = 0x%08x  ",   pregctx->REG_uiOutput[2]);
        _PrintFormat("o3  = 0x%08x\r\n", pregctx->REG_uiOutput[3]);
        _PrintFormat("o4  = 0x%08x  ",   pregctx->REG_uiOutput[4]);
        _PrintFormat("o5  = 0x%08x\r\n", pregctx->REG_uiOutput[5]);
        _PrintFormat("o6  = 0x%08x  ",   pregctx->REG_uiOutput[6]);     /*  �쳣ѹջǰ�� SP             */
        _PrintFormat("o7  = 0x%08x\r\n", pregctx->REG_uiOutput[7]);

        _PrintFormat("l0  = 0x%08x  ",   pregctx->REG_uiLocal[0]);
        _PrintFormat("l1  = 0x%08x\r\n", pregctx->REG_uiLocal[1]);
        _PrintFormat("l2  = 0x%08x  ",   pregctx->REG_uiLocal[2]);
        _PrintFormat("l3  = 0x%08x\r\n", pregctx->REG_uiLocal[3]);
        _PrintFormat("l4  = 0x%08x  ",   pregctx->REG_uiLocal[4]);
        _PrintFormat("l5  = 0x%08x\r\n", pregctx->REG_uiLocal[5]);
        _PrintFormat("l6  = 0x%08x  ",   pregctx->REG_uiLocal[6]);
        _PrintFormat("l7  = 0x%08x\r\n", pregctx->REG_uiLocal[7]);

        _PrintFormat("i0  = 0x%08x  ",   pregctx->REG_uiInput[0]);
        _PrintFormat("i1  = 0x%08x\r\n", pregctx->REG_uiInput[1]);
        _PrintFormat("i2  = 0x%08x  ",   pregctx->REG_uiInput[2]);
        _PrintFormat("i3  = 0x%08x\r\n", pregctx->REG_uiInput[3]);
        _PrintFormat("i4  = 0x%08x  ",   pregctx->REG_uiInput[4]);
        _PrintFormat("i5  = 0x%08x\r\n", pregctx->REG_uiInput[5]);
        _PrintFormat("i6  = 0x%08x  ",   pregctx->REG_uiInput[6]);      /*  FP                          */
        _PrintFormat("i7  = 0x%08x\r\n", pregctx->REG_uiInput[7]);

        _PrintFormat("PSR = 0x%08x  ",   pregctx->REG_uiPsr);
        _PrintFormat("Y   = 0x%08x\r\n", pregctx->REG_uiY);
        _PrintFormat("PC  = 0x%08x  ",   pregctx->REG_uiPc);
        _PrintFormat("NPC = 0x%08x\r\n", pregctx->REG_uiNPc);

        _PrintFormat("SP  = 0x%08x\r\n", (ARCH_REG_T)pstkTop);          /*  �쳣ѹջ��� SP             */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
