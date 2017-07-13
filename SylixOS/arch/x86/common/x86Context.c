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
** ��   ��   ��: x86Context.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 04 ��
**
** ��        ��: x86 ��ϵ���������Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86Segment.h"
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

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, 8);                        /*  ��֤��ջ�� SP 8 �ֽڶ���    */

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiArg     = (ARCH_REG_T)pvArg;
    pfpctx->FP_uiRetAddr = (ARCH_REG_T)LW_NULL;

    lib_bzero(pregctx, sizeof(ARCH_REG_CTX));

    pregctx->REG_uiEAX = 0xeaeaeaea;                                    /*  4 �����ݼĴ���              */
    pregctx->REG_uiEBX = 0xebebebeb;
    pregctx->REG_uiECX = 0xecececec;
    pregctx->REG_uiEDX = 0xedededed;

    pregctx->REG_uiESI   = 0xe0e0e0e0;
    pregctx->REG_uiEDI   = 0xe1e1e1e1;                                  /*  2 ����ַ��ָ��Ĵ���        */
    pregctx->REG_uiEBP   = (ARCH_REG_T)pfpctx;                          /*  EBP ָ��Ĵ���              */
    pregctx->REG_uiError = 0x00000000;                                  /*  ERROR CODE                  */
    pregctx->REG_uiEIP   = (ARCH_REG_T)pfuncTask;

    pregctx->REG_uiCS = X86_CS_KERNEL;                                  /*  6 ���μĴ���                */
    pregctx->REG_usDS = X86_DS_KERNEL;
    pregctx->REG_usES = X86_DS_KERNEL;
    pregctx->REG_usSS = X86_DS_KERNEL;
    pregctx->REG_usFS = X86_DS_KERNEL;
    pregctx->REG_usGS = X86_DS_KERNEL;

    pregctx->REG_uiEFLAGS = X86_EFLAGS_IF;                              /*  �����ж�ʹ��λ              */

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

    pregctxDest->REG_uiEBP = (ARCH_REG_T)pregctxSrc->REG_uiEBP;
    pfpctx->FP_uiRetAddr   = (ARCH_REG_T)pregctxSrc->REG_uiEIP;
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
** ��������: ͨ��ջ��ָ�����üĴ����� (��ջ�ṹ)
** �䡡��  : pstkTop        ��ջ����
**           pregctx        �Ĵ�����
** �䡡��  : �Ĵ����ṹ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���޸ĶμĴ���, EBP ESP
*********************************************************************************************************/
VOID  archTaskRegsSet (PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx)
{
    ARCH_REG_CTX  *pdestregctx = (ARCH_REG_CTX *)pstkTop;

    pdestregctx->REG_uiEAX = pregctx->REG_uiEAX;
    pdestregctx->REG_uiECX = pregctx->REG_uiECX;
    pdestregctx->REG_uiEDX = pregctx->REG_uiEDX;
    pdestregctx->REG_uiEBX = pregctx->REG_uiEBX;

    pdestregctx->REG_uiESI = pregctx->REG_uiESI;
    pdestregctx->REG_uiEDI = pregctx->REG_uiEDI;
    pdestregctx->REG_uiEIP = pregctx->REG_uiEIP;

    pdestregctx->REG_uiEFLAGS = pregctx->REG_uiEFLAGS;
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
        fdprintf(iFd, "EFLAGS = 0x%08x\n", pregctx->REG_uiEFLAGS);

        fdprintf(iFd, "EIP = 0x%08x  ", pregctx->REG_uiEIP);
        fdprintf(iFd, "EBP = 0x%08x\n", pregctx->REG_uiEBP);
        fdprintf(iFd, "ESI = 0x%08x  ", pregctx->REG_uiESI);
        fdprintf(iFd, "EDI = 0x%08x\n", pregctx->REG_uiEDI);

        fdprintf(iFd, "EAX = 0x%08x  ", pregctx->REG_uiEAX);
        fdprintf(iFd, "EBX = 0x%08x\n", pregctx->REG_uiEBX);
        fdprintf(iFd, "ECX = 0x%08x  ", pregctx->REG_uiECX);
        fdprintf(iFd, "EDX = 0x%08x\n", pregctx->REG_uiEDX);

        fdprintf(iFd, "SS  = 0x%08x  ", pregctx->REG_usSS);
        fdprintf(iFd, "GS  = 0x%08x\n", pregctx->REG_usGS);
        fdprintf(iFd, "FS  = 0x%08x  ", pregctx->REG_usFS);
        fdprintf(iFd, "ES  = 0x%08x\n", pregctx->REG_usES);
        fdprintf(iFd, "DS  = 0x%08x  ", pregctx->REG_usDS);
        fdprintf(iFd, "CS  = 0x%08x\n", pregctx->REG_uiCS);

        fdprintf(iFd, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);           /*  �쳣ѹջ��� SP             */

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
    _PrintFormat("EFLAGS = 0x%08x\r\n", pregctx->REG_uiEFLAGS);

    _PrintFormat("EIP = 0x%08x  ",   pregctx->REG_uiEIP);
    _PrintFormat("EBP = 0x%08x\r\n", pregctx->REG_uiEBP);
    _PrintFormat("ESI = 0x%08x  ",   pregctx->REG_uiESI);
    _PrintFormat("EDI = 0x%08x\r\n", pregctx->REG_uiEDI);

    _PrintFormat("EAX = 0x%08x  ",   pregctx->REG_uiEAX);
    _PrintFormat("EBX = 0x%08x\r\n", pregctx->REG_uiEBX);
    _PrintFormat("ECX = 0x%08x  ",   pregctx->REG_uiECX);
    _PrintFormat("EDX = 0x%08x\r\n", pregctx->REG_uiEDX);

    _PrintFormat("SS  = 0x%08x  ",   pregctx->REG_usSS);
    _PrintFormat("GS  = 0x%08x\r\n", pregctx->REG_usGS);
    _PrintFormat("FS  = 0x%08x  ",   pregctx->REG_usFS);
    _PrintFormat("ES  = 0x%08x\r\n", pregctx->REG_usES);
    _PrintFormat("DS  = 0x%08x  ",   pregctx->REG_usDS);
    _PrintFormat("CS  = 0x%08x\r\n", pregctx->REG_uiCS);

    _PrintFormat("SP  = 0x%08x\r\n", (ARCH_REG_T)pstkTop);              /*  �쳣ѹջ��� SP             */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
