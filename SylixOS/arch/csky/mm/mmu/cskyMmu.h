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
** 文   件   名: cskyMmuCommon.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 14 日
**
** 描        述: C-SKY 体系构架 MMU 通用接口.
*********************************************************************************************************/

#ifndef __ARCH_CSKYMMU_H
#define __ARCH_CSKYMMU_H

VOID   cskyMmuInit(LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName);
ULONG  cskyMmuTlbLdStExceptHandle(VOID);

#endif                                                                  /*  __ARCH_CSKYMMU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
