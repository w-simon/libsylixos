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
** ��   ��   ��: x64Context.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 06 �� 06 ��
**
** ��        ��: x86-64 ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../x86Segment.h"
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

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, 16);                       /*  ��֤��ջ�� SP 16 �ֽڶ���   */
    pstkTop--;                                                          /*  GCC PUSH ��תΪ 16 �ֽڶ��� */

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_ulSavedRBP = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_ulRetAddr  = (ARCH_REG_T)LW_NULL;

    lib_bzero(pregctx, sizeof(ARCH_REG_CTX));

    pregctx->REG_ulR8  = 0x0808080808080808;
    pregctx->REG_ulR9  = 0x0909090909090909;
    pregctx->REG_ulR10 = 0x1010101010101010;
    pregctx->REG_ulR11 = 0x1111111111111111;
    pregctx->REG_ulR12 = 0x1212121212121212;
    pregctx->REG_ulR13 = 0x1313131313131313;
    pregctx->REG_ulR14 = 0x1414141414141414;
    pregctx->REG_ulR15 = 0x1515151515151515;

    pregctx->REG_ulRAX = 0xeaeaeaeaeaeaeaea;
    pregctx->REG_ulRBX = 0xebebebebebebebeb;
    pregctx->REG_ulRCX = 0xecececececececec;
    pregctx->REG_ulRDX = 0xedededededededed;

    pregctx->REG_ulRSI   = 0xe0e0e0e0e0e0e0e0;
    pregctx->REG_ulRDI   = (ARCH_REG_T)pvArg;                           /*  ����                        */

    pregctx->REG_ulRBP   = (ARCH_REG_T)pfpctx;                          /*  RBP ָ��Ĵ���              */
    pregctx->REG_ulRSP   = (ARCH_REG_T)pfpctx;                          /*  RSP ָ��Ĵ���              */
    pregctx->REG_ulError = 0x0000000000000000;                          /*  ERROR CODE                  */
    pregctx->REG_ulRIP   = (ARCH_REG_T)pfuncTask;

    pregctx->REG_ulCS = X86_CS_KERNEL;
    pregctx->REG_usDS = X86_DS_KERNEL;
    pregctx->REG_usES = X86_DS_KERNEL;
    pregctx->REG_ulSS = X86_DS_KERNEL;
    pregctx->REG_usFS = X86_DS_KERNEL;
    pregctx->REG_usGS = X86_DS_KERNEL;

    pregctx->REG_ulRFLAGS = X86_EFLAGS_IF;                              /*  �����ж�ʹ��λ              */

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
    ARCH_FP_CTX       *pfpctx      = (ARCH_FP_CTX  *)((PCHAR)pstkDest + sizeof(ARCH_REG_CTX));
    ARCH_REG_CTX      *pregctxDest = (ARCH_REG_CTX *)pstkDest;
    ARCH_REG_CTX      *pregctxSrc  = (ARCH_REG_CTX *)pstkSrc;

    pregctxDest->REG_ulRBP = (ARCH_REG_T)pregctxSrc->REG_ulRBP;
    pfpctx->FP_ulRetAddr   = (ARCH_REG_T)pregctxSrc->REG_ulRIP;
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
ARCH_REG_CTX  *archTaskRegsGet (PLW_STACK  pstkTop, ARCH_REG_T  *pregSp)
{
    ARCH_REG_CTX  *pregctx = (ARCH_REG_CTX *)pstkTop;
    
    *pregSp = pregctx->REG_ulRSP;
    
    return  (pregctx);
}
/*********************************************************************************************************
** ��������: archTaskRegsSet
** ��������: ͨ��ջ��ָ�����üĴ����� (��ջ�ṹ)
** �䡡��  : pstkTop        ��ջ����
**           pregctx        �Ĵ�����
** �䡡��  : �Ĵ����ṹ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���޸ĶμĴ���, RBP RSP
*********************************************************************************************************/
VOID  archTaskRegsSet (PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx)
{
    ARCH_REG_CTX  *pdestregctx = (ARCH_REG_CTX *)pstkTop;

    pdestregctx->REG_ulR8  = pregctx->REG_ulR8;
    pdestregctx->REG_ulR9  = pregctx->REG_ulR9;
    pdestregctx->REG_ulR10 = pregctx->REG_ulR10;
    pdestregctx->REG_ulR11 = pregctx->REG_ulR11;
    pdestregctx->REG_ulR12 = pregctx->REG_ulR12;
    pdestregctx->REG_ulR13 = pregctx->REG_ulR13;
    pdestregctx->REG_ulR14 = pregctx->REG_ulR14;
    pdestregctx->REG_ulR15 = pregctx->REG_ulR15;

    pdestregctx->REG_ulRAX = pregctx->REG_ulRAX;
    pdestregctx->REG_ulRBX = pregctx->REG_ulRBX;
    pdestregctx->REG_ulRCX = pregctx->REG_ulRCX;
    pdestregctx->REG_ulRDX = pregctx->REG_ulRDX;

    pdestregctx->REG_ulRSI = pregctx->REG_ulRSI;
    pdestregctx->REG_ulRDI = pregctx->REG_ulRDI;
    pdestregctx->REG_ulRIP = pregctx->REG_ulRIP;

    pdestregctx->REG_ulRFLAGS = pregctx->REG_ulRFLAGS;
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
        fdprintf(iFd, "RFLAGS = 0x%016lx\n", pregctx->REG_ulRFLAGS);

        fdprintf(iFd, "RIP = 0x%016lx  ", pregctx->REG_ulRIP);
        fdprintf(iFd, "RBP = 0x%016lx\n", pregctx->REG_ulRBP);
        fdprintf(iFd, "RSI = 0x%016lx  ", pregctx->REG_ulRSI);
        fdprintf(iFd, "RDI = 0x%016lx\n", pregctx->REG_ulRDI);

        fdprintf(iFd, "RAX = 0x%016lx  ", pregctx->REG_ulRAX);
        fdprintf(iFd, "RBX = 0x%016lx\n", pregctx->REG_ulRBX);
        fdprintf(iFd, "RCX = 0x%016lx  ", pregctx->REG_ulRCX);
        fdprintf(iFd, "RDX = 0x%016lx\n", pregctx->REG_ulRDX);

        fdprintf(iFd, "SS  = 0x%016lx  ", pregctx->REG_ulSS);
        fdprintf(iFd, "GS  = 0x%016lx\n", pregctx->REG_usGS);
        fdprintf(iFd, "FS  = 0x%016lx  ", pregctx->REG_usFS);
        fdprintf(iFd, "ES  = 0x%016lx\n", pregctx->REG_usES);
        fdprintf(iFd, "DS  = 0x%016lx  ", pregctx->REG_usDS);
        fdprintf(iFd, "CS  = 0x%016lx\n", pregctx->REG_ulCS);

        fdprintf(iFd, "R8  = 0x%016lx  ", pregctx->REG_ulR8);
        fdprintf(iFd, "R9  = 0x%016lx\n", pregctx->REG_ulR9);
        fdprintf(iFd, "R10 = 0x%016lx  ", pregctx->REG_ulR10);
        fdprintf(iFd, "R11 = 0x%016lx\n", pregctx->REG_ulR11);
        fdprintf(iFd, "R12 = 0x%016lx  ", pregctx->REG_ulR12);
        fdprintf(iFd, "R13 = 0x%016lx\n", pregctx->REG_ulR13);
        fdprintf(iFd, "R14 = 0x%016lx  ", pregctx->REG_ulR14);
        fdprintf(iFd, "R15 = 0x%016lx\n", pregctx->REG_ulR15);

        fdprintf(iFd, "RSP = 0x%016lx  ", pregctx->REG_ulRSP);          /*  �쳣ѹջǰ�� SP             */
        fdprintf(iFd, "SP  = 0x%016lx\n", (ARCH_REG_T)pstkTop);         /*  �쳣ѹջ��� SP             */

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

    _PrintFormat("\r\n");
    _PrintFormat("RFLAGS = 0x%016qx\r\n", pregctx->REG_ulRFLAGS);

    _PrintFormat("RIP = 0x%016qx  ",   pregctx->REG_ulRIP);
    _PrintFormat("RBP = 0x%016qx\r\n", pregctx->REG_ulRBP);
    _PrintFormat("RSI = 0x%016qx  ",   pregctx->REG_ulRSI);
    _PrintFormat("RDI = 0x%016qx\r\n", pregctx->REG_ulRDI);

    _PrintFormat("RAX = 0x%016qx  ",   pregctx->REG_ulRAX);
    _PrintFormat("RBX = 0x%016qx\r\n", pregctx->REG_ulRBX);
    _PrintFormat("RCX = 0x%016qx  ",   pregctx->REG_ulRCX);
    _PrintFormat("RDX = 0x%016qx\r\n", pregctx->REG_ulRDX);

    _PrintFormat("SS  = 0x%016qx  ",   pregctx->REG_ulSS);
    _PrintFormat("GS  = 0x%016qx\r\n", pregctx->REG_usGS);
    _PrintFormat("FS  = 0x%016qx  ",   pregctx->REG_usFS);
    _PrintFormat("ES  = 0x%016qx\r\n", pregctx->REG_usES);
    _PrintFormat("DS  = 0x%016qx  ",   pregctx->REG_usDS);
    _PrintFormat("CS  = 0x%016qx\r\n", pregctx->REG_ulCS);

    _PrintFormat("R8  = 0x%016qx  ",   pregctx->REG_ulR8);
    _PrintFormat("R9  = 0x%016qx\r\n", pregctx->REG_ulR9);
    _PrintFormat("R10 = 0x%016qx  ",   pregctx->REG_ulR10);
    _PrintFormat("R11 = 0x%016qx\r\n", pregctx->REG_ulR11);
    _PrintFormat("R12 = 0x%016qx  ",   pregctx->REG_ulR12);
    _PrintFormat("R13 = 0x%016qx\r\n", pregctx->REG_ulR13);
    _PrintFormat("R14 = 0x%016qx  ",   pregctx->REG_ulR14);
    _PrintFormat("R15 = 0x%016qx\r\n", pregctx->REG_ulR15);

    _PrintFormat("RSP = 0x%016qx  ",   pregctx->REG_ulRSP);             /*  �쳣ѹջǰ�� SP             */
    _PrintFormat("SP  = 0x%016qx\r\n", (ARCH_REG_T)pstkTop);            /*  �쳣ѹջ��� SP             */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
