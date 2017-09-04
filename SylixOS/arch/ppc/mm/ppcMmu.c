/*********************************************************************************************************
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
** 文   件   名: ppcMmu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: PowerPC 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "mmu/common/ppcMmu.h"
#include "mmu/e500/ppcMmuE500.h"
/*********************************************************************************************************
** 函数名称: archCacheInit
** 功能描述: 初始化 CACHE 
** 输　入  : uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archMmuInit (CPCHAR  pcMachineName)
{
    LW_MMU_OP *pmmuop = API_VmmGetLibBlock();

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_603)     == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_EC603)   == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_604)     == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_750)     == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_MPC83XX) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_745X)    == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E300)    == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E600)    == 0)) {
        ppcMmuInit(pmmuop, pcMachineName);

    } else if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500)   == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E500V1) == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        ppcE500MmuInit(pmmuop, pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
