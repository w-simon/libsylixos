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
** 文   件   名: SemaphoreDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 02 日
**
** 描        述: 各类型信号量兼容等待函数. 为了方便移植其他操作系统应用软件而编写.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreDelete
** 功能描述: 删除信号量
** 输　入  : 
**           ulId            事件句柄
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_SEM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API 
ULONG  API_SemaphoreDelete (LW_OBJECT_HANDLE  *pulId)
{
    REGISTER ULONG      ulObjectClass;
    REGISTER ULONG      ulErrorCode;
    
    ulObjectClass = _ObjectGetClass(*pulId);                            /*  获得信号量句柄的类型        */
    
    switch (ulObjectClass) {
    
#if LW_CFG_SEMB_EN > 0
    case _OBJECT_SEM_B:
        ulErrorCode = API_SemaphoreBDelete(pulId);
        break;
#endif                                                                  /*  LW_CFG_SEMB_EN > 0          */

#if LW_CFG_SEMC_EN > 0
    case _OBJECT_SEM_C:
        ulErrorCode = API_SemaphoreCDelete(pulId);
        break;
#endif                                                                  /*  LW_CFG_SEMC_EN > 0          */

#if LW_CFG_SEMM_EN > 0
    case _OBJECT_SEM_M:
        ulErrorCode = API_SemaphoreMDelete(pulId);
        break;
#endif                                                                  /*  LW_CFG_SEMM_EN > 0          */

#if LW_CFG_SEMRW_EN > 0
    case _OBJECT_SEM_RW:
        ulErrorCode = API_SemaphoreRWDelete(pulId);
        break;
#endif                                                                  /*  LW_CFG_SEMRW_EN > 0         */
    
    default:
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  句柄类型错误                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    return  (ulErrorCode);
}

#endif                                                                  /*  (LW_CFG_SEM_EN > 0) &&      */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
