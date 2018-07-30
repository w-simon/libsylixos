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
** 文   件   名: cpu_cfg_riscv.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 03 月 20 日
**
** 描        述: RISC-V CPU 类型与功能配置.
*********************************************************************************************************/

#ifndef __CPU_CFG_RISCV_H
#define __CPU_CFG_RISCV_H

/*********************************************************************************************************
  CPU 体系结构
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_RISCV           1                               /*  CPU 架构                    */
#define LW_CFG_CPU_ARCH_FAMILY          "RISC-V(R)"                     /*  RISC-V family               */

/*********************************************************************************************************
  SMT 同步多线程调度优化
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_SMT             0                               /*  同步多线程优化              */

/*********************************************************************************************************
  CACHE LINE 对齐
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_CACHE_LINE      64                              /*  cache 最大行对齐属性        */

/*********************************************************************************************************
  CPU 体系结构配置
*********************************************************************************************************/

/*********************************************************************************************************
  CPU 字长与整型大小端定义
*********************************************************************************************************/

#define LW_CFG_CPU_ENDIAN               0                               /*  0: 小端  1: 大端            */
#define LW_CFG_CPU_WORD_LENGHT          __riscv_xlen                    /*  CPU 字长                    */

/*********************************************************************************************************
  RISC-V 配置
*********************************************************************************************************/

#define LW_CFG_RISCV_M_LEVEL            0                               /*  是否使用 Machine level      */

#if LW_CFG_CPU_WORD_LENGHT == 64
#define LW_CFG_RISCV_MMU_SV39           1                               /*  1: SV39 MMU, 0: SV48 MMU    */
#endif

#define LW_CFG_RISCV_GOT_SIZE           (8 * LW_CFG_KB_SIZE)            /*  内核模块 GOT 大小           */
#define LW_CFG_RISCV_HI20_SIZE          (8 * LW_CFG_KB_SIZE)            /*  内核模块 HI20 信息大小      */

#define LW_CFG_RISCV_MPU_EN             0                               /*  CPU 是否拥有 MPU            */

/*********************************************************************************************************
  浮点运算单元
*********************************************************************************************************/

#define LW_CFG_CPU_FPU_EN               1                               /*  CPU 是否拥有 FPU            */

/*********************************************************************************************************
  DSP 数字信号处理器
*********************************************************************************************************/

#define LW_CFG_CPU_DSP_EN               0                               /*  CPU 是否拥有 DSP            */

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/

#ifdef __riscv_atomic
#define LW_CFG_CPU_ATOMIC_EN            1
#else
#define LW_CFG_CPU_ATOMIC_EN            0
#endif                                                                  /*  defined(__riscv_atomic)     */

#endif                                                                  /*  __CPU_CFG_RISCV_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
