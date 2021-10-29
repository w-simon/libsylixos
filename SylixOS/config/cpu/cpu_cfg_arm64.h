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
** 文   件   名: cpu_cfg_arm64.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 05 日
**
** 描        述: ARM64 CPU 类型与功能配置.
*********************************************************************************************************/

#ifndef __CPU_CFG_ARM64_H
#define __CPU_CFG_ARM64_H

/*********************************************************************************************************
  CPU 体系结构
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_ARM64           1                               /*  CPU 架构                    */
#define LW_CFG_CPU_ARCH_FAMILY          "ARM64(R)"                      /*  ARM family                  */

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

#define LW_CFG_CPU_WORD_LENGHT          64                              /*  CPU 字长                    */
#define LW_CFG_CPU_PHYS_ADDR_64BIT      1                               /*  物理地址 64bit 宽度         */

/*********************************************************************************************************
  ARM64 MMU 配置

  LW_CFG_ARM64_PAGE_SHIFT 可配置的值有如下:
        12 :  4K Bytes per page.
        14 : 16K Bytes per page. (部分处理器支持)
        16 : 64K Bytes per page.
*********************************************************************************************************/

#define LW_CFG_ARM64_PAGE_SHIFT         12                              /*  MMU 页面大小                */

/*********************************************************************************************************
  ARM 配置
*********************************************************************************************************/

#define LW_CFG_ARM64_PREFETCH_W         1                               /*  是否使用 PLDW 指令          */
#define LW_CFG_ARM64_CACHE_L2           1                               /*  是否允许管理 ARM 二级 CACHE */
#define LW_CFG_ARM64_CACHE_L2_ECC       1                               /*  是否打开 ARM 二级 CACHE ECC */
#define LW_CFG_ARM64_HW_AFDBM           1                               /*  是否打开硬件 A、D 位更新    */

/*********************************************************************************************************
  ARM 勘误
  ERRATUM_1024718: Update of DBM/AP bits without break before make might result in incorrect update
*********************************************************************************************************/

#define LW_CFG_ARM64_ERRATUM_1024718    1                               /*  DBM/AP 功能勘误             */

/*********************************************************************************************************
  调试相关配置
*********************************************************************************************************/

#define LW_CFG_ARM64_HW_STEP_EN         1                               /*  硬件单步功能                */

/*********************************************************************************************************
  Spinlock LW_ACCESS_ONCE() 需要进行 inner shareable 内存同步 (飞腾 2000 需要)
*********************************************************************************************************/

#define LW_CFG_ARM64_ACCESS_ONCE_RMB    1

/*********************************************************************************************************
  快速获取线程上下文 (使用 x18 Platform 特性, 仅特殊应用会使用默认关闭)
*********************************************************************************************************/

#define LW_CFG_ARM64_FAST_TCB_CUR       0                               /*  快速获取任务上下文          */

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

#define LW_CFG_CPU_ATOMIC_EN            1
#define LW_CFG_CPU_ATOMIC64_EN          1

#endif                                                                  /*  __CPU_CFG_ARM64_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
