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
** 文   件   名: mips_support.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 体系构架支持.
*********************************************************************************************************/

#ifndef __ARCH_MIPS_SUPPORT_H
#define __ARCH_MIPS_SUPPORT_H

#define __LW_SCHEDULER_BUG_TRACE_EN                                     /*  测试多核调度器              */
#define __LW_KERNLOCK_BUG_TRACE_EN                                      /*  测试内核锁                  */

/*********************************************************************************************************
  汇编相关头文件
*********************************************************************************************************/

#include "arch/assembler.h"

/*********************************************************************************************************
  存储器定义 (CPU 栈区操作)
*********************************************************************************************************/

#define CPU_STK_GROWTH              1                                   /*  1：入栈从高地址向低地址     */
                                                                        /*  0：入栈从低地址向高地址     */
/*********************************************************************************************************
  arch 已经提供的接口如下:
*********************************************************************************************************/
/*********************************************************************************************************
  MIPS 处理器断言
*********************************************************************************************************/

VOID    archAssert(INT  iCond, CPCHAR  pcFunc, CPCHAR  pcFile, INT  iLine);

/*********************************************************************************************************
  MIPS 处理器线程上下文相关接口
*********************************************************************************************************/

PLW_STACK       archTaskCtxCreate(PTHREAD_START_ROUTINE  pfuncTask,
                                  PVOID                  pvArg,
                                  PLW_STACK              pstkTop,
                                  ULONG                  ulOpt);
VOID            archTaskCtxSetFp(PLW_STACK  pstkDest, PLW_STACK  pstkSrc);
ARCH_REG_CTX   *archTaskRegsGet(PLW_STACK  pstkTop, ARCH_REG_T *pregSp);
VOID            archTaskRegsSet(PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx);

#if LW_CFG_DEVICE_EN > 0
VOID        archTaskCtxShow(INT  iFd, PLW_STACK  pstkTop);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

VOID        archTaskCtxStart(PLW_CLASS_CPU  pcpuSw);
VOID        archTaskCtxSwitch(PLW_CLASS_CPU  pcpuSw);

#if LW_CFG_COROUTINE_EN > 0
VOID        archCrtCtxSwitch(PLW_CLASS_CPU  pcpuSw);
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

VOID        archIntCtxLoad(PLW_CLASS_CPU  pcpuSw);
VOID        archSigCtxLoad(PVOID  pvStack);

/*********************************************************************************************************
  MIPS 处理器调试接口
*********************************************************************************************************/

#if LW_CFG_GDB_EN > 0
VOID    archDbgBpInsert(addr_t   ulAddr, size_t stSize, ULONG  *pulIns, BOOL  bLocal);
VOID    archDbgAbInsert(addr_t   ulAddr, ULONG  *pulIns);
VOID    archDbgBpRemove(addr_t   ulAddr, size_t stSize, ULONG   ulIns, BOOL  bLocal);
VOID    archDbgBpPrefetch(addr_t ulAddr);
UINT    archDbgTrapType(addr_t   ulAddr, PVOID   pvArch);
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  MIPS 处理器异常接口
*********************************************************************************************************/

VOID    archIntHandle(ULONG  ulVector, BOOL  bPreemptive);              /*  bspIntHandle 需要调用此函数 */

/*********************************************************************************************************
  MIPS 通用库
*********************************************************************************************************/

INT     archFindLsb(UINT32 ui32);
INT     archFindMsb(UINT32 ui32);

/*********************************************************************************************************
  ARM 处理器标准底层库
*********************************************************************************************************/

#define KN_INT_DISABLE()            archIntDisable()
#define KN_INT_ENABLE(intLevel)     archIntEnable(intLevel)
#define KN_INT_ENABLE_FORCE()       archIntEnableForce()

INTREG  archIntDisable(VOID);
VOID    archIntEnable(INTREG  iregInterLevel);
VOID    archIntEnableForce(VOID);

VOID    archPageCopy(PVOID pvTo, PVOID pvFrom);

#define KN_COPY_PAGE(to, from)      archPageCopy(to, from)

VOID    archReboot(INT  iRebootType, addr_t  ulStartAddress);

/*********************************************************************************************************
  MIPS 处理器 CACHE 操作
*********************************************************************************************************/

#define MIPS_MACHINE_24KF   "24kf"

#if LW_CFG_CACHE_EN > 0
VOID    archCacheReset(CPCHAR     pcMachineName);
VOID    archCacheInit(CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName);

#define __ARCH_CACHE_INIT   archCacheInit
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

/*********************************************************************************************************
  MIPS 处理器 MMU 操作 (armMmu*() 可供给 bsp 使用)
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
VOID    archMmuInit(CPCHAR  pcMachineName);

#define __ARCH_MMU_INIT     archMmuInit
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

/*********************************************************************************************************
  MIPS 处理器多核自旋锁操作
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
VOID    archSpinInit(spinlock_t  *psl);
VOID    archSpinDelay(VOID);
VOID    archSpinNotify(VOID);

#define __ARCH_SPIN_INIT    archSpinInit
#define __ARCH_SPIN_DELAY   archSpinDelay
#define __ARCH_SPIN_NOTIFY  archSpinNotify

INT     archSpinLock(spinlock_t  *psl);
INT     archSpinTryLock(spinlock_t  *psl);
INT     archSpinUnlock(spinlock_t  *psl);

#define __ARCH_SPIN_LOCK    archSpinLock
#define __ARCH_SPIN_TRYLOCK archSpinTryLock
#define __ARCH_SPIN_UNLOCK  archSpinUnlock

INT     archSpinLockRaw(spinlock_t  *psl);
INT     archSpinTryLockRaw(spinlock_t  *psl);
INT     archSpinUnlockRaw(spinlock_t  *psl);

#define __ARCH_SPIN_LOCK_RAW    archSpinLockRaw
#define __ARCH_SPIN_TRYLOCK_RAW archSpinTryLockRaw
#define __ARCH_SPIN_UNLOCK_RAW  archSpinUnlockRaw

ULONG   archMpCur(VOID);
VOID    archMpInt(ULONG  ulCPUId);
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  MIPS 内存屏障
*********************************************************************************************************/

#define KN_BARRIER()            __asm__ __volatile__ ("" : : : "memory")

#if LW_CFG_MIPS_HAS_SYNC_INSTR > 0
#define KN_SYNC()                       \
        __asm__ __volatile__ (          \
        ".set   push\n\t"               \
        ".set   noreorder\n\t"          \
        ".set   mips2\n\t"              \
        "sync\n\t"                      \
        ".set   pop"                    \
        : : : "memory")
#else
#define KN_SYNC()               KN_BARRIER()
#endif                                                                  /*  LW_CFG_MIPS_HAS_SYNC_INSTR  */

#define KN_MB()                 KN_SYNC()
#define KN_RMB()                KN_SYNC()
#define KN_WMB()                KN_SYNC()

#if LW_CFG_SMP_EN > 0
#define KN_SMP_MB()             __asm__ __volatile__ ("sync" : : : "memory")
#define KN_SMP_RMB()            __asm__ __volatile__ ("sync" : : : "memory")
#define KN_SMP_WMB()            __asm__ __volatile__ ("sync" : : : "memory")

#else
#define KN_SMP_MB()             KN_BARRIER()
#define KN_SMP_RMB()            KN_BARRIER()
#define KN_SMP_WMB()            KN_BARRIER()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  MIPS 处理器浮点运算器
*********************************************************************************************************/

#define MIPS_FPU_VFP32    "vfp32"
#define MIPS_FPU_VFP64    "vfp64"

#if LW_CFG_CPU_FPU_EN > 0
VOID    archFpuPrimaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
#if LW_CFG_SMP_EN > 0
VOID    archFpuSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

VOID    archFpuCtxInit(PVOID pvFpuCtx);
VOID    archFpuEnable(VOID);
VOID    archFpuDisable(VOID);
VOID    archFpuSave(PVOID pvFpuCtx);
VOID    archFpuRestore(PVOID pvFpuCtx);

#define __ARCH_FPU_CTX_INIT     archFpuCtxInit
#define __ARCH_FPU_ENABLE       archFpuEnable
#define __ARCH_FPU_DISABLE      archFpuDisable
#define __ARCH_FPU_SAVE         archFpuSave
#define __ARCH_FPU_RESTORE      archFpuRestore

#if LW_CFG_DEVICE_EN > 0
VOID    archFpuCtxShow(INT  iFd, PVOID pvFpuCtx);
#define __ARCH_FPU_CTX_SHOW     archFpuCtxShow
#endif                                                                  /*  LW_CFG_DEVICE_EN            */

INT     archFpuUndHandle(PLW_CLASS_TCB  ptcbCur);
#endif                                                                  /*  LW_CFG_CPU_FPU_EN           */

/*********************************************************************************************************
  bsp 需要提供的接口如下:
*********************************************************************************************************/
/*********************************************************************************************************
  MIPS 处理器中断向量判读
*********************************************************************************************************/

VOID    bspIntInit(VOID);
VOID    bspIntHandle(VOID);

VOID    bspIntVectorEnable(ULONG  ulVector);
VOID    bspIntVectorDisable(ULONG  ulVector);
BOOL    bspIntVectorIsEnable(ULONG  ulVector);

#define __ARCH_INT_VECTOR_ENABLE    bspIntVectorEnable
#define __ARCH_INT_VECTOR_DISABLE   bspIntVectorDisable
#define __ARCH_INT_VECTOR_ISENABLE  bspIntVectorIsEnable

/*********************************************************************************************************
  CPU 定时器时钟
*********************************************************************************************************/

VOID    bspTickInit(VOID);
VOID    bspDelayUs(ULONG ulUs);
VOID    bspDelayNs(ULONG ulNs);

#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
VOID    bspTickHighResolution(struct timespec *ptv);
#endif                                                                  /*  LW_CFG_TIME_HIGH_...        */

/*********************************************************************************************************
  内核关键位置回调函数
*********************************************************************************************************/

VOID    bspTaskCreateHook(LW_OBJECT_ID  ulId);
VOID    bspTaskDeleteHook(LW_OBJECT_ID  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb);
VOID    bspTaskSwapHook(LW_OBJECT_HANDLE  hOldThread, LW_OBJECT_HANDLE  hNewThread);
VOID    bspTaskIdleHook(VOID);
VOID    bspTickHook(INT64  i64Tick);
VOID    bspWdTimerHook(LW_OBJECT_ID  ulId);
VOID    bspTCBInitHook(LW_OBJECT_ID  ulId, PLW_CLASS_TCB   ptcb);
VOID    bspKernelInitHook(VOID);

/*********************************************************************************************************
  系统重启操作 (ulStartAddress 参数用于调试, BSP 可忽略)
*********************************************************************************************************/

VOID    bspReboot(INT  iRebootType, addr_t  ulStartAddress);

/*********************************************************************************************************
  系统关键信息打印 (打印信息不可依赖任何操作系统 api)
*********************************************************************************************************/

VOID    bspDebugMsg(CPCHAR pcMsg);

/*********************************************************************************************************
  BSP 信息
*********************************************************************************************************/

CPCHAR  bspInfoCpu(VOID);
CPCHAR  bspInfoCache(VOID);
CPCHAR  bspInfoPacket(VOID);
CPCHAR  bspInfoVersion(VOID);
ULONG   bspInfoHwcap(VOID);
addr_t  bspInfoRomBase(VOID);
size_t  bspInfoRomSize(VOID);
addr_t  bspInfoRamBase(VOID);
size_t  bspInfoRamSize(VOID);

/*********************************************************************************************************
  MIPS 处理器 MMU 操作
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
ULONG   bspMmuPgdMaxNum(VOID);
ULONG   bspMmuPteMaxNum(VOID);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

/*********************************************************************************************************
  MIPS 处理器多核操作
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
VOID    bspMpInt(ULONG  ulCPUId);
VOID    bspCpuUp(ULONG  ulCPUId);                                       /*  启动一个 CPU                */
VOID    bspCpuDown(ULONG  ulCPUId);                                     /*  停止一个 CPU                */
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  MIPS 处理器 CPU 功耗 (升降主频)
*********************************************************************************************************/

#if LW_CFG_POWERM_EN > 0
VOID    bspSuspend(VOID);                                               /*  系统休眠                    */
VOID    bspCpuPowerSet(UINT  uiPowerLevel);                             /*  设置 CPU 主频等级           */
VOID    bspCpuPowerGet(UINT *puiPowerLevel);                            /*  获取 CPU 主频等级           */
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

#endif                                                                  /*  __ARCH_MIPS_SUPPORT_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
