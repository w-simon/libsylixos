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
** 文   件   名: vutexLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2020 年 12 月 26 日
**
** 描        述: 等待变量锁内部实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define LW_VUTEX_ALIGN_LOG      3
#define LW_VUTEX_HASH_SIZE      1024
#define LW_VUTEX_HASH_MASK      0x3ff
#define LW_VUTEX_HASH_INDEX(a)  (((a) >> LW_VUTEX_ALIGN_LOG) & LW_VUTEX_HASH_MASK)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER   _k_plineVutexHashHeader[LW_VUTEX_HASH_SIZE];
/*********************************************************************************************************
** 函数名称: _VutexWaitQueue
** 功能描述: 将当前任务加入等待队列
** 输　入  : ptcbCur   当前任务
**           phyaddr   等待的物理地址
**           iExpect   期望的值
**           iFlags    操作选项
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _VutexWaitQueue (PLW_CLASS_TCB  ptcbCur, phys_addr_t  phyaddr, INT32  iExpect, INT  iFlags)
{
    REGISTER PLW_CLASS_PCB         ppcb;
             LW_LIST_LINE_HEADER  *pplineHeader;
             PLW_VUTEX_CONTEXT     pvutex = &ptcbCur->TCB_vutex;

    pvutex->VUTEX_phyaddrVutex = phyaddr;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪队列中删除            */

    pvutex->VUTEX_iFlags       = iFlags;
    pvutex->VUTEX_iVutexExpect = iExpect;
    pvutex->VUTEX_uiVutexHash  = LW_VUTEX_HASH_INDEX(phyaddr);
    pplineHeader = &_k_plineVutexHashHeader[pvutex->VUTEX_uiVutexHash];
    _List_Line_Add_Ahead(&pvutex->VUTEX_lineVutex, pplineHeader);       /*  加入等待队列                */

    if (ptcbCur->TCB_ulDelay) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  加入等待扫描链              */
    }
}
/*********************************************************************************************************
** 函数名称: _VutexWaitQueue
** 功能描述: 根据地址和期望数据唤醒目标任务
** 输　入  : ptcbCur   当前任务
**           phyaddr   物理地址
**           iValue    写入的值
**           iFlags    操作选项
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _VutexWakeQueue (PLW_CLASS_TCB  ptcbCur, phys_addr_t  phyaddr, INT32  iValue, INT  iFlags)
{
    UINT32                uiHash = LW_VUTEX_HASH_INDEX(phyaddr);
    BOOL                  bMatch, bGlobal = iFlags & LW_OPTION_VUTEX_GLOBAL;
    PLW_CLASS_TCB         ptcb;
    PLW_CLASS_PCB         ppcb;
    PLW_VUTEX_CONTEXT     pvutex;
    LW_LIST_LINE_HEADER  *pplineHeader;
    PLW_LIST_LINE         plineTemp, plineNext;

    pplineHeader = &_k_plineVutexHashHeader[uiHash];
    plineTemp    = *pplineHeader;

    while (plineTemp) {
        plineNext = _list_line_get_next(plineTemp);

        pvutex = _LIST_ENTRY(plineTemp, LW_VUTEX_CONTEXT, VUTEX_lineVutex);
        bMatch = LW_FALSE;

        if (bGlobal && (pvutex->VUTEX_iFlags & LW_OPTION_VUTEX_GLOBAL)) {
            if ((pvutex->VUTEX_phyaddrVutex == phyaddr) &&
                (pvutex->VUTEX_iVutexExpect == iValue || iValue == __ARCH_INT_MAX)) {
                ptcb   = _LIST_ENTRY(pvutex, LW_CLASS_TCB, TCB_vutex);
                bMatch = LW_TRUE;                                       /*  全局匹配                    */
            }

        } else if (!(pvutex->VUTEX_iFlags & LW_OPTION_VUTEX_GLOBAL)) {
            if ((pvutex->VUTEX_phyaddrVutex == phyaddr) &&
                (pvutex->VUTEX_iVutexExpect == iValue || iValue == __ARCH_INT_MAX)) {
                ptcb = _LIST_ENTRY(pvutex, LW_CLASS_TCB, TCB_vutex);
                if (ptcbCur->TCB_pvVProcessContext == ptcb->TCB_pvVProcessContext) {
                    bMatch = LW_TRUE;                                   /*  进程内匹配                  */
                }
            }
        }

        if (bMatch) {
            _List_Line_Del(&pvutex->VUTEX_lineVutex, pplineHeader);
            if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
                __DEL_FROM_WAKEUP_LINE(ptcb);                           /*  退出等待队列                */
                ptcb->TCB_ulDelay = 0ul;
            }

            if (ptcb->TCB_ucWaitTimeout) {
                ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;           /*  清除超时位                  */
            } else {
                ptcb->TCB_usStatus = (UINT16)(ptcb->TCB_usStatus & ~LW_THREAD_STATUS_VUTEX);
                if (__LW_THREAD_IS_READY(ptcb)) {                       /*  是否就绪                    */
                    ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;     /*  调度激活方式                */
                    ppcb = _GetPcb(ptcb);
                    __ADD_TO_READY_RING(ptcb, ppcb);                    /*  加入到相对优先级就绪环      */
                }
            }
        }

        plineTemp = plineNext;
    }
}
/*********************************************************************************************************
** 函数名称: _VutexInitCtx
** 功能描述: 初始化任务控制块 vutex 上下文
** 输　入  : ptcb      任务
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _VutexInitCtx (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT  pvutex = &ptcb->TCB_vutex;

    pvutex->VUTEX_phyaddrVutex = LW_PHY_ADDR_INVALID;
    _LIST_LINE_INIT_IN_CODE(pvutex->VUTEX_lineVutex);
}
/*********************************************************************************************************
** 函数名称: _VutexUnQueue
** 功能描述: 将目标任务退出等待队列
** 输　入  : ptcb      目标任务
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _VutexUnQueue (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT     pvutex = &ptcb->TCB_vutex;
    LW_LIST_LINE_HEADER  *pplineHeader;

    pplineHeader = &_k_plineVutexHashHeader[pvutex->VUTEX_uiVutexHash];
    _List_Line_Del(&pvutex->VUTEX_lineVutex, pplineHeader);

    pvutex->VUTEX_uiVutexHash  = 0;
    pvutex->VUTEX_phyaddrVutex = LW_PHY_ADDR_INVALID;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
