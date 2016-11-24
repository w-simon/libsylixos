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
** 文   件   名: ppcExcE500.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 04 日
**
** 描        述: PowerPC E500 体系构架异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "ppcSpr.h"
#include "ppcSprE500.h"
#include "arch/ppc/arch_e500.h"
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/mm/mmu/e500/ppcMmuE500.h"
#endif
/*********************************************************************************************************
** 函数名称: archE500CriticalInputExceptionHandle
** 功能描述: 临界输入异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500CriticalInputExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500MachineCheckExceptionHandle
** 功能描述: 机器检查异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500MachineCheckExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500DataStorageExceptionHandle
** 功能描述: 数据存储异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500DataStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcE500GetDEAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiESR = ppcE500GetESR();

    if (uiESR & ARCH_PPC_ESR_BO) {
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else if (uiESR & ARCH_PPC_ESR_DLK) {
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else if (uiESR & ARCH_PPC_ESR_ST) {
        abtInfo.VMABT_uiType   = ppcE500MmuDataStorageAbortType(ulAbortAddr, LW_TRUE);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;

    } else {
        abtInfo.VMABT_uiType   = ppcE500MmuDataStorageAbortType(ulAbortAddr, LW_FALSE);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archE500InstructionStorageExceptionHandle
** 功能描述: 指令访问异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500InstructionStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ulRetAddr;
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiESR = ppcE500GetESR();
    if (uiESR & ARCH_PPC_ESR_BO) {
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else {
        abtInfo.VMABT_uiType   = ppcE500MmuInstStorageAbortType(ulAbortAddr);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archE500AlignmentExceptionHandle
** 功能描述: 非对齐异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500AlignmentExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr            = ppcE500GetDEAR();
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    abtInfo.VMABT_uiMethod = BUS_ADRALN;

    LW_TCB_GET_CUR(ptcbCur);

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500ProgramExceptionHandle
** 功能描述: 程序异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500ProgramExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

#if LW_CFG_GDB_EN > 0
    UINT    uiBpType = archDbgTrapType(ulRetAddr, (PVOID)LW_NULL);      /*  断点指令探测                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {   /*  进入调试接口断点处理        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500SystemCallHandle
** 功能描述: 系统调用处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500SystemCallHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500DataTLBErrorHandle
** 功能描述: 数据访问 TLB 错误异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

VOID  archE500DataTLBErrorHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    ulAbortAddr = ppcE500GetDEAR();

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}

#endif                                                                 /*  LW_CFG_VMM_EN > 0            */
/*********************************************************************************************************
** 函数名称: archE500InstructionTLBErrorHandle
** 功能描述: 指令访问 TLB 错误异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

VOID  archE500InstructionTLBErrorHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}

#endif                                                                 /*  LW_CFG_VMM_EN > 0            */
/*********************************************************************************************************
** 函数名称: archE500DebugInterruptHandle
** 功能描述: 调试中断处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500DebugInterruptHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500FpuUnavailableExceptionHandle
** 功能描述: FPU 不可用异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500FpuUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = 0;                                         /*  FPU 不可用                  */
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptAck
** 功能描述: Decrementer 中断 ACK
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_DIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptHandle
** 功能描述: Decrementer 中断处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
__attribute__((weak)) VOID  archE500DecrementerInterruptHandle (VOID)
{
    archE500DecrementerInterruptAck();
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptEnable
** 功能描述: Decrementer 中断使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_DIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptDisable
** 功能描述: Decrementer 中断禁能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_DIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerAutoReloadEnable
** 功能描述: Decrementer 中断使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerAutoReloadEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_ARE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerAutoReloadDisable
** 功能描述: Decrementer 中断禁能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerAutoReloadDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_ARE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptAck
** 功能描述: 固定间隔定时器中断 ACK
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500TimerInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_FIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptHandle
** 功能描述: 固定间隔定时器中断处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
__attribute__((weak)) VOID  archE500TimerInterruptHandle (VOID)
{
    archE500TimerInterruptAck();
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptEnable
** 功能描述: 固定间隔定时器中断使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500TimerInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_FIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptDisable
** 功能描述: 固定间隔定时器中断禁能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500TimerInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_FIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptAck
** 功能描述: 看门狗定时器中断 ACK
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_WIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptHandle
** 功能描述: 看门狗定时器中断处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
__attribute__((weak)) VOID  archE500WatchdogInterruptHandle (VOID)
{
    archE500WatchdogInterruptAck();
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptEnable
** 功能描述: 看门狗定时器中断使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_WIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptDisable
** 功能描述: 看门狗定时器中断禁能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_WIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
