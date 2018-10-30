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
** 文   件   名: cskyExc.c
**
** 创   建   人: Hui.Kai (惠凯)
**
** 文件创建日期: 2018 年 05 月 11 日
**
** 描        述: C-SKY 体系架构异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "arch/csky/common/unaligned/cskyUnaligned.h"
#if LW_CFG_VMM_EN > 0
#include "arch/csky/mm/mmu/cskyMmu.h"
#endif
#include "arch/csky/param/cskyParam.h"
/*********************************************************************************************************
  向量使能与禁能锁
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif
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
LW_WEAK VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t irqret;

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
** 函数名称: archAutoIntVectorGet
** 功能描述: 获取自动中断号
** 输　入  : NONE
** 输　出  : 中断号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG  archAutoIntVectorGet (VOID)
{
#define CSKY_AUTO_IRQ_REG      0xbfcb0000
#define CSKY_AUTO_IRQ_MASK     0xff
    return  (read32(CSKY_AUTO_IRQ_REG) & CSKY_AUTO_IRQ_MASK);
}
/*********************************************************************************************************
** 函数名称: archFastAutoIntHandle
** 功能描述: 快速中断
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  archFastAutoIntHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    bspIntHandle(archAutoIntVectorGet());
}
/*********************************************************************************************************
** 函数名称: archAutoIntHandle
** 功能描述: 普通中断
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archAutoIntHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    bspIntHandle(archAutoIntVectorGet());
}
/*********************************************************************************************************
** 函数名称: archTlbLoadExceptHandle
** 功能描述: TLB load or ifetch 异常处理
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTlbLoadExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = cskyMmuTlbLdStExceptHandle();
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulMEH, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archTlbStoreExceptHandle
** 功能描述: TLB store 异常处理
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTlbStoreExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = cskyMmuTlbLdStExceptHandle();
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulMEH, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archTlbModExceptHandle
** 功能描述: TLB modified 异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTlbModExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulMEH, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTrapExceptHandle
** 功能描述: 跟踪异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrapExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulMEH, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTrap0ExceptHandle
** 功能描述: 陷阱指令异常 0
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrap0ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** 函数名称: archTrap1ExceptHandle
** 功能描述: 陷阱指令异常 1
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrap1ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** 函数名称: archTrap2ExceptHandle
** 功能描述: 陷阱指令异常 2
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrap2ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** 函数名称: archTrap3ExceptHandle
** 功能描述: 陷阱指令异常 3
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrap3ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** 函数名称: archTlbFatalExceptHandle
** 功能描述: TLB 不可恢复异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTlbFatalExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulMEH, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTlbRefillExceptHandle
** 功能描述: TLB 重填异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

VOID  archTlbRefillExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulMEH, &abtInfo, ptcbCur);
}

#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
/*********************************************************************************************************
** 函数名称: archReservedExceptHandle
** 功能描述: Reserved 中断
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archReservedExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulPC, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archIdlyExceptHandle
** 功能描述: Idly 异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archIdlyExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulPC, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archFatalErrExceptHandle
** 功能描述: 不可恢复错误异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFatalErrExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulPC, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archBreakPointExceptHandle
** 功能描述: 断点异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archBreakPointExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_GDB_EN > 0
    UINT           uiBpType;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_GDB_EN > 0
    uiBpType = archDbgTrapType(pregctx->REG_ulPC, LW_NULL);             /*  断点指令探测                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(pregctx->REG_ulPC, uiBpType) == ERROR_NONE) {
            return;                                                     /*  进入调试接口断点处理        */
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulPC, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archPrivilegeExceptHandle
** 功能描述: 特权违反异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archPrivilegeExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulPC, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archIllegalInstExceptHandle
** 功能描述: 非法指令异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archIllegalInstExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulPC, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archDivideZeroExceptHandle
** 功能描述: 除以零异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDivideZeroExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulPC, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archAccessExceptHandle
** 功能描述: 访问错误异常
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archAccessExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulMEH, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archUnalignedExceptHandle
** 功能描述: 未对齐访问异常处理
** 输　入  : ulVector  中断向量
**           pregctx   上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archUnalignedExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    CSKY_PARAM    *param;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    param = archKernelParamGet();
    if (param->CP_bUnalign) {
        abtInfo.VMABT_uiType = cskyUnalignedHandle(pregctx, pregctx->REG_ulPC);

    } else {
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BUS;
    }

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_ulPC, pregctx->REG_ulPC, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
