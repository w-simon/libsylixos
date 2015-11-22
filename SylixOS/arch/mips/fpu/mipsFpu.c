/**********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: mipsFpu.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 11 月 17 日
**
** 描        述: MIPS 体系架构硬件浮点运算器 (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "mipsFpu.h"
#include "fpu32/mipsVfp32.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit;
static PMIPS_FPU_OP     _G_pfpuop;
/*********************************************************************************************************
** 函数名称: archFpuPrimaryInit
** 功能描述: 主核 Fpu 控制器初始化
** 输　入  : pcMachineName 机器名称
**           pcFpuName     fpu 名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpuPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s FPU pri-core initialization.\r\n",
                 pcMachineName, pcFpuName);

    if (lib_strcmp(pcFpuName, MIPS_FPU_VFP32) == 0) {                   /*  选择 VFP 架构               */
        _G_pfpuop = mipsVfp32PrimaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }

    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));

    MIPS_VFP_ENABLE(_G_pfpuop);

    MIPS_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);

    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpcsr = 0x00000000;     /*  Set FZ bit in VFP           */

    MIPS_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** 函数名称: archFpuSecondaryInit
** 功能描述: 从核 Fpu 控制器初始化
** 输　入  : pcMachineName 机器名称
**           pcFpuName     fpu 名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  archFpuSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s FPU sec-core initialization.\r\n",
                 pcMachineName, pcFpuName);

    if (lib_strcmp(pcFpuName, MIPS_FPU_VFP32) == 0) {                   /*  选择 VFP 架构               */
        mipsVfp32SecondaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }

    return;
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: OSFpuInitCtx
** 功能描述: 初始化一个 Fpu 上下文控制块 (这里并没有使能 FPU)
** 输　入  : pvFpuCtx   FPU 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpuCtxInit (PVOID pvFpuCtx)
{
    lib_memcpy(pvFpuCtx, &_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
}
/*********************************************************************************************************
** 函数名称: archFpuEnable
** 功能描述: 使能 FPU.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpuEnable (VOID)
{
    MIPS_VFP_ENABLE(_G_pfpuop);
}
/*********************************************************************************************************
** 函数名称: archFpuDisable
** 功能描述: 禁能 FPU.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpuDisable (VOID)
{
    MIPS_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** 函数名称: archFpuSave
** 功能描述: 保存 FPU 上下文.
** 输　入  : pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpuSave (PVOID pvFpuCtx)
{
    MIPS_VFP_SAVE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuRestore
** 功能描述: 回复 FPU 上下文.
** 输　入  : pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpuRestore (PVOID pvFpuCtx)
{
    MIPS_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuCtxShow
** 功能描述: 显示 FPU 上下文.
** 输　入  : iFd       文件描述符
**           pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpuCtxShow (INT  iFd, PVOID pvFpuCtx)
{
    MIPS_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuUndHandle
** 功能描述: 系统发生 undef 异常时, 调用此函数.
**           只有某个任务或者中断, 真正使用浮点运算时 (即运行到浮点运算指令产生异常)
**           这时才可以打开浮点运算库.
** 输　入  : ptcbCur   当前任务 TCB
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archFpuUndHandle (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_REG_T     regSp;

    if (MIPS_VFP_ISENABLE(_G_pfpuop)) {                                 /*  如果当前上下文 FPU 使能     */
        return  (PX_ERROR);                                             /*  此未定义指令与 FPU 无关     */
    }

    pregctx = archTaskRegsGet(ptcbCur->TCB_pstkStackNow, &regSp);
    pregctx->REG_uiCP0Status |= M_StatusCU1;

    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;
    MIPS_VFP_ENABLE(_G_pfpuop);                                         /*  使能 FPU                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
