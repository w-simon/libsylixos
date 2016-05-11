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
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/mm/mmu/common/ppcMmu.h"
#endif
#include "arch/ppc/arch_e500.h"
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
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

    {
        /*
         * TODO
         */
    }

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

    {
        /*
         * TODO
         */
    }

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

    ulAbortAddr          = ppcE500GetDEAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BUS;

    LW_TCB_GET_CUR(ptcbCur);

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
