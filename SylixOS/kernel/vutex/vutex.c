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
** 文   件   名: vutex.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2020 年 12 月 26 日
**
** 描        述: 等待变量锁.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "vutexLib.h"
/*********************************************************************************************************
** 函数名称: API_VutexPend
** 功能描述: 等待一个变量到达某个值
** 输　入  : piVar     等待的变量地址
**           iExpect   期望的数值
**           iFlags    操作选项
**           ulTimeout 等待时间
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
INT  API_VutexPend (INT  *piVar, INT  iExpect, INT  iFlags, ULONG  ulTimeout)
{
    phys_addr_t    phyaddr;
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulTimeSave;                                          /*  系统事件记录                */
    INT            iSchedRet;

    if (!piVar) {
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

__wait_again:
    if (*(volatile INT *)piVar == iExpect) {
        return  (ERROR_NONE);

    } else if (ulTimeout == LW_OPTION_NOT_WAIT) {                       /*  不等待                      */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (PX_ERROR);
    }

    if (iFlags & LW_OPTION_VUTEX_GLOBAL) {                              /*  是否为全局地址              */
#if LW_CFG_VMM_EN > 0
        if (vmmVirtualToPhysical((addr_t)piVar, &phyaddr)) {            /*  转换为物理地址              */
            return  (PX_ERROR);
        }
#else                                                                   /*  LW_CFG_VMM_EN > 0           */
        phyaddr = (phys_addr_t)piVar;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
    } else {
        phyaddr = (phys_addr_t)piVar;
    }

    if (phyaddr == LW_PHY_ADDR_INVALID) {                               /*  地址无效                    */
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (*(volatile INT *)piVar == iExpect) {
        __KERNEL_EXIT();
        return  (ERROR_NONE);
    }

    LW_TCB_GET_CUR(ptcbCur);

    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_VUTEX;               /*  写状态位，开始等待          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  清空等待时间                */

    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  是否是无穷等待              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  设置超时时间                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  记录系统时间                */

    _VutexWaitQueue(ptcbCur, phyaddr, iExpect, iFlags);                 /*  加入等待表                  */

    iSchedRet = __KERNEL_EXIT();                                        /*  退出内核                    */
    if (*(volatile INT *)piVar == iExpect) {
        return  (ERROR_NONE);
    }

    if (iSchedRet == LW_SIGNAL_EINTR) {
        _ErrorHandle(EINTR);                                            /*  被信号打断                  */
        return  (PX_ERROR);

    } else if (iSchedRet == LW_SIGNAL_RESTART) {
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);

    } else {
        if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {           /*  被唤醒或超时了              */
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                    /*  等待超时                    */
            return  (PX_ERROR);

        } else {
            ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);
        }
    }

    if (ulTimeout != LW_OPTION_NOT_WAIT) {
        goto    __wait_again;                                           /*  重新等待                    */
    }

    _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                            /*  等待超时                    */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_VutexPost
** 功能描述: 改变一个变量为某个值
** 输　入  : piVar     变量地址
**           iValue    要设置的值
**           iFlags    操作选项
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
INT  API_VutexPost (INT  *piVar, INT  iValue, INT  iFlags)
{
    phys_addr_t    phyaddr;
    PLW_CLASS_TCB  ptcbCur;

    if (!piVar) {
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

    if (*(volatile INT *)piVar == iValue) {
        return  (ERROR_NONE);
    }

    if (iFlags & LW_OPTION_VUTEX_GLOBAL) {                              /*  是否为全局地址              */
#if LW_CFG_VMM_EN > 0
        if (vmmVirtualToPhysical((addr_t)piVar, &phyaddr)) {            /*  转换为物理地址              */
            return  (PX_ERROR);
        }
#else                                                                   /*  LW_CFG_VMM_EN > 0           */
        phyaddr = (phys_addr_t)piVar;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
    } else {
        phyaddr = (phys_addr_t)piVar;
    }

    if (phyaddr == LW_PHY_ADDR_INVALID) {                               /*  地址无效                    */
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (*(volatile INT *)piVar == iValue) {
        __KERNEL_EXIT();
        return  (ERROR_NONE);
    }

    if (iValue != __ARCH_INT_MAX) {                                     /*  __ARCH_INT_MAX 仅唤醒       */
        *piVar = iValue;
        KN_SMP_WMB();
    }

    LW_TCB_GET_CUR(ptcbCur);

    _VutexWakeQueue(ptcbCur, phyaddr, iValue, iFlags);                  /*  唤醒合适的任务              */

    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
