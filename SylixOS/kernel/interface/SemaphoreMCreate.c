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
** 文   件   名: SemaphoreMCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 21 日
**
** 描        述: 互斥信号量建立               

** BUG
2007.07.21  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2009.04.08  加入对 SMP 多核的支持.
2009.07.28  自旋锁的初始化放在初始化所有的控制块中, 这里去除相关操作.
2011.07.29  加入对象创建/销毁回调.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  注意:
       MUTEX 不同于 COUNTING, BINERY, 一个线程必须成对使用.
       
       API_SemaphoreMPend();
       ... (do something as fast as possible...)
       API_SemaphoreMPost();
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_SemaphoreMCreate
** 功能描述: 互斥信号量建立
** 输　入  : 
**           pcName             事件名缓冲区
**           ucCeilingPriority  如果使用天花板优先级机制，此参数为天花板优先级
**           ulOption           事件选项        推荐使用 PRIORITY 队列和 DELELTE SAFE 选项
**           pulId              事件ID指针
** 输　出  : 事件句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
LW_OBJECT_HANDLE  API_SemaphoreMCreate (CPCHAR             pcName,
                                        UINT8              ucCeilingPriority,
                                        ULONG              ulOption,
                                        LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER ULONG                 ulI;
    REGISTER PLW_CLASS_WAITQUEUE   pwqTemp;
    REGISTER ULONG                 ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

#if LW_CFG_ARG_CHK_EN > 0
    if (_PriorityCheck(ucCeilingPriority)) {                            /*  优先级错误                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "priority invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

    __KERNEL_MODE_PROC(
        pevent = _Allocate_Event_Object();                              /*  获得一个事件控制块          */
    );
    
    if (!pevent) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a semaphore.\r\n");
        _ErrorHandle(ERROR_EVENT_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(pevent->EVENT_cEventName, pcName);
    } else {
        pevent->EVENT_cEventName[0] = PX_EOS;                           /*  清空名字                    */
    }
                                                                        /*  开始初始化                  */
    pevent->EVENT_ucType            = LW_TYPE_EVENT_MUTEX;
    pevent->EVENT_ulCounter         = (ULONG)LW_TRUE;                   /*  初始化为有效                */
    pevent->EVENT_ulMaxCounter      = LW_PRIO_LOWEST;                   /*  被用作保存线程优先级        */
    pevent->EVENT_ucCeilingPriority = ucCeilingPriority;                /*  天花板优先级                */
    pevent->EVENT_ulOption          = ulOption;
    pevent->EVENT_pvPtr             = LW_NULL;
    pevent->EVENT_pvTcbOwn          = LW_NULL;
    
    pwqTemp = &pevent->EVENT_wqWaitList;
    pwqTemp->WAITQUEUE_usWaitNum = 0;                                   /*  没有线程                    */
    
    for (ulI = 0; ulI < __THREAD_PRIORITY_Q_NUM; ulI++) {               /*  初始化等待队列              */
        pwqTemp->WAITQUEUE_wlWaitList.WAITLIST_pringPRIOList[ulI] = LW_NULL;
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_SEM_M, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pevent->EVENT_usIndex);                    /*  构建对象 id                 */

    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_SEMM, MONITOR_EVENT_SEM_CREATE, 
                      ulIdTemp, ucCeilingPriority, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "semaphore \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
