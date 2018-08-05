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
** 文   件   名: armParam.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 04 月 27 日
**
** 描        述: ARM 体系构架启动参数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "armParam.h"
/*********************************************************************************************************
  启动参数
*********************************************************************************************************/
static ARM_PARAM    armParam = { LW_TRUE, LW_FALSE };
/*********************************************************************************************************
** 函数名称: archKernelParam
** 功能描述: ARM 体系构架启动参数设置.
** 输　入  : pcParam       启动参数
**                         unalign=yes      是否支持非对齐访问
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archKernelParam (CPCHAR  pcParam)
{
    if (lib_strncmp(pcParam, "unalign=", 8) == 0) {                     /*  是否支持非对齐访问          */
        if (pcParam[8] == 'n') {
            armParam.AP_bUnalign = LW_FALSE;
        } else {
            armParam.AP_bUnalign = LW_TRUE;
        }
    
#if LW_CFG_SMP_EN > 0
    } else if (lib_strncmp(pcParam, "sldepcache=", 11) == 0) {
        if (pcParam[8] == 'n') {
            armParam.AP_bSLDepCache = LW_FALSE;
        } else {
            armParam.AP_bSLDepCache = LW_TRUE;
            __ARCH_SPIN_BYPASS();
        }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
}
/*********************************************************************************************************
** 函数名称: archKernelParamGet
** 功能描述: 获取启动参数.
** 输　入  : NONE
** 输　出  : 参数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ARM_PARAM  *archKernelParamGet (VOID)
{
    return  (&armParam);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
