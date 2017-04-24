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
** 文   件   名: bspTime.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 7 月 31 日
**
** 描        述: 处理器需要为 SylixOS 提供的功能支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86Idt.h"
#include "driver/timer/i8254.h"
/*********************************************************************************************************
  配置
*********************************************************************************************************/
#define TICK_IN_THREAD              0
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define READ_TSC_LL(count)          __asm__ __volatile__("rdtsc" : "=A" (count))
#define CALIBRATE_CYCLES            14551
#define CALIBRATE_MULT              82
#define NSECS_PER_SEC               1000000000UL

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
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
/*********************************************************************************************************
  i8254 平台数据
*********************************************************************************************************/
static I8254_CTL            _G_i8254Data = {
    .iobase   = PIT_BASE_ADR,
    .iobuzzer = DIAG_CTRL,
    .qcofreq  = PC_PIT_CLOCK,
};
/*********************************************************************************************************
  精确时间换算参数
*********************************************************************************************************/
static UINT32               _G_uiFullCnt;
static UINT64               _G_ui64NSecPerCnt7;                         /*  提高 7bit 精度              */
/*********************************************************************************************************
  修正偏差
*********************************************************************************************************/
static          UINT64      _G_ui64CpuFreq;                             /*  每秒 CPU 周期数             */
static volatile UINT64      _G_ui64PreTickTscNs;                        /*  上一 Tick 发生时 TSC 时刻   */
static volatile INT64       _G_i64TscDiffNs;                            /*  TSC 时间和 Tick 时间偏差    */
/*********************************************************************************************************
  操作系统时钟服务线程句柄
*********************************************************************************************************/
#if TICK_IN_THREAD > 0
static LW_HANDLE            _G_htKernelTicks;
#endif                                                                  /*  TICK_IN_THREAD > 0          */
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
        API_ThreadSuspend(_G_htKernelTicks);
        API_KernelTicks();                                              /*  内核 TICKS 通知             */
        API_TimerHTicks();                                              /*  高速 TIMER TICKS 通知       */
    }
}

#endif                                                                  /*  TICK_IN_THREAD > 0          */
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
    API_ThreadResume(_G_htKernelTicks);
#else
    API_KernelTicks();                                                  /*  内核 TICKS 通知             */
    API_TimerHTicks();                                                  /*  高速 TIMER TICKS 通知       */
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: bspTscInit
** 功能描述: 初始化 tsc 模块，获取 1 秒有多少个 CPU 时钟周期
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  bspTscInit (VOID)
{
    UINT64  ui64Start;
    UINT64  ui64Delta;
    UINT64  ui64Stop;

    out8((in8(DIAG_CTRL) & ~0x02) | 0x01, DIAG_CTRL);
    out8(0xb0, PIT_MODE);
    out8(CALIBRATE_CYCLES & 0xff, PIT_CH2);
    out8(CALIBRATE_CYCLES >> 8,   PIT_CH2);
    READ_TSC_LL(ui64Start);

    READ_TSC_LL(ui64Delta);
    in8(DIAG_CTRL);
    READ_TSC_LL(ui64Stop);
    ui64Delta = ui64Stop - ui64Delta;

    while ((in8(DIAG_CTRL) & 0x20) == 0) {
    }
    READ_TSC_LL(ui64Stop);

    _G_ui64CpuFreq      = ((ui64Stop - ui64Delta) - ui64Start) * CALIBRATE_MULT;
    _G_ui64PreTickTscNs = 0;
}
/*********************************************************************************************************
** 函数名称: bspTscGetNsec
** 功能描述: 获得开机到现在运行的纳秒数
** 输　入  : NONE
** 输　出  : 纳秒数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT64  bspTscGetNsec (VOID)
{
    UINT64  ui64Cycles;                                                 /*  CPU 周期数                  */
    UINT64  ui64Second;                                                 /*  秒数                        */

    if (_G_ui64CpuFreq == 0) {
        return  (0);
    }

    READ_TSC_LL(ui64Cycles);

    ui64Second = ui64Cycles / _G_ui64CpuFreq;

    return  (ui64Second * NSECS_PER_SEC +
            ((ui64Cycles % _G_ui64CpuFreq) * NSECS_PER_SEC)
            / _G_ui64CpuFreq);                                          /*  返回纳秒                    */
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

    _G_htKernelTicks = API_ThreadCreate("t_tick", (PTHREAD_START_ROUTINE)__tickThread,
                                        &threadattr, LW_NULL);
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    bspTscInit();                                                       /*  初始化 tsc 模块             */

    _G_uiFullCnt       = (_G_i8254Data.qcofreq / LW_TICK_HZ);
    _G_ui64NSecPerCnt7 = ((1000 * 1000 * 1000  / LW_TICK_HZ) << 7) / _G_uiFullCnt;

    i8254InitAsTick(&_G_i8254Data);                                     /*  初始化 8254 定时器          */

    if (bspIntModeGet() == X86_INT_MODE_SYMMETRIC_IO) {
        ulVector = LW_IRQ_2;                                            /*  IRQ2 (8254 Count0 中断)     */

    } else {
        ulVector = X86_IRQ_TIMER;                                       /*  IRQ0                        */
    }

    API_InterVectorConnect(ulVector,
                           (PINT_SVR_ROUTINE)__tickTimerIsr,
                           LW_NULL,
                           "tick_timer");                               /*  连接定时器中断服务程序      */

    API_InterVectorEnable(ulVector);                                    /*  使能定时器中断              */
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
    INT64  i64CurTickUseNs;                                             /*  本 Tick 走过的 TSC 时间     */

    if (unlikely(_G_ui64PreTickTscNs == 0)) {                           /*  第一个 Tick 中断            */
        _G_ui64PreTickTscNs  = bspTscGetNsec();
        _G_i64TscDiffNs      = 0;

    } else {
        i64CurTickUseNs      = bspTscGetNsec() - _G_ui64PreTickTscNs;
        _G_i64TscDiffNs     += i64CurTickUseNs - LW_NSEC_PER_TICK;      /*  汇总所有 Tick 产生的偏移    */
        _G_ui64PreTickTscNs += i64CurTickUseNs;

        TOD_TSCDIFF(&_K_tvTODCurrent,                                   /*  校正 _K_tvTODCurrent 偏差   */
                    (i64CurTickUseNs - LW_NSEC_PER_TICK));
    }
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
    INT64  i64Ns = ptv->tv_nsec;                                        /*  ns 使用 INT64 类型，因为下  */
                                                                        /*  面运算有可能超出 long 范围  */
    if ((ptv->tv_sec  != _K_tvTODCurrent.tv_sec) ||                     /*  _K_tvTODCurrent 偏差已在每  */
        (ptv->tv_nsec != _K_tvTODCurrent.tv_nsec)) {                    /*  个 Tick 中被校正了          */
        ptv->tv_sec   += (_G_i64TscDiffNs / NSECS_PER_SEC);             /*  Tick 时间校正到 TSC 时间    */
        i64Ns         += (_G_i64TscDiffNs % NSECS_PER_SEC);
    }

    i64Ns += (bspTscGetNsec() - _G_ui64PreTickTscNs);                   /*  加上 Tick 后 TSC 走过的时间 */
    while (i64Ns >= NSECS_PER_SEC) {
        i64Ns -= NSECS_PER_SEC;
        ptv->tv_sec++;
    }
    while (i64Ns < 0) {
        i64Ns += NSECS_PER_SEC;
        ptv->tv_sec--;
    }
    ptv->tv_nsec = i64Ns;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
