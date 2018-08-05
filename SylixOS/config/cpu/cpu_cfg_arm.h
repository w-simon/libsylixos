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
** 文   件   名: cpu_cfg_arm.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 09 月 29 日
**
** 描        述: ARM CPU 类型与功能配置.
*********************************************************************************************************/

#ifndef __CPU_CFG_ARM_H
#define __CPU_CFG_ARM_H

/*********************************************************************************************************
  CPU 体系结构
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_ARM             1                               /*  CPU 架构                    */
#define LW_CFG_CPU_ARCH_FAMILY          "ARM(R)"                        /*  ARM family                  */

/*********************************************************************************************************
  SMT 同步多线程调度优化
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_SMT             0                               /*  同步多线程优化              */

/*********************************************************************************************************
  CACHE LINE 对齐
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_CACHE_LINE      64                              /*  cache 最大行对齐属性        */

/*********************************************************************************************************
  CPU 字长与整型大小端定义
*********************************************************************************************************/

#if defined(__GNUC__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LW_CFG_CPU_ENDIAN               0                               /*  0: 小端                     */
#else 
#define LW_CFG_CPU_ENDIAN               1                               /*  1: 大端                     */
#endif                                                                  /*  __BYTE_ORDER__ == LITTLE    */
#else
#define LW_CFG_CPU_ENDIAN               0                               /*  0: 小端  1: 大端            */
#endif                                                                  /*  defined(__GNUC__)           */

#define LW_CFG_CPU_WORD_LENGHT          32                              /*  CPU 字长                    */

/*********************************************************************************************************
  ARM 配置
*********************************************************************************************************/

#define LW_CFG_ARM_CP15                 1                               /*  ARM9 以上必须为 1           */
#define LW_CFG_ARM_PL330                1                               /*  是否允许 PL330 DMA 驱动     */
#define LW_CFG_ARM_CACHE_L2             1                               /*  是否允许管理 ARM 二级 CACHE */
#define LW_CFG_ARM_CACHE_L2_ECC         1                               /*  是否打开 ARM 二级 CACHE ECC */

#if defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#define LW_CFG_ARM_MPU                  1
#else
#define LW_CFG_ARM_MPU                  0
#endif                                                                  /*  Cortex-R, M                 */

/*********************************************************************************************************
  Cortex-M 特殊配置
*********************************************************************************************************/

#define LW_CFG_CORTEX_M_INT_MASK        (2 << 4)                        /*  关中断屏蔽的优先级          */
#define LW_CFG_CORTEX_M_FAULT_REBOOT    1                               /*  关键性异常重启              */

/*********************************************************************************************************
  浮点运算单元
*********************************************************************************************************/

#define LW_CFG_CPU_FPU_EN               1                               /*  CPU 是否拥有 FPU            */
#define LW_CFG_CPU_FPU_NONSEC_EN        0                               /*  非安全模式访问 CP10 CP11    */

/*********************************************************************************************************
  DSP 数字信号处理器
*********************************************************************************************************/

#define LW_CFG_CPU_DSP_EN               0                               /*  CPU 是否拥有 DSP            */

#endif                                                                  /*  __CPU_CFG_ARM_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
