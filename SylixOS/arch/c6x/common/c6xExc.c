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
** ��   ��   ��: c6xExc.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 03 �� 17 ��
**
** ��        ��: c6x ��ϵ�����쳣/�жϴ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include <linux/compat.h>
/*********************************************************************************************************
  ����ʹ���������
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slVectorTable)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slVectorTable)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: archIntHandle
** ��������: bspIntHandle ��Ҫ���ô˺��������ж� (�ر��ж����������)
** �䡡��  : ulVector         �ж�����
**           bPreemptive      �ж��Ƿ����ռ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t  irqret;

    if (_Inter_Vector_Invalid(ulVector)) {
        return;                                                         /*  �����Ų���ȷ                */
    }

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_PREEMPTIVE) {
        bPreemptive = LW_TRUE;
    }

    if (bPreemptive) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  ���� vector �ж�            */
        VECTOR_OP_UNLOCK();
        KN_INT_ENABLE_FORCE();                                          /*  �����ж�                    */
    }

    irqret = API_InterVectorIsr(ulVector);                              /*  �����жϷ������            */

    KN_INT_DISABLE();                                                   /*  �����ж�                    */

    if (bPreemptive) {
        if (irqret != LW_IRQ_HANDLED_DISV) {
            VECTOR_OP_LOCK();
            __ARCH_INT_VECTOR_ENABLE(ulVector);                         /*  ���� vector �ж�            */
            VECTOR_OP_UNLOCK();
        }

    } else if (irqret == LW_IRQ_HANDLED_DISV) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  ���� vector �ж�            */
        VECTOR_OP_UNLOCK();
    }
}
/*********************************************************************************************************
  �쳣��ؼĴ���
*********************************************************************************************************/
extern volatile cregister UINT32  EFR;
extern volatile cregister UINT32  ECR;
extern volatile cregister UINT32  IERR;
/*********************************************************************************************************
  �쳣��ز�����
*********************************************************************************************************/
#define disable_exception()
#define get_except_type()       EFR
#define ack_exception(type)     ECR = 1 << (type)
#define get_iexcept()           IERR
#define set_iexcept(mask)       IERR = (mask)
/*********************************************************************************************************
  �쳣����
*********************************************************************************************************/
#define EXCEPT_TYPE_NXF         31                                      /*  NMI                         */
#define EXCEPT_TYPE_EXC         30                                      /*  external exception          */
#define EXCEPT_TYPE_IXF         1                                       /*  internal exception          */
#define EXCEPT_TYPE_SXF         0                                       /*  software exception          */

#define EXCEPT_CAUSE_LBX        (1 << 7)                                /*  loop buffer exception       */
#define EXCEPT_CAUSE_PRX        (1 << 6)                                /*  privilege exception         */
#define EXCEPT_CAUSE_RAX        (1 << 5)                                /*  resource access exception   */
#define EXCEPT_CAUSE_RCX        (1 << 4)                                /*  resource conflict exception */
#define EXCEPT_CAUSE_OPX        (1 << 3)                                /*  opcode exception            */
#define EXCEPT_CAUSE_EPX        (1 << 2)                                /*  execute packet exception    */
#define EXCEPT_CAUSE_FPX        (1 << 1)                                /*  fetch packet exception      */
#define EXCEPT_CAUSE_IFX        (1 << 0)                                /*  instruction fetch exception */
/*********************************************************************************************************
  �쳣��Ϣ
*********************************************************************************************************/
typedef struct {
    CPCHAR      EXCI_pcString;
    INT         EXCI_iSigNo;
    INT         EXCI_iCode;
} ARCH_C6X_EXC_INFO;
/*********************************************************************************************************
  �ڲ��쳣��Ϣ
*********************************************************************************************************/
static ARCH_C6X_EXC_INFO    _G_c6xIntExcTbl[11] = {
    { "Oops - instruction fetch",     LW_VMM_ABORT_TYPE_BUS,   BUS_ADRERR },
    { "Oops - fetch packet",          LW_VMM_ABORT_TYPE_BUS,   BUS_ADRERR },
    { "Oops - execute packet",        LW_VMM_ABORT_TYPE_UNDEF, ILL_ILLOPC },
    { "Oops - undefined instruction", LW_VMM_ABORT_TYPE_UNDEF, ILL_ILLOPC },
    { "Oops - resource conflict",     LW_VMM_ABORT_TYPE_UNDEF, ILL_ILLOPC },
    { "Oops - resource access",       LW_VMM_ABORT_TYPE_UNDEF, ILL_PRVREG },
    { "Oops - privilege",             LW_VMM_ABORT_TYPE_UNDEF, ILL_PRVOPC },
    { "Oops - loops buffer",          LW_VMM_ABORT_TYPE_UNDEF, ILL_ILLOPC },
    { "Oops - software exception",    LW_VMM_ABORT_TYPE_UNDEF, ILL_ILLTRP },
    { "Oops - unknown exception",     LW_VMM_ABORT_TYPE_UNDEF, ILL_ILLOPC },
    { "Oops - fatal error",           LW_VMM_ABORT_TYPE_FATAL_ERROR, ILL_ILLOPC }
};
/*********************************************************************************************************
  �ⲿ�쳣��Ϣ
*********************************************************************************************************/
static ARCH_C6X_EXC_INFO    _G_c6xExtExcTbl[128] = {
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },

    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },

    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },

    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },
    { "Oops - external exception", LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR },

    { "Oops - CPU memory protection fault",        LW_VMM_ABORT_TYPE_MAP, SEGV_ACCERR },
    { "Oops - CPU memory protection fault in L1P", LW_VMM_ABORT_TYPE_MAP, SEGV_ACCERR },
    { "Oops - DMA memory protection fault in L1P", LW_VMM_ABORT_TYPE_MAP, SEGV_ACCERR },
    { "Oops - CPU memory protection fault in L1D", LW_VMM_ABORT_TYPE_MAP, SEGV_ACCERR },
    { "Oops - DMA memory protection fault in L1D", LW_VMM_ABORT_TYPE_MAP, SEGV_ACCERR },
    { "Oops - CPU memory protection fault in L2",  LW_VMM_ABORT_TYPE_MAP, SEGV_ACCERR },
    { "Oops - DMA memory protection fault in L2",  LW_VMM_ABORT_TYPE_MAP, SEGV_ACCERR },
    { "Oops - EMC CPU memory protection fault",    LW_VMM_ABORT_TYPE_MAP, SEGV_ACCERR },

    { "Oops - EMC bus error",      LW_VMM_ABORT_TYPE_BUS, BUS_ADRERR }
};
/*********************************************************************************************************
** ��������: archExcInit
** ��������: ��ʼ���쳣
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archExcInit (VOID)
{
    INTREG  intreg;

    ack_exception(EXCEPT_TYPE_NXF);                                     /*  ����������쳣              */
    ack_exception(EXCEPT_TYPE_EXC);
    ack_exception(EXCEPT_TYPE_IXF);
    ack_exception(EXCEPT_TYPE_SXF);

    intreg = KN_INT_DISABLE();
    archExcEnable();                                                    /*  ʹ���쳣                    */
    KN_INT_ENABLE(intreg);
}
/*********************************************************************************************************
** ��������: archExcProcess
** ��������: �쳣����
** �䡡��  : pExcInfo      �쳣��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archExcProcess (ARCH_C6X_EXC_INFO  *pExcInfo)
{
    PLW_CLASS_TCB      ptcbCur;
    LW_VMM_ABORT       abtInfo;
    ARCH_REG_IRQ_CTX  *pregctx;

    LW_TCB_GET_CUR(ptcbCur);

    pregctx = (ARCH_REG_IRQ_CTX *)ptcbCur->TCB_pstkStackNow;

    abtInfo.VMABT_uiType   = pExcInfo->EXCI_iSigNo;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_uiIrp, 0, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archIntExcProcess
** ��������: ����һ���ڲ��쳣 (���ܱ�����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archIntExcProcess (VOID)
{
    INT  iIntExcReport = get_iexcept();
    INT  iIntExcNum;

    ack_exception(EXCEPT_TYPE_IXF);                                     /*  ����쳣                    */

    while (iIntExcReport) {                                             /*  ��һ�����쳣                */
        iIntExcNum     = __ffs(iIntExcReport);
        iIntExcReport &= ~(1 << iIntExcNum);
        set_iexcept(iIntExcReport);
        archExcProcess(&_G_c6xIntExcTbl[iIntExcNum]);
    }
}
/*********************************************************************************************************
** ��������: archExtExcProcess
** ��������: ����һ���ⲿ�쳣 (�ܱ�����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archExtExcProcess (VOID)
{
    INT  iExtExcNum;

    while ((iExtExcNum = bspExtExcGet()) >= 0) {                        /*  BSP ��ѯ�쳣��              */
        archExcProcess(&_G_c6xExtExcTbl[iExtExcNum]);
    }

    ack_exception(EXCEPT_TYPE_EXC);                                     /*  ����쳣                    */
}
/*********************************************************************************************************
** ��������: archExcHandle
** ��������: �쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archExcHandle (VOID)
{
    UINT  uiExcType;
    UINT  uiTypeNum;
    UINT  uiIntExcNum = 9;                                              /*  ȱʡΪδ֪�쳣              */

    while ((uiExcType = get_except_type()) != 0) {                      /*  �쳣����                    */
        uiTypeNum = fls(uiExcType) - 1;                                 /*  ����쳣���ͺ�              */

        switch (uiTypeNum) {

        case EXCEPT_TYPE_NXF:                                           /*  NMI �쳣                    */
            ack_exception(EXCEPT_TYPE_NXF);                             /*  ����쳣                    */
            if (bspNmiExcHandler() < 0) {                               /*  ���� BSP ����               */
                uiIntExcNum = 10;                                       /*  �����ɹ�: fatal error     */
                archExcProcess(&_G_c6xIntExcTbl[uiIntExcNum]);
            }
            break;

        case EXCEPT_TYPE_IXF:                                           /*  �ڲ��쳣                    */
            archIntExcProcess();
            break;

        case EXCEPT_TYPE_EXC:                                           /*  �ⲿ�쳣                    */
            archExtExcProcess();
            break;

        case EXCEPT_TYPE_SXF:
            uiIntExcNum = 8;                                            /*  ����쳣                    */

        default:
            ack_exception(uiTypeNum);                                   /*  ����쳣                    */
            archExcProcess(&_G_c6xIntExcTbl[uiIntExcNum]);
            break;
        }
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
