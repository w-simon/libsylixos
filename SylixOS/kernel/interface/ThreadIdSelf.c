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
** 文   件   名: ThreadIdSelf.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 线程获得自己的句柄

** BUG
2007.07.18  加入 _DebugHandle() 功能
2009.10.12  加入提示注释.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2020.09.02  加入 Fast 获取方法.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadIdSelf
** 功能描述: 当前线程 ID
** 输　入  : NONE
** 输　出  : ID
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
LW_OBJECT_HANDLE  API_ThreadIdSelf (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (0);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur->TCB_ulId);
}
/*********************************************************************************************************
** 函数名称: API_ThreadIdSelfFast
** 功能描述: 当前线程 ID
** 输　入  : NONE
** 输　出  : ID
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
LW_OBJECT_HANDLE  API_ThreadIdSelfFast (VOID)
{
#if (defined(LW_CFG_CPU_ARCH_ARM64) && (LW_CFG_ARM64_FAST_TCB_CUR > 0))
    REGISTER PLW_CLASS_TCB   ptcbCur asm("x18");                        /*  x18 saved current tcb       */

    return  (ptcbCur->TCB_ulId);

#elif (defined(LW_CFG_CPU_ARCH_X86) && (LW_CFG_X64_FAST_TCB_CUR > 0))
    REGISTER LW_OBJECT_HANDLE  ulId;

    __asm__ __volatile__("movq %%fs:%P1, %q0"
                         : "=r" (ulId)
                         : "i" (offsetof(LW_CLASS_TCB, TCB_ulId)));
    return  (ulId);
#else
    REGISTER PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur->TCB_ulId);
#endif
}
/*********************************************************************************************************
** 函数名称: API_ThreadTcbSelf
** 功能描述: 当前线程 TCB (危险...)
** 输　入  : NONE
** 输　出  : TCB
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
PLW_CLASS_TCB  API_ThreadTcbSelf (VOID)
{
    PLW_CLASS_TCB   ptcbCur;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_NULL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur);
}
/*********************************************************************************************************
** 函数名称: API_ThreadTcbSelfFast
** 功能描述: 当前线程 TCB (危险...)
** 输　入  : NONE
** 输　出  : TCB
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
PLW_CLASS_TCB  API_ThreadTcbSelfFast (VOID)
{
#if (defined(LW_CFG_CPU_ARCH_ARM64) && (LW_CFG_ARM64_FAST_TCB_CUR > 0))
    REGISTER PLW_CLASS_TCB   ptcbCur asm("x18");                        /*  x18 saved current tcb       */

    return  (ptcbCur);

#elif (defined(LW_CFG_CPU_ARCH_X86) && (LW_CFG_X64_FAST_TCB_CUR > 0))
    REGISTER UINT16  usIndex;

    __asm__ __volatile__("movw %%fs:%P1, %q0"
                         : "=r" (usIndex)
                         : "i" (offsetof(LW_CLASS_TCB, TCB_usIndex)));

    return  (_K_ptcbTCBIdTable[usIndex]);
#else
    REGISTER PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur);
#endif
}
/*********************************************************************************************************
** 函数名称: API_ThreadIdInter
** 功能描述: 当前被中断线程 ID, BSP 中断程序使用
** 输　入  : NONE
** 输　出  : ID
** 全局变量: 
** 调用模块: 
** 注  意  : 只能在中断服务函数中使用
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ThreadIdInter (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);

    return  (ptcbCur->TCB_ulId);
}
/*********************************************************************************************************
** 函数名称: API_ThreadTcbInter
** 功能描述: 当前被中断线程 TCB, BSP 中断程序使用
** 输　入  : NONE
** 输　出  : TCB
** 全局变量: 
** 调用模块: 
** 注  意  : 只能在中断服务函数中使用
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_CLASS_TCB  API_ThreadTcbInter (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);

    return  (ptcbCur);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
