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
** 文   件   名: x86Param.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 04 日
**
** 描        述: x86 体系构架启动参数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86Param.h"
/*********************************************************************************************************
  启动参数
*********************************************************************************************************/
static X86_PARAM    _G_x86Param = { LW_FALSE };
/*********************************************************************************************************
** 函数名称: archKernelParam
** 功能描述: PowerPC 体系构架启动参数设置.
** 输　入  : pcParam       启动参数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archKernelParam (CPCHAR  pcParam)
{
    if (lib_strncmp(pcParam, "ht=", 3) == 0) {
        if (pcParam[3] == 'y') {                                        /*  支持超线程                  */
            _G_x86Param.X86_bHyperThreading = LW_TRUE;

        } else {
            _G_x86Param.X86_bHyperThreading = LW_FALSE;
        }

    } else if (lib_strncmp(pcParam, "video=", 6) == 0) {
        _G_x86Param.X86_bHasVideoParam = LW_TRUE;                       /*  有视频参数                  */
        lib_strncpy(_G_x86Param.X86_cVideoParam,
                    pcParam + 6,
                    sizeof(_G_x86Param.X86_cVideoParam));               /*  记录视频参数                */

    } else if (lib_strncmp(pcParam, "utc=", 4) == 0) {
        if (pcParam[4] == 'y') {                                        /*  RTC 使用 UTC                */
            _G_x86Param.X86_bRtcUtc = LW_TRUE;

        } else {
            _G_x86Param.X86_bRtcUtc = LW_FALSE;
        }

    } else if (lib_strncmp(pcParam, "busclk=", 7) == 0) {               /*  系统总线时钟                */
        _G_x86Param.X86_ulSysBusClk = lib_strtoul(pcParam + 7, LW_NULL, 10);

    } else if (lib_strncmp(pcParam, "console=", 8) == 0) {              /*  控制台设备                  */
        _G_x86Param.X86_bHasConsoleDev = LW_TRUE;                       /*  有指定的控制台设备          */
        lib_strncpy(_G_x86Param.X86_cConsoleDev,
                    pcParam + 8,
                    sizeof(_G_x86Param.X86_cConsoleDev));
        *lib_strchr(_G_x86Param.X86_cConsoleDev, ' ') = '\0';
    }

    x86MpInit(_G_x86Param.X86_bHyperThreading);                         /*  初始化 MP 配置              */
}
/*********************************************************************************************************
** 函数名称: archKernelParamGet
** 功能描述: 获取启动参数.
** 输　入  : NONE
** 输　出  : 参数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
X86_PARAM  *archKernelParamGet (VOID)
{
    return  (&_G_x86Param);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
