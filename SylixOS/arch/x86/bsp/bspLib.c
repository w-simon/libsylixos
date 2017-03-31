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
** 文   件   名: bspLib.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 7 月 31 日
**
** 描        述: 处理器需要为 SylixOS 提供的功能支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/asm/hwcap.h"
#include "arch/x86/common/x86Idt.h"
#include "arch/x86/common/x86CpuId.h"
#include "arch/x86/mpcore/x86MpCore.h"
#include "arch/x86/apic/x86IoApic.h"
#include "arch/x86/apic/x86LocalApic.h"
#include "arch/x86/param/x86Param.h"
#include "arch/x86/acpi/include/acpi_interface.h"
#include "driver/int/i8259a.h"
#include "driver/timer/i8254.h"
/*********************************************************************************************************
  高精度 TIMER 配置
*********************************************************************************************************/
#define TICK_HIGH_RESOLUTION_TSC    1                                   /*  高精度实现 0: 8254, 1: TSC  */
/*********************************************************************************************************
** 函数名称: bspInfoCpu
** 功能描述: BSP CPU 信息
** 输　入  : NONE
** 输　出  : CPU 信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK CPCHAR  bspInfoCpu (VOID)
{
    return  (_G_pcX86CpuInfo);
}
/*********************************************************************************************************
** 函数名称: bspInfoCache
** 功能描述: BSP CACHE 信息
** 输　入  : NONE
** 输　出  : CACHE 信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK CPCHAR  bspInfoCache (VOID)
{
    return  (_G_pcX86CacheInfo);
}
/*********************************************************************************************************
** 函数名称: bspInfoHwcap
** 功能描述: BSP 硬件特性
** 输　入  : NONE
** 输　出  : 硬件特性 (如果支持硬浮点, 可以加入 HWCAP_FPU, HWCAP_MMX, HWCAP_SSE, HWCAP_SSE2, HWCAP_AVX)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG  bspInfoHwcap (VOID)
{
    ULONG  ulHwcap = 0;

    if (_G_bX86HasX87FPU) {
        ulHwcap |= HWCAP_FPU;
    }

    if (_G_bX86HasMMX) {
        ulHwcap |= HWCAP_MMX;
    }

    if (_G_bX86HasSSE) {
        ulHwcap |= HWCAP_SSE;
    }

    if (_G_bX86HasSSE2) {
        ulHwcap |= HWCAP_SSE2;
    }

    if (_G_bX86HasAVX) {
        ulHwcap |= HWCAP_AVX;
    }

    return  (ulHwcap);
}
/*********************************************************************************************************
  中断相关

  x86 Int Vector:
  +-----------------+
  |  0              |
  |                 |
  |  x86 exception  |
  |                 |
  |  31             |           SylixOS Int Vector:
  +-----------------+ --------> +-----------------+
  |  32             |           |  0              |
  |                 |           |                 | ------> 16
  |  x86 IRQ        |           |  x86 IRQ        | PCI x 8 Intx
  |                 |           |                 | ------> 23
  |  63             |           |  31             |
  +-----------------+           +-----------------+
  |  64             |           |  32             |
  |                 |           |                 |
  |  x86 SMP IPI    |  INT MAP  |  SMP IPI        |
  |                 |           |                 |
  |  95             |           |  63             |
  +-----------------+           +-----------------+
  |  96             |           |  64             |
  |                 |           |                 |
  |  MSI & other... |           |  IRQ            |
  |                 |           |                 |
  |  255            |           |  223            |
  +-----------------+ --------> +-----------------+
*********************************************************************************************************/
/*********************************************************************************************************
  8259A PIC
*********************************************************************************************************/
#define X86_8259A_MASTER_IO_BASE    0x20
#define X86_8259A_SLAVE_IO_BASE     0xa0
#define X86_8259A_VECTOR_NR         16

#define X86_8259A_MASTER_IMR        (X86_8259A_MASTER_IO_BASE + 0x01)
#define X86_8259A_SLAVE_IMR         (X86_8259A_SLAVE_IO_BASE  + 0x01)
/*********************************************************************************************************
  i8259A 平台数据
*********************************************************************************************************/
static I8259A_CTL _G_i8259aData = {
    .iobase_master  = X86_8259A_MASTER_IO_BASE,
    .iobase_slave   = X86_8259A_SLAVE_IO_BASE,
    .trigger        = 0,
#if TICK_HIGH_RESOLUTION_TSC > 0
    .manual_eoi     = 0,
#else
    .manual_eoi     = 1,
#endif
    .vector_base    = 32,
};
/*********************************************************************************************************
** 函数名称: bspIntInit
** 功能描述: 中断系统初始化
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspIntInit (VOID)
{
    /*
     * 如果某中断为链式中断，请加入形如:
     * API_InterVectorSetFlag(LW_IRQ_4, LW_IRQ_FLAG_QUEUE);
     * 的代码.
     *
     * 如果某中断可用作初始化随机化种子，请加入形如:
     * API_InterVectorSetFlag(LW_IRQ_0, LW_IRQ_FLAG_SAMPLE_RAND);
     * 的代码.
     */
    i8259aInit(&_G_i8259aData);                                         /*  初始化 8259A                */
    i8259aIrqEnable(&_G_i8259aData, LW_IRQ_2);                          /*  使能 IRQ2                   */

    if (_G_bX86HasAPIC) {
        /*
         * 8 个 PCI 中断设置为链式中断
         */
        API_InterVectorSetFlag(LW_IRQ_16, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_17, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_18, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_19, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_20, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_21, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_22, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_23, LW_IRQ_FLAG_QUEUE);

        x86LocalApicInit();                                             /*  初始化 Local APIC           */
        x86IoApicInit();                                                /*  初始化 IO APIC              */
    }
}
/*********************************************************************************************************
  向量使能与禁能锁
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slVectorTable)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slVectorTable)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: bspIntHandle
** 功能描述: 中断处理函数
** 输  入  : ulVector     中断向量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspIntHandle (ULONG  ulVector)
{
    archIntHandle(ulVector, LW_FALSE);                                  /*  不允许中断嵌套(MSI 中断无法 */
                                                                        /*  屏蔽)                       */
    if (ulVector != X86_IRQ_TIMER) {                                    /*  TIMER HOOK 独立清理 8259    */
        if (_G_bX86HasAPIC) {
            VECTOR_OP_LOCK();
            x86IoApicIrqEoi(ulVector);                                  /*  SylixOS vector 与 ioapic irq*/
            VECTOR_OP_UNLOCK();                                         /*  一致                        */

        }
#if TICK_HIGH_RESOLUTION_TSC == 0
          else {
            i8259aIrqEoi(&_G_i8259aData, (UINT)ulVector);
        }
#endif                                                                  /*  !TICK_HIGH_RESOLUTION_TSC   */
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorEnable
** 功能描述: 使能指定的中断向量
** 输  入  : ulVector     中断向量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspIntVectorEnable (ULONG  ulVector)
{
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer 用虚拟线模式          */
        x86IoApicIrqEnable(ulVector);
        x86LocalApicIrqEnable(ulVector);

    } else {
        if (ulVector < X86_8259A_VECTOR_NR) {
            i8259aIrqEnable(&_G_i8259aData, ulVector);
        }
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorDisable
** 功能描述: 禁能指定的中断向量
** 输  入  : ulVector     中断向量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspIntVectorDisable (ULONG  ulVector)
{
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer 用虚拟线模式          */
        x86LocalApicIrqDisable(ulVector);
        x86IoApicIrqDisable(ulVector);

    } else {
        if (ulVector < X86_8259A_VECTOR_NR) {
            i8259aIrqDisable(&_G_i8259aData, ulVector);
        }
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorIsEnable
** 功能描述: 检查指定的中断向量是否使能
** 输  入  : ulVector     中断向量
** 输  出  : LW_FALSE 或 LW_TRUE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK BOOL  bspIntVectorIsEnable (ULONG  ulVector)
{
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer 用虚拟线模式          */
        return  (x86IoApicIrqIsEnable(ulVector));

    } else {
        if (ulVector < X86_8259A_VECTOR_NR) {
            return  (i8259aIrqIsEnable(&_G_i8259aData, ulVector));

        } else {
            return  (LW_FALSE);
        }
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorSetPriority
** 功能描述: 设置指定的中断向量的优先级
** 输  入  : ulVector     中断向量
**           uiPrio       优先级
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_INTER_PRIO > 0

LW_WEAK ULONG   bspIntVectorSetPriority (ULONG  ulVector, UINT  uiPrio)
{
    /*
     * 不能设置优先级
     */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: bspIntVectorGetPriority
** 功能描述: 获取指定的中断向量的优先级
** 输  入  : ulVector     中断向量
**           puiPrio      优先级
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG   bspIntVectorGetPriority (ULONG  ulVector, UINT  *puiPrio)
{
    *puiPrio = ulVector;                                                /*  Vector 越大，优先级越高     */
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
** 函数名称: bspIntVectorSetTarget
** 功能描述: 设置指定的中断向量的目标 CPU
** 输　入  : ulVector      中断向量
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_INTER_TARGET > 0

LW_WEAK ULONG   bspIntVectorSetTarget (ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer 用虚拟线模式          */
        ULONG   i;
        ULONG   ulNumChk;

        ulNumChk = ((ULONG)stSize << 3);
        ulNumChk = (ulNumChk > LW_NCPUS) ? LW_NCPUS : ulNumChk;

        for (i = 0; i < ulNumChk; i++) {
            if (LW_CPU_ISSET(i, pcpuset)) {
                x86IoApicIrqSetTarget(ulVector, X86_CPUID_TO_APICID(i));
                break;
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: bspIntVectorGetTarget
** 功能描述: 获取指定的中断向量的目标 CPU
** 输　入  : ulVector      中断向量
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG   bspIntVectorGetTarget (ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    LW_CPU_ZERO(pcpuset);

    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer 用虚拟线模式          */
        UINT8  ucTargetLocalApicId;

        x86IoApicIrqGetTarget(ulVector, &ucTargetLocalApicId);
        LW_CPU_SET(X86_APICID_TO_CPUID(ucTargetLocalApicId), pcpuset);

    } else {
        LW_CPU_SET(0, pcpuset);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */
/*********************************************************************************************************
  MMU 相关接口
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: bspMmuPgdMaxNum
** 功能描述: 获得 PGD 池的数量
** 输  入  : NONE
** 输  出  : PGD 池的数量 (1 个池可映射 4GB 空间, 推荐返回 1)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuPgdMaxNum (VOID)
{
    return  (1);
}
/*********************************************************************************************************
** 函数名称: bspMmuPgdMaxNum
** 功能描述: 获得 PTE 池的数量
** 输  入  : NONE
** 输  出  : PTE 池的数量 (映射 4GB 空间, 需要 1024 个 PTE 池)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuPteMaxNum (VOID)
{
    return  (1024);
}
/*********************************************************************************************************
  多核相关接口
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define X86_AP_BOOT_STACK_SIZE      (32 * LW_CFG_KB_SIZE)
#define X86_AP_BOOT_ENTRY_ADDR      (0x7000)
/*********************************************************************************************************
  外部变量声明
*********************************************************************************************************/
extern PLW_STACK    _K_pstkInterruptBase[LW_CFG_MAX_PROCESSORS];        /*  中断处理时的堆栈基地址      */
/*********************************************************************************************************
  AP 进入点代码
*********************************************************************************************************/
static const UINT8  _G_ucX86ApEntryCode[] = {
    0xfa, 0x31, 0xc0, 0x8e, 0xd8, 0x8e,
    0xc0, 0x8e, 0xd0, 0x0f, 0x01, 0x16,
    0x68, 0x70, 0x0f, 0x20, 0xc0, 0x66,
    0x83, 0xc8, 0x01, 0x0f, 0x22, 0xc0,
    0x66, 0xea, 0x20, 0x70, 0x00, 0x00,
    0x08, 0x00, 0x66, 0xb8, 0x10, 0x00,
    0x8e, 0xd8, 0x8e, 0xc0, 0x8e, 0xd0,
    0x66, 0xb8, 0x00, 0x00, 0x8e, 0xe0,
    0x8e, 0xe8, 0x8b, 0x25, 0xfc, 0x6f,
    0x00, 0x00, 0xff, 0x15, 0xf8, 0x6f,
    0x00, 0x00, 0x66, 0xb8, 0x00, 0x8a,
    0x66, 0x89, 0xc2, 0x66, 0xef, 0x66,
    0xb8, 0xe0, 0x8a, 0x66, 0xef, 0xeb,
    0xfe, 0x90, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x92,
    0xcf, 0x00, 0x17, 0x00, 0x50, 0x70,
    0x00, 0x00, 0x00
};
/*********************************************************************************************************
  AP 自旋锁
*********************************************************************************************************/
static SPINLOCKTYPE _G_slX86ApLock = 0;
/*********************************************************************************************************
** 函数名称: bspCpuUp
** 功能描述: 启动一个 CPU
** 输  入  : ulCPUId      目标 CPU
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID   bspCpuUp (ULONG  ulCPUId)
{
    UINT32     *puiEntryAddr;
    PLW_STACK   pstkStack;

    if (_G_iX86LProcNr <= 1) {
        return;
    }

    x86SpinLock(&_G_slX86ApLock);

    pstkStack    = _K_pstkInterruptBase[ulCPUId];
    puiEntryAddr = (UINT32 *)X86_AP_BOOT_ENTRY_ADDR;

    lib_memcpy(puiEntryAddr, _G_ucX86ApEntryCode, (size_t)sizeof(_G_ucX86ApEntryCode));

    *(puiEntryAddr - 1) = ((UINT32)pstkStack) + X86_AP_BOOT_STACK_SIZE;
    *(puiEntryAddr - 2) = (UINT32)bspSecondaryInit;

    x86LocalApicSecondaryStart(X86_CPUID_TO_APICID(ulCPUId), (addr_t)puiEntryAddr);
}
/*********************************************************************************************************
** 函数名称: bspCpuUpDone
** 功能描述: 一个 CPU 启动完成
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID   bspCpuUpDone (VOID)
{
    x86SpinUnlock(&_G_slX86ApLock);
}
/*********************************************************************************************************
** 函数名称: bspSecondaryCpusUp
** 功能描述: 启动所有的 Secondary CPU
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspSecondaryCpusUp (VOID)
{
    UINT8  ucLocalApicId;
    INT    i;

    if (_G_iX86LProcNr <= 1) {
        return;
    }

    ucLocalApicId = x86LocalApicId();

    for (i = 0; i < (2 * LW_CFG_MAX_PROCESSORS); i++) {
        if (i != ucLocalApicId) {
            if (X86_APICID_PRESEND(i)) {
                API_CpuUp(X86_APICID_TO_CPUID(i));
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: bspCpuLogic2Physical
** 功能描述: 获得逻辑 CPU 的物理 ID
** 输  入  : ulCPUId      目标 CPU
** 输  出  : 物理 ID
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CPU_ARCH_SMT > 0

LW_WEAK ULONG  bspCpuLogic2Physical (ULONG  ulCPUId)
{
    UINT8   ucLocalApicId = X86_CPUID_TO_APICID(ulCPUId);

    if (X86_APICID_IS_HT(ucLocalApicId)) {
        return  (ulCPUId - 1);
    }

    return  (ulCPUId);
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
/*********************************************************************************************************
** 函数名称: bspCpuDown
** 功能描述: 停止一个 CPU
** 输  入  : ulCPUId      目标 CPU
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

LW_WEAK VOID   bspCpuDown (ULONG  ulCPUId)
{
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: bspCpuIpiVectorInstall
** 功能描述: 安装 IPI 向量
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspCpuIpiVectorInstall (VOID)
{
    ULONG  ulCPUId;

    ulCPUId = archMpCur();

    API_InterVectorIpi(ulCPUId, 32 + ulCPUId);                          /*  SylixOS IPI 向量从 32 开始  */
}
/*********************************************************************************************************
  操作系统时间相关函数
*********************************************************************************************************/
/*********************************************************************************************************
  8254 TIMER
*********************************************************************************************************/
#define X86_8254_IO_BASE            0x40
#define X86_8254_BUZZER_IO_BASE     0x61                                /*  8255 buzzer                 */
#define X86_8254_MAX_FREQ           1193182
/*********************************************************************************************************
  TICK 服务相关配置
*********************************************************************************************************/
#define TICK_IN_THREAD  0

#if TICK_IN_THREAD > 0
static LW_HANDLE    htKernelTicks;                                      /*  操作系统时钟服务线程句柄    */
#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
  i8254 平台数据
*********************************************************************************************************/
static I8254_CTL _G_i8254Data = {
    .iobase   = X86_8254_IO_BASE,
    .iobuzzer = X86_8254_BUZZER_IO_BASE,
    .qcofreq  = X86_8254_MAX_FREQ,
};
/*********************************************************************************************************
  精确时间换算参数
*********************************************************************************************************/
static UINT32   _G_uiFullCnt;
static UINT64   _G_ui64NSecPerCnt7;                                     /*  提高 7bit 精度              */
/*********************************************************************************************************
** 函数名称: __tickThread
** 功能描述: 初始化 tick 服务线程
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if TICK_IN_THREAD > 0

static VOID  __tickThread (VOID)
{
    for (;;) {
        API_ThreadSuspend(htKernelTicks);
        API_KernelTicks();                                              /*  内核 TICKS 通知             */
        API_TimerHTicks();                                              /*  高速 TIMER TICKS 通知       */
    }
}

#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
  TSC TIMER
*********************************************************************************************************/
#if TICK_HIGH_RESOLUTION_TSC > 0

#define READ_TSC_LL(count)  __asm__ __volatile__("rdtsc" : "=A" (count))
#define PIT_CH2             0x42
#define PIT_MODE            0x43
#define CALIBRATE_CYCLES    14551
#define CALIBRATE_MULT      82
#define NSECS_PER_SEC       1000000000UL

/*********************************************************************************************************
  修正偏差
*********************************************************************************************************/
#define TOD_TSCDIFF(tod, diff_nsec)                     \
        do {                                            \
            (tod)->tv_nsec += diff_nsec;                \
            if ((tod)->tv_nsec >= NSECS_PER_SEC) {      \
                (tod)->tv_nsec -= NSECS_PER_SEC;        \
                (tod)->tv_sec++;                        \
            }                                           \
            if ((tod)->tv_nsec < 0) {                   \
                (tod)->tv_nsec += NSECS_PER_SEC;        \
                (tod)->tv_sec--;                        \
            }                                           \
        } while (0)

static          UINT64      _G_ullCpuFreq;                              /*  每秒 CPU 周期数             */
static volatile UINT64      _G_ullPreTickTscNs;                         /*  上一 Tick 发生时 TSC 时刻   */
static volatile INT64       _G_llTscDiffNs;                             /*  TSC 时间和 Tick 时间偏差    */
/*********************************************************************************************************
** 函数名称: bspTscInit
** 功能描述: 初始化 tsc 模块，获取1秒有多少个CPU时钟周期
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  bspTscInit (VOID)
{
    UINT64 ullStart;
    UINT64 ullDelta;
    UINT64 ullStop;

    out8((in8(0x61) & ~0x02) | 0x01, 0x61);
    out8(0xb0, PIT_MODE);
    out8(CALIBRATE_CYCLES & 0xff, PIT_CH2);
    out8(CALIBRATE_CYCLES >> 8,   PIT_CH2);
    READ_TSC_LL(ullStart);

    READ_TSC_LL(ullDelta);
    in8(0x61);
    READ_TSC_LL(ullStop);
    ullDelta = ullStop - ullDelta;

    while ((in8(0x61) & 0x20) == 0);
    READ_TSC_LL(ullStop);

    _G_ullCpuFreq = ((ullStop - ullDelta) - ullStart) * CALIBRATE_MULT;

    _G_ullPreTickTscNs = 0;
}
/*********************************************************************************************************
** 函数名称: drvTscGetNsec
** 功能描述: 获得开机到现在运行的纳秒数
** 输　入  : NONE
** 输　出  : 纳秒数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT64  bspTscGetNsec (VOID)
{
    UINT64 ullCycles;                                                   /*  CPU 周期数                  */
    UINT64 ullSecond;                                                   /*  秒数                        */

    if (_G_ullCpuFreq == 0) {
        return 0;
    }

    READ_TSC_LL(ullCycles);

    ullSecond = ullCycles / _G_ullCpuFreq;

    return (ullSecond * NSECS_PER_SEC +
            ((ullCycles % _G_ullCpuFreq) * NSECS_PER_SEC)
            / _G_ullCpuFreq);                                           /*  返回纳秒                    */
}

#endif                                                                  /*  TICK_HIGH_RESOLUTION_TSC > 0*/
/*********************************************************************************************************
** 函数名称: __tickTimerIsr
** 功能描述: tick 定时器中断服务程序
** 输  入  : NONE
** 输  出  : 中断服务返回
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  __tickTimerIsr (VOID)
{
    API_KernelTicksContext();                                           /*  保存被时钟中断的线程控制块  */

#if TICK_IN_THREAD > 0
    API_ThreadResume(htKernelTicks);
#else
    API_KernelTicks();                                                  /*  内核 TICKS 通知             */
    API_TimerHTicks();                                                  /*  高速 TIMER TICKS 通知       */
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: bspTickInit
** 功能描述: 初始化 tick 时钟
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspTickInit (VOID)
{
#if TICK_IN_THREAD > 0
    LW_CLASS_THREADATTR  threadattr;
#endif
    ULONG                ulVector;

#if TICK_IN_THREAD > 0
    API_ThreadAttrBuild(&threadattr, (8 * LW_CFG_KB_SIZE),
                        LW_PRIO_T_TICK,
                        LW_OPTION_THREAD_STK_CHK |
                        LW_OPTION_THREAD_UNSELECT |
                        LW_OPTION_OBJECT_GLOBAL |
                        LW_OPTION_THREAD_SAFE, LW_NULL);

    htKernelTicks = API_ThreadCreate("t_tick", (PTHREAD_START_ROUTINE)__tickThread,
                                     &threadattr, LW_NULL);
#endif                                                                  /*  TICK_IN_THREAD > 0          */

#if TICK_HIGH_RESOLUTION_TSC > 0
    bspTscInit();
#endif

    _G_uiFullCnt          = (_G_i8254Data.qcofreq / LW_TICK_HZ);
    _G_ui64NSecPerCnt7    = ((1000 * 1000 * 1000 / LW_TICK_HZ) << 7) / _G_uiFullCnt;

    i8254InitAsTick(&_G_i8254Data);

    if (_G_bX86HasAPIC && TICK_HIGH_RESOLUTION_TSC) {
        ulVector = X86_IRQ_SLAVE_PIC;                                   /*  IRQ2                        */
    } else {
        ulVector = X86_IRQ_TIMER;                                       /*  IRQ0                        */
    }

    API_InterVectorConnect(ulVector,
                           (PINT_SVR_ROUTINE)__tickTimerIsr,
                           LW_NULL,
                           "tick_timer");

    API_InterVectorEnable(ulVector);
}
/*********************************************************************************************************
** 函数名称: bspTickHook
** 功能描述: 每个操作系统时钟节拍，系统将调用这个函数
** 输  入  : i64Tick      系统当前时钟
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTickHook (INT64  i64Tick)
{
#if TICK_HIGH_RESOLUTION_TSC > 0
    INT64  llCurTickUseNs;                                              /*  本 Tick 走过的 TSC 时间     */

    if (unlikely(_G_ullPreTickTscNs == 0)) {                            /*  第一个 Tick 中断            */
        _G_ullPreTickTscNs = bspTscGetNsec();
        _G_llTscDiffNs     = 0;
    
	} else {
        llCurTickUseNs      = bspTscGetNsec() - _G_ullPreTickTscNs;
        _G_llTscDiffNs     += llCurTickUseNs - LW_NSEC_PER_TICK;        /*  汇总所有 Tick 产生的偏移    */
        _G_ullPreTickTscNs += llCurTickUseNs;

        TOD_TSCDIFF(&_K_tvTODCurrent,                                   /*  校正 _K_tvTODCurrent 偏差   */
                    (llCurTickUseNs - LW_NSEC_PER_TICK));
    }

#else
    /*
     * 定时器中断需要在更新 TICK 后立即被清除.
     */
    i8259aIrqEoi(&_G_i8259aData, X86_IRQ_TIMER);
#endif                                                                  /*  TICK_HIGH_RESOLUTION_TSC    */
}
/*********************************************************************************************************
** 函数名称: bspTickHighResolution
** 功能描述: 修正从最近一次 tick 到当前的精确时间.
** 输　入  : ptv       需要修正的时间
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspTickHighResolution (struct timespec  *ptv)
{
#if TICK_HIGH_RESOLUTION_TSC > 0
    INT64 llNs = ptv->tv_nsec;                                          /*  ns 使用 INT64 类型，因为下  */
                                                                        /*  面运算有可能超出 long 范围  */
    if ((ptv->tv_sec  != _K_tvTODCurrent.tv_sec) ||                     /*  _K_tvTODCurrent 偏差已在每  */
        (ptv->tv_nsec != _K_tvTODCurrent.tv_nsec)) {                    /*  个 Tick 中被校正了          */
        ptv->tv_sec += (_G_llTscDiffNs / NSECS_PER_SEC);                /*  Tick 时间校正到 TSC 时间    */
        llNs        += (_G_llTscDiffNs % NSECS_PER_SEC);
    }

    llNs += (bspTscGetNsec() - _G_ullPreTickTscNs);                     /*  加上 Tick 后 TSC 走过的时间 */
    while (llNs >= NSECS_PER_SEC) {
        llNs -= NSECS_PER_SEC;
        ptv->tv_sec++;
    }
    while (llNs < 0) {
        llNs += NSECS_PER_SEC;
        ptv->tv_sec--;
    }
    ptv->tv_nsec = llNs;

#else
    REGISTER UINT32  uiCntCur, uiDone;

    uiCntCur = (UINT32)i8254GetCnt(&_G_i8254Data);
    uiDone   = _G_uiFullCnt - uiCntCur;

    /*
     *  检查是否有 TICK 中断请求
     */
    if (i8259aIrqIsPending(&_G_i8259aData, X86_IRQ_TIMER)) {
        /*
         *  这里由于 TICK 没有及时更新, 所以需要重新获取并且加上一个 TICK 的时间
         */
        uiCntCur = (UINT32)i8254GetCnt(&_G_i8254Data);
        uiDone   = _G_uiFullCnt - uiCntCur;

        if (uiCntCur != 0) {
            uiDone   += _G_uiFullCnt;
        }
    }

    ptv->tv_nsec += (LONG)((_G_ui64NSecPerCnt7 * uiDone) >> 7);
    if (ptv->tv_nsec >= 1000000000) {
        ptv->tv_nsec -= 1000000000;
        ptv->tv_sec++;
    }
#endif                                                                  /*  TICK_HIGH_RESOLUTION_TSC > 0*/
}
/*********************************************************************************************************
** 函数名称: bspSysBusClkGet
** 功能描述: 获得系统总线时钟
** 输  入  : NONE
** 输  出  : 系统总线时钟
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG   bspSysBusClkGet (VOID)
{
    X86_PARAM   *pparam;

    pparam = archKernelParamGet();

    return  (pparam->X86_ulSysBusClk);
}
/*********************************************************************************************************
  电源相关接口
*********************************************************************************************************/
#define KBD_DATAP           0x60                                        /*  kbd data port               */
#define KBD_STATP           0x64                                        /*  kbd controller status port  */

#define KBD_KDIB            0x01                                        /*  kbd data in buffer          */
#define KBD_UDIB            0x02                                        /*  user data in buffer         */

#define KBRD_RESET          0xFE                                        /*  reset CPU command           */
/*********************************************************************************************************
** 函数名称: bspResetByKeyboard
** 功能描述: 通过键盘重新启动
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  bspResetByKeyboard (VOID)
{
    INT     i;
    UINT8   ucTemp;

    /*
     * Clear all keyboard buffers (output and command buffers)
     */
    do {
        ucTemp = in8(KBD_STATP);
        if (ucTemp & KBD_KDIB) {                                        /*  Empty keyboard data         */
            in8(KBD_DATAP);
        }
    } while (ucTemp & KBD_UDIB);                                        /*  Empty user data             */

    for (i = 0; i < 10; i++) {
        out8(KBRD_RESET, KBD_STATP);                                    /*  Pulse CPU reset line        */
        bspDelayUs(50);
    }

    while (1) {
        X86_HLT();
    }
}
/*********************************************************************************************************
** 函数名称: bspReboot
** 功能描述: 系统重新启动
** 输　入  : iRebootType       重启类型
**           ulStartAddress    启动开始地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspReboot (INT  iRebootType, addr_t  ulStartAddress)
{
    (VOID)iRebootType;
    (VOID)ulStartAddress;

    if (AcpiAvailable() == ACPI_AVAILABLE) {                            /*  ACPI 可用                   */
        if (iRebootType == LW_REBOOT_SHUTDOWN) {                        /*  关机的重启类型              */
            AcpiEnterSleepStatePrep(ACPI_STATE_S5);
            AcpiEnterSleepState(ACPI_STATE_S5);                         /*  连电源在内的所有设备全部关闭*/

        } else {
            AcpiReset();                                                /*  ACPI 重启                   */
        }
        bspResetByKeyboard();                                           /*  上面失败了，使用键盘重启    */

    } else {
        bspResetByKeyboard();                                           /*  ACPI 不可用，使用键盘重启   */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
