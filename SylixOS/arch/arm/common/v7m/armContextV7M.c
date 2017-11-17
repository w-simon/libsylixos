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
** ��   ��   ��: armContextV7M.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 14 ��
**
** ��        ��: ARMv7M ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
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

    if ((addr_t)pstkTop & 0x7) {                                        /*  ��֤��ջ�� CPU SP 8 �ֽڶ���*/
        pstkTop = (PLW_STACK)((addr_t)pstkTop - 4);                     /*  ��͵�ַ�ƽ� 4 �ֽ�         */
    }

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiFp = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_uiLr = (ARCH_REG_T)LW_NULL;

    pregctx->REG_uiCpsr = 0x01000000;
    pregctx->REG_uiR0   = (ARCH_REG_T)pvArg;
    pregctx->REG_uiR1   = 0x01010101;
    pregctx->REG_uiR2   = 0x02020202;
    pregctx->REG_uiR3   = 0x03030303;
    pregctx->REG_uiR4   = 0x04040404;
    pregctx->REG_uiR5   = 0x05050505;
    pregctx->REG_uiR6   = 0x06060606;
    pregctx->REG_uiR7   = 0x07070707;
    pregctx->REG_uiR8   = 0x08080808;
    pregctx->REG_uiR9   = 0x09090909;
    pregctx->REG_uiR10  = 0x10101010;
    pregctx->REG_uiFp   = pfpctx->FP_uiFp;
    pregctx->REG_uiIp   = 0x12121212;
    pregctx->REG_uiLr   = 0xfffffffe;
    pregctx->REG_uiPc   = (ARCH_REG_T)pfuncTask;

    pregctx->REG_uiExcRet  = 0xfffffffd;
    pregctx->REG_uiBASEPRI = 0;

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
    if (iFd >= 0) {
        fdprintf(iFd, "XPSR    = 0x%08x\n", pstkTop[17]);
        fdprintf(iFd, "BASEPRI = 0x%08x\n", pstkTop[0]);
        fdprintf(iFd, "EXCRET  = 0x%08x\n", pstkTop[9]);

        fdprintf(iFd, "r0  = 0x%08x  ", pstkTop[10]);
        fdprintf(iFd, "r1  = 0x%08x\n", pstkTop[11]);
        fdprintf(iFd, "r2  = 0x%08x  ", pstkTop[12]);
        fdprintf(iFd, "r3  = 0x%08x\n", pstkTop[13]);
        fdprintf(iFd, "r4  = 0x%08x  ", pstkTop[1]);
        fdprintf(iFd, "r5  = 0x%08x\n", pstkTop[2]);
        fdprintf(iFd, "r6  = 0x%08x  ", pstkTop[3]);
        fdprintf(iFd, "r7  = 0x%08x\n", pstkTop[4]);
        fdprintf(iFd, "r8  = 0x%08x  ", pstkTop[5]);
        fdprintf(iFd, "r9  = 0x%08x\n", pstkTop[6]);
        fdprintf(iFd, "r10 = 0x%08x  ", pstkTop[7]);
        fdprintf(iFd, "fp  = 0x%08x\n", pstkTop[8]);
        fdprintf(iFd, "ip  = 0x%08x  ", pstkTop[14]);
        fdprintf(iFd, "sp  = 0x%08x\n", (ARCH_REG_T)pstkTop);
        fdprintf(iFd, "lr  = 0x%08x  ", pstkTop[15]);
        fdprintf(iFd, "pc  = 0x%08x\n", pstkTop[16]);

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
    _PrintFormat("XPSR    = 0x%08x\r\n", pstkTop[17]);
    _PrintFormat("BASEPRI = 0x%08x\r\n", pstkTop[0]);
    _PrintFormat("EXCRET  = 0x%08x\r\n", pstkTop[9]);

    _PrintFormat("r0  = 0x%08x  ",   pstkTop[10]);
    _PrintFormat("r1  = 0x%08x\r\n", pstkTop[11]);
    _PrintFormat("r2  = 0x%08x  ",   pstkTop[12]);
    _PrintFormat("r3  = 0x%08x\r\n", pstkTop[13]);
    _PrintFormat("r4  = 0x%08x  ",   pstkTop[1]);
    _PrintFormat("r5  = 0x%08x\r\n", pstkTop[2]);
    _PrintFormat("r6  = 0x%08x  ",   pstkTop[3]);
    _PrintFormat("r7  = 0x%08x\r\n", pstkTop[4]);
    _PrintFormat("r8  = 0x%08x  ",   pstkTop[5]);
    _PrintFormat("r9  = 0x%08x\r\n", pstkTop[6]);
    _PrintFormat("r10 = 0x%08x  ",   pstkTop[7]);
    _PrintFormat("fp  = 0x%08x\r\n", pstkTop[8]);
    _PrintFormat("ip  = 0x%08x  ",   pstkTop[14]);
    _PrintFormat("sp  = 0x%08x\r\n", (ARCH_REG_T)pstkTop);
    _PrintFormat("lr  = 0x%08x  ",   pstkTop[15]);
    _PrintFormat("pc  = 0x%08x\r\n", pstkTop[16]);
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
