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
** 文   件   名: cpu_cfg_ppc.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 11 月 20 日
**
** 描        述: PowerPC CPU 类型与功能配置.
*********************************************************************************************************/

#ifndef __CPU_CFG_PPC_H
#define __CPU_CFG_PPC_H

/*********************************************************************************************************
  CPU 体系结构
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_PPC             1                               /*  CPU 架构                    */
#define LW_CFG_CPU_ARCH_FAMILY          "PowerPC(R)"                    /*  PowerPC family              */

/*********************************************************************************************************
  整型大小端定义
*********************************************************************************************************/

#define LW_CFG_CPU_ENDIAN               1                               /*  0: 小端  1: 大端            */

/*********************************************************************************************************
  PowerPC 配置
*********************************************************************************************************/

#define LW_CFG_PPC_CACHE_L2             1                               /*  是否允许管理 PPC 二级 CACHE */

/*********************************************************************************************************
  浮点运算单元
*********************************************************************************************************/

#define LW_CFG_CPU_FPU_EN               1                               /*  CPU 是否拥有 FPU            */

#endif                                                                  /*  __CPU_CFG_PPC_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
