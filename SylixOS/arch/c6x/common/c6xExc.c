/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: c6xExc.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 03 月 17 日
**
** 描        述: c6x 体系构架异常/中断处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include <linux/compat.h>
/*********************************************************************************************************
  向量使能与禁能锁
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slVectorTable)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slVectorTable)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: archIntHandle
** 功能描述: bspIntHandle 需要调用此函数处理中断 (关闭中断情况被调用)
** 输　入  : ulVector         中断向量
**           bPreemptive      中断是否可抢占
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t  irqret;

    if (_Inter_Vector_Invalid(ulVector)) {
        return;                                                         /*  向量号不正确                */
    }

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_PREEMPTIVE) {
        bPreemptive = LW_TRUE;
    }

    if (bPreemptive) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  屏蔽 vector 中断            */
        VECTOR_OP_UNLOCK();
        KN_INT_ENABLE_FORCE();                                          /*  允许中断                    */
    }

    irqret = API_InterVectorIsr(ulVector);                              /*  调用中断服务程序            */

    KN_INT_DISABLE();                                                   /*  禁能中断                    */

    if (bPreemptive) {
        if (irqret != LW_IRQ_HANDLED_DISV) {
            VECTOR_OP_LOCK();
            __ARCH_INT_VECTOR_ENABLE(ulVector);                         /*  允许 vector 中断            */
            VECTOR_OP_UNLOCK();
        }

    } else if (irqret == LW_IRQ_HANDLED_DISV) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  屏蔽 vector 中断            */
        VECTOR_OP_UNLOCK();
    }
}
/*********************************************************************************************************
  异常相关寄存器
*********************************************************************************************************/
extern volatile cregister UINT32  EFR;
extern volatile cregister UINT32  ECR;
extern volatile cregister UINT32  IERR;
/*********************************************************************************************************
  异常相关操作宏
*********************************************************************************************************/
#define disable_exception()
#define get_except_type()       EFR
#define ack_exception(type)     ECR = 1 << (type)
#define get_iexcept()           IERR
#define set_iexcept(mask)       IERR = (mask)
/*********************************************************************************************************
  异常类型
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
  异常信息
*********************************************************************************************************/
typedef struct {
    CPCHAR      EXCI_pcString;
    INT         EXCI_iSigNo;
    INT         EXCI_iCode;
} ARCH_C6X_EXC_INFO;
/*********************************************************************************************************
  内部异常信息
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
  外部异常信息
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
** 函数名称: archExcInit
** 功能描述: 初始化异常
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archExcInit (VOID)
{
    INTREG  intreg;

    ack_exception(EXCEPT_TYPE_NXF);                                     /*  清除各类型异常              */
    ack_exception(EXCEPT_TYPE_EXC);
    ack_exception(EXCEPT_TYPE_IXF);
    ack_exception(EXCEPT_TYPE_SXF);

    intreg = KN_INT_DISABLE();
    archExcEnable();                                                    /*  使能异常                    */
    KN_INT_ENABLE(intreg);
}
/*********************************************************************************************************
** 函数名称: archExcProcess
** 功能描述: 异常处理
** 输　入  : pExcInfo      异常信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: archIntExcProcess
** 功能描述: 处理一个内部异常 (不能被屏蔽)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archIntExcProcess (VOID)
{
    INT  iIntExcReport = get_iexcept();
    INT  iIntExcNum;

    ack_exception(EXCEPT_TYPE_IXF);                                     /*  清除异常                    */

    while (iIntExcReport) {                                             /*  逐一处理异常                */
        iIntExcNum     = __ffs(iIntExcReport);
        iIntExcReport &= ~(1 << iIntExcNum);
        set_iexcept(iIntExcReport);
        archExcProcess(&_G_c6xIntExcTbl[iIntExcNum]);
    }
}
/*********************************************************************************************************
** 函数名称: archExtExcProcess
** 功能描述: 处理一个外部异常 (能被屏蔽)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archExtExcProcess (VOID)
{
    INT  iExtExcNum;

    while ((iExtExcNum = bspExtExcGet()) >= 0) {                        /*  BSP 查询异常号              */
        archExcProcess(&_G_c6xExtExcTbl[iExtExcNum]);
    }

    ack_exception(EXCEPT_TYPE_EXC);                                     /*  清除异常                    */
}
/*********************************************************************************************************
** 函数名称: archExcHandle
** 功能描述: 异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archExcHandle (VOID)
{
    UINT  uiExcType;
    UINT  uiTypeNum;
    UINT  uiIntExcNum = 9;                                              /*  缺省为未知异常              */

    while ((uiExcType = get_except_type()) != 0) {                      /*  异常类型                    */
        uiTypeNum = fls(uiExcType) - 1;                                 /*  获得异常类型号              */

        switch (uiTypeNum) {

        case EXCEPT_TYPE_NXF:                                           /*  NMI 异常                    */
            ack_exception(EXCEPT_TYPE_NXF);                             /*  清除异常                    */
            if (bspNmiExcHandler() < 0) {                               /*  交由 BSP 处理               */
                uiIntExcNum = 10;                                       /*  处理不成功: fatal error     */
                archExcProcess(&_G_c6xIntExcTbl[uiIntExcNum]);
            }
            break;

        case EXCEPT_TYPE_IXF:                                           /*  内部异常                    */
            archIntExcProcess();
            break;

        case EXCEPT_TYPE_EXC:                                           /*  外部异常                    */
            archExtExcProcess();
            break;

        case EXCEPT_TYPE_SXF:
            uiIntExcNum = 8;                                            /*  软件异常                    */

        default:
            ack_exception(uiTypeNum);                                   /*  清除异常                    */
            archExcProcess(&_G_c6xIntExcTbl[uiIntExcNum]);
            break;
        }
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
