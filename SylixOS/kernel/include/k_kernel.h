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
** 文   件   名: k_kernel.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 12 日
**
** 描        述: 这是系统综合头文件库。(特别感谢 Google 代码搜索)

** BUG
2007.06.03  将 k_routine.h 提前到 BSP 之前.
2007.11.04  加入 k_ptype.h 文件.
2007.11.04  加入 k_perror.h 文件.
2007.11.21  修改注释, 加入 k_functype.h 文件.
2008.01.24  加入 LOGO 图像.
2012.03.19  加入 __SYLIXOS_RELSTR 加入 GEMINI 表示 SylixOS 进入[双子星座], 即针对 SMP 的支持的实际开发.
2012.12.07  加入资源管理器.
2013.05.04  使用新的体系结构移植头文件组.
2013.08.27  加入内核事件监控器.
2015.11.20  修改项目代号为: NeZha(哪吒).
2016.10.08  修改项目代号为: LongYuan(龙渊).
2018.01.18  修改项目代号为: Octopus(八爪鱼).
2019.06.19  修改项目代号为: Tangram(七巧板).
2020.09.02  升级版本到 2.0.
            修改项目代号为: Enterprise(进取).
*********************************************************************************************************/

#ifndef __K_KERNEL_H
#define __K_KERNEL_H

/*********************************************************************************************************
  编译标志
*********************************************************************************************************/

#ifndef SYLIXOS
#define SYLIXOS
#endif                                                                  /*  SYLIXOS                     */

/*********************************************************************************************************
  版本号
*********************************************************************************************************/

#define __SYLIXOS_MAJOR_VER     2
#define __SYLIXOS_MINOR_VER     0
#define __SYLIXOS_PATCH_VER     4
#define __SYLIXOS_PATCH_PAD     0

/*********************************************************************************************************
  版本格式
*********************************************************************************************************/

#define __SYLIXOS_STRXCHG(x)        #x
#define __SYLIXOS_MAKEVER(a, b, c)  (((a) << 16) + ((b) << 8) + (c))

#if __SYLIXOS_PATCH_PAD > 0
#define __SYLIXOS_MAKEVERSTR(a, b, c, d)    \
        __SYLIXOS_STRXCHG(a) "." __SYLIXOS_STRXCHG(b) "." __SYLIXOS_STRXCHG(c) "-" __SYLIXOS_STRXCHG(d)
#else
#define __SYLIXOS_MAKEVERSTR(a, b, c, d)    \
        __SYLIXOS_STRXCHG(a) "." __SYLIXOS_STRXCHG(b) "." __SYLIXOS_STRXCHG(c)
#endif                                                                  /*  __SYLIXOS_PATCH_PAD         */

/*********************************************************************************************************
  版本信息
*********************************************************************************************************/

#define __SYLIXOS_VERSION       __SYLIXOS_MAKEVER(__SYLIXOS_MAJOR_VER, \
                                                  __SYLIXOS_MINOR_VER, \
                                                  __SYLIXOS_PATCH_VER)
#define __SYLIXOS_VERSTR        __SYLIXOS_MAKEVERSTR(__SYLIXOS_MAJOR_VER, \
                                                     __SYLIXOS_MINOR_VER, \
                                                     __SYLIXOS_PATCH_VER, \
                                                     __SYLIXOS_PATCH_PAD)

#define __SYLIXOS_RELSTR        "Enterprise"
#define __SYLIXOS_VERINFO       "SylixOS kernel version: "  \
                                __SYLIXOS_VERSTR            \
                                " Code name: " __SYLIXOS_RELSTR
#define __SYLIXOS_LICENSE       "SylixOS license: Commercial & GPL."

/*********************************************************************************************************
  版本匹配弱符号 (__sylixos_version 版本信息将保留在使用此版本头文件编译的库中)
*********************************************************************************************************/

#ifdef  __GNUC__
__attribute__((weak)) char      __sylixos_version[] = __SYLIXOS_VERSTR;
#endif                                                                  /*  __GNUC__                    */

/*********************************************************************************************************
  系统 LOGO
*********************************************************************************************************/

#include "k_logo.h"

/*********************************************************************************************************
  系统配置与裁减头文件
*********************************************************************************************************/
#include "../SylixOS/config/cpu/cpu_cfg.h"                              /*  CPU 功能配置                */
#include "../SylixOS/config/debug/debug_cfg.h"                          /*  调试器功能配置              */
#include "../SylixOS/config/kernel/kernel_cfg.h"                        /*  系统内核配置文件            */
#include "../SylixOS/config/system/system_cfg.h"                        /*  外围系统配置                */
#include "../SylixOS/config/mosipc/mosipc_cfg.h"                        /*  多操作系统通信配置文件      */
#include "../SylixOS/config/mp/mp_cfg.h"                                /*  多处理器配置文件            */
#include "../SylixOS/config/shell/shell_cfg.h"                          /*  shell 系统配置文件          */
#include "../SylixOS/config/fs/fs_cfg.h"                                /*  文件系统配置文件            */
#include "../SylixOS/config/gjb/gjb_cfg.h"                              /*  国军标系统配置文件          */
#include "../SylixOS/config/net/net_cfg.h"                              /*  网络系统配置文件            */
#include "../SylixOS/config/loader/loader_cfg.h"                        /*  装载器服务配置文件          */
#include "../SylixOS/config/symbol/symbol_cfg.h"                        /*  符号表服务配置文件          */
#include "../SylixOS/config/posix/posix_cfg.h"                          /*  posix 兼容系统配置文件      */
/*********************************************************************************************************
  体系结构配置
*********************************************************************************************************/
#include "../SylixOS/include/arch/arch_inc.h"                           /*  体系结构相关                */
/*********************************************************************************************************
  依存关系
*********************************************************************************************************/

#undef  LW_CFG_CPU_HOOKS_EN
#define LW_CFG_CPU_HOOKS_EN        1                                    /*  0.9.5 版本后必须支持 HOOK   */

#if     LW_CFG_MAX_THREADS < 8
#undef  LW_CFG_MAX_THREADS
#define LW_CFG_MAX_THREADS         8                                    /*  最少 8 条线程               */
#endif

#if     LW_CFG_THREAD_RESTART_EN > 0
#undef  LW_CFG_THREAD_DEL_EN
#define LW_CFG_THREAD_DEL_EN       1                                    /*  支持删除                    */
#endif

#if     LW_CFG_OBJECT_NAME_SIZE < 16
#undef  LW_CFG_OBJECT_NAME_SIZE
#define LW_CFG_OBJECT_NAME_SIZE   16                                    /*  对象名至少 16 字节          */
#endif

#undef  LW_CFG_MSGQUEUE_EN
#define LW_CFG_MSGQUEUE_EN         1                                    /*  必须支持消息队列            */

#undef  LW_CFG_RTC_EN
#define LW_CFG_RTC_EN              1                                    /*  从 0.9.7 版后, 必须支持     */

/*********************************************************************************************************
  事件
*********************************************************************************************************/

#undef  LW_CFG_SEMCBM_EN
#define LW_CFG_SEMCBM_EN           ((LW_CFG_SEMC_EN  > 0) || \
                                    (LW_CFG_SEMM_EN  > 0) || \
                                    (LW_CFG_SEMB_EN  > 0))

#undef  LW_CFG_SEM_EN
#define LW_CFG_SEM_EN              ((LW_CFG_SEMCBM_EN > 0) || \
                                    (LW_CFG_SEMRW_EN  > 0))

#undef  LW_CFG_EVENT_EN
#define LW_CFG_EVENT_EN            (((LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)) || \
                                    (LW_CFG_SEM_EN > 0))
/*********************************************************************************************************
  编译器
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_compiler.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  系统优先级
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_priority.h"                       /*  优先级定义                  */
/*********************************************************************************************************
  系统错误定义
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_object.h"                         /*  对象类型                    */
#include "../SylixOS/kernel/include/k_error.h"                          /*  错误定义                    */
/*********************************************************************************************************
  KERNEL ROUTINE TYPE DEFINE
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_routine.h"                        /*  KERNEL ROUTINE TYPE DEFINE  */
/*********************************************************************************************************
  系统内部宏和附值
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_value.h"                          /*  系统定义的特殊值文件        */
#include "../SylixOS/kernel/include/k_option.h"
#include "../SylixOS/kernel/include/k_hook.h"
#include "../SylixOS/kernel/include/k_typemacro.h"
/*********************************************************************************************************
  LIST & TREE
*********************************************************************************************************/
#include "../SylixOS/kernel/list/list.h"                                /*  系统链表类型定义            */
#include "../SylixOS/kernel/tree/tree.h"                                /*  系统链树类型定义            */
/*********************************************************************************************************
  POSIX
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_pconst.h"
#include "../SylixOS/kernel/include/k_ptype.h"
#include "../SylixOS/kernel/include/k_perror.h"
/*********************************************************************************************************
  SMP
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_spinlock.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  系统内部功能与结构头文件
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_const.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#include "../SylixOS/kernel/include/k_functype.h"
#include "../SylixOS/kernel/include/k_class.h"
/*********************************************************************************************************
  资源管理器
*********************************************************************************************************/
#include "../SylixOS/kernel/resource/resource.h"
/*********************************************************************************************************
  JOB QUEUE
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_jobqueue.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  LIB
*********************************************************************************************************/
#include "../SylixOS/lib/lib_lib.h"
/*********************************************************************************************************
  CPU
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_ipi.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#include "../SylixOS/kernel/include/k_cpu.h"
/*********************************************************************************************************
  ARCH 原子库
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/arch/arch_atomic.h"
/*********************************************************************************************************
  变量, 回调, 位图, 原子...
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_globalvar.h"
#include "../SylixOS/kernel/include/k_bitmap.h"
#include "../SylixOS/kernel/include/k_atomic.h"
/*********************************************************************************************************
  系统内部功能函数声明
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_internal.h"
#include "../SylixOS/kernel/include/k_timecvt.h"
#include "../SylixOS/kernel/include/k_kernelinit.h"                     /*  系统初始化函数集            */
#include "../SylixOS/kernel/include/k_sched.h"                          /*  调度器                      */
/*********************************************************************************************************
  系统内联函数库
*********************************************************************************************************/
#include "../SylixOS/kernel/inline/inl.h"
/*********************************************************************************************************
  资源分配文件
*********************************************************************************************************/
#include "../SylixOS/kernel/list/listResource.h"                        /*  资源分配与回收              */
/*********************************************************************************************************
  内核扫描链
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_scanlink.h"
/*********************************************************************************************************
  系统/应用崩溃信息记录
*********************************************************************************************************/
#include "../SylixOS/kernel/cdump/cdumpLib.h"
#include "../SylixOS/kernel/cdump/cdump.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  MMU & CACHE
*********************************************************************************************************/
#include "../SylixOS/kernel/cache/cache.h"
#include "../SylixOS/kernel/vmm/vmm.h"
#include "../SylixOS/kernel/vmm/vmmMmap.h"
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/kernel/vmm/vmmIo.h"
#include "../SylixOS/kernel/vmm/pageLib.h"
#include "../SylixOS/kernel/vmm/vmmArea.h"
#include "../SylixOS/kernel/vmm/vmmSwap.h"
#include "../SylixOS/kernel/vmm/pageTable.h"
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  内部 API 函数声明 
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_api.h"
#include "../SylixOS/kernel/threadext/threadext.h"
/*********************************************************************************************************
  内核事件监控器
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/monitor/include/monitor.h"
#endif
/*********************************************************************************************************
  体系结构支持代码
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#include "../SylixOS/arch/arch_support.h"                               /*  体系架构支持                */
#ifdef __SYLIXOS_IO
#include "../SylixOS/arch/arch_io.h"                                    /*  体系架构 I/O 访问           */
#endif                                                                  /*  __SYLIXOS_IO                */
#ifdef __SYLIXOS_GDB
#include "../SylixOS/arch/arch_gdb.h"                                   /*  体系架构 GDB 支持           */
#endif                                                                  /*  __SYLIXOS_GDB               */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  __K_KERNEL_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
