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
** 文   件   名: cpu_cfg_csky.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 07 日
**
** 描        述: C-SKY CPU 类型与功能配置.
*********************************************************************************************************/

#ifndef __CPU_CFG_CSKY_H
#define __CPU_CFG_CSKY_H

/*********************************************************************************************************
  CPU 体系结构
*********************************************************************************************************/

#define LW_CFG_CPU_ARCH_CSKY            1                               /*  CPU 架构                    */
#define LW_CFG_CPU_ARCH_FAMILY          "C-SKY(R)"                      /*  C-SKY family                 */

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
#define LW_CFG_CPU_PHYS_ADDR_64BIT      0                               /*  物理地址 64bit 宽度         */

/*********************************************************************************************************
  C-SKY MMU 配置

  LW_CFG_CSKY_PAGE_SHIFT 可配置的值有如下:
        12 :  4K Bytes per page.
        14 : 16K Bytes per page.
        16 : 64K Bytes per page.

  LW_CFG_CSKY_PAGE_SHIFT == 12     LW_CFG_CSKY_HARD_TLB_REFILL 推荐为 1
  LW_CFG_CSKY_PAGE_SHIFT == 14/16  LW_CFG_CSKY_HARD_TLB_REFILL 必须为 0
*********************************************************************************************************/

#define LW_CFG_CSKY_PAGE_SHIFT          12                              /*  MMU 页面大小                */
#define LW_CFG_CSKY_HARD_TLB_REFILL     1                               /*  MMU 是否使用硬件重填        */

/*********************************************************************************************************
  C-SKY CACHE 配置
*********************************************************************************************************/

#define LW_CFG_CSKY_CACHE_L2            0                               /*  是否允许管理 C-SKY 二级CACHE*/
#define LW_CFG_CSKY_CACHE_L3            0                               /*  是否允许管理 C-SKY 三级CACHE*/
#define LW_CFG_CSKY_CACHE_V2            0                               /*  是否为 CACHEv2              */

/*********************************************************************************************************
  C-SKY 指令配置
*********************************************************************************************************/

#define LW_CFG_CSKY_HAS_LDSTEX_INSTR    0                               /*  是否支持 LDSTEX 指令        */

/*********************************************************************************************************
  浮点运算单元
*********************************************************************************************************/

#define LW_CFG_CPU_FPU_EN               1                               /*  CPU 是否拥有 FPU            */
#define LW_CFG_CPU_FPU_IDE              0                               /*  FPU 是否使能非规格化数异常  */
#define LW_CFG_CPU_FPU_IXE              0                               /*  FPU 是否使能非精确异常      */
#define LW_CFG_CPU_FPU_UFE              0                               /*  FPU 是否使能下溢异常        */
#define LW_CFG_CPU_FPU_OFE              0                               /*  FPU 是否使能上溢异常        */
#define LW_CFG_CPU_FPU_DZE              0                               /*  FPU 是否使能除 0 异常       */
#define LW_CFG_CPU_FPU_IOE              0                               /*  FPU 是否使能非法操作异常    */

/*********************************************************************************************************
  DSP 数字信号处理器
*********************************************************************************************************/

#define LW_CFG_CPU_DSP_EN               0                               /*  CPU 是否拥有 DSP            */

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/

#if LW_CFG_CSKY_HAS_LDSTEX_INSTR > 0
#define LW_CFG_CPU_ATOMIC_EN            1
#else
#define LW_CFG_CPU_ATOMIC_EN            0
#endif

#define LW_CFG_CPU_ATOMIC64_EN          0

#endif                                                                  /*  __CPU_CFG_CSKY_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
