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
** 文   件   名: cskyParam.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 11 日
**
** 描        述: C-SKY 体系架构启动参数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "cskyParam.h"
/*********************************************************************************************************
  启动参数
*********************************************************************************************************/
static CSKY_PARAM    cskyParam = { LW_TRUE, LW_TRUE };
/*********************************************************************************************************
** 函数名称: archKernelParam
** 功能描述: C-SKY 体系架构启动参数设置.
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
            cskyParam.CP_bUnalign = LW_FALSE;
        } else {
            cskyParam.CP_bUnalign = LW_TRUE;
        }

    } else if (lib_strncmp(pcParam, "mmuenbyboot=", 12) == 0) {
        if (pcParam[8] == 'n') {
            cskyParam.CP_bMmuEnByBoot = LW_FALSE;
        } else {
            cskyParam.CP_bMmuEnByBoot = LW_TRUE;
        }
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
CSKY_PARAM  *archKernelParamGet (VOID)
{
    return  (&cskyParam);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
