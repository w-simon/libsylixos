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
** 文   件   名: mipsVfp64.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 12 月 01 日
**
** 描        述: MIPS 体系架构 VPU64 支持.
*********************************************************************************************************/

#ifndef __MIPSVFP64_H
#define __MIPSVFP64_H

PMIPS_FPU_OP  mipsVfp64PrimaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
VOID          mipsVfp64SecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);

#endif                                                                  /*  __MIPSVFP64_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
