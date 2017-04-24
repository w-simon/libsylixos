/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: bspTime.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 7 �� 31 ��
**
** ��        ��: ��������ҪΪ SylixOS �ṩ�Ĺ���֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86Idt.h"
#include "driver/timer/i8254.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define TICK_IN_THREAD              0
/*********************************************************************************************************
  �궨��
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
  ȫ�ֱ�������
*********************************************************************************************************/
/*********************************************************************************************************
  i8254 ƽ̨����
*********************************************************************************************************/
static I8254_CTL            _G_i8254Data = {
    .iobase   = PIT_BASE_ADR,
    .iobuzzer = DIAG_CTRL,
    .qcofreq  = PC_PIT_CLOCK,
};
/*********************************************************************************************************
  ��ȷʱ�任�����
*********************************************************************************************************/
static UINT32               _G_uiFullCnt;
static UINT64               _G_ui64NSecPerCnt7;                         /*  ��� 7bit ����              */
/*********************************************************************************************************
  ����ƫ��
*********************************************************************************************************/
static          UINT64      _G_ui64CpuFreq;                             /*  ÿ�� CPU ������             */
static volatile UINT64      _G_ui64PreTickTscNs;                        /*  ��һ Tick ����ʱ TSC ʱ��   */
static volatile INT64       _G_i64TscDiffNs;                            /*  TSC ʱ��� Tick ʱ��ƫ��    */
/*********************************************************************************************************
  ����ϵͳʱ�ӷ����߳̾��
*********************************************************************************************************/
#if TICK_IN_THREAD > 0
static LW_HANDLE            _G_htKernelTicks;
#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
** ��������: __tickThread
** ��������: ��ʼ�� tick �����߳�
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if TICK_IN_THREAD > 0

static VOID  __tickThread (VOID)
{
    for (;;) {
        API_ThreadSuspend(_G_htKernelTicks);
        API_KernelTicks();                                              /*  �ں� TICKS ֪ͨ             */
        API_TimerHTicks();                                              /*  ���� TIMER TICKS ֪ͨ       */
    }
}

#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
** ��������: __tickTimerIsr
** ��������: tick ��ʱ���жϷ������
** ��  ��  : NONE
** ��  ��  : �жϷ��񷵻�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  __tickTimerIsr (VOID)
{
    API_KernelTicksContext();                                           /*  ���汻ʱ���жϵ��߳̿��ƿ�  */

#if TICK_IN_THREAD > 0
    API_ThreadResume(_G_htKernelTicks);
#else
    API_KernelTicks();                                                  /*  �ں� TICKS ֪ͨ             */
    API_TimerHTicks();                                                  /*  ���� TIMER TICKS ֪ͨ       */
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: bspTscInit
** ��������: ��ʼ�� tsc ģ�飬��ȡ 1 ���ж��ٸ� CPU ʱ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: bspTscGetNsec
** ��������: ��ÿ������������е�������
** �䡡��  : NONE
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT64  bspTscGetNsec (VOID)
{
    UINT64  ui64Cycles;                                                 /*  CPU ������                  */
    UINT64  ui64Second;                                                 /*  ����                        */

    if (_G_ui64CpuFreq == 0) {
        return  (0);
    }

    READ_TSC_LL(ui64Cycles);

    ui64Second = ui64Cycles / _G_ui64CpuFreq;

    return  (ui64Second * NSECS_PER_SEC +
            ((ui64Cycles % _G_ui64CpuFreq) * NSECS_PER_SEC)
            / _G_ui64CpuFreq);                                          /*  ��������                    */
}
/*********************************************************************************************************
** ��������: bspTickInit
** ��������: ��ʼ�� tick ʱ��
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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

    bspTscInit();                                                       /*  ��ʼ�� tsc ģ��             */

    _G_uiFullCnt       = (_G_i8254Data.qcofreq / LW_TICK_HZ);
    _G_ui64NSecPerCnt7 = ((1000 * 1000 * 1000  / LW_TICK_HZ) << 7) / _G_uiFullCnt;

    i8254InitAsTick(&_G_i8254Data);                                     /*  ��ʼ�� 8254 ��ʱ��          */

    if (bspIntModeGet() == X86_INT_MODE_SYMMETRIC_IO) {
        ulVector = LW_IRQ_2;                                            /*  IRQ2 (8254 Count0 �ж�)     */

    } else {
        ulVector = X86_IRQ_TIMER;                                       /*  IRQ0                        */
    }

    API_InterVectorConnect(ulVector,
                           (PINT_SVR_ROUTINE)__tickTimerIsr,
                           LW_NULL,
                           "tick_timer");                               /*  ���Ӷ�ʱ���жϷ������      */

    API_InterVectorEnable(ulVector);                                    /*  ʹ�ܶ�ʱ���ж�              */
}
/*********************************************************************************************************
** ��������: bspTickHook
** ��������: ÿ������ϵͳʱ�ӽ��ģ�ϵͳ�������������
** ��  ��  : i64Tick      ϵͳ��ǰʱ��
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  bspTickHook (INT64  i64Tick)
{
    INT64  i64CurTickUseNs;                                             /*  �� Tick �߹��� TSC ʱ��     */

    if (unlikely(_G_ui64PreTickTscNs == 0)) {                           /*  ��һ�� Tick �ж�            */
        _G_ui64PreTickTscNs  = bspTscGetNsec();
        _G_i64TscDiffNs      = 0;

    } else {
        i64CurTickUseNs      = bspTscGetNsec() - _G_ui64PreTickTscNs;
        _G_i64TscDiffNs     += i64CurTickUseNs - LW_NSEC_PER_TICK;      /*  �������� Tick ������ƫ��    */
        _G_ui64PreTickTscNs += i64CurTickUseNs;

        TOD_TSCDIFF(&_K_tvTODCurrent,                                   /*  У�� _K_tvTODCurrent ƫ��   */
                    (i64CurTickUseNs - LW_NSEC_PER_TICK));
    }
}
/*********************************************************************************************************
** ��������: bspTickHighResolution
** ��������: ���������һ�� tick ����ǰ�ľ�ȷʱ��.
** �䡡��  : ptv       ��Ҫ������ʱ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspTickHighResolution (struct timespec  *ptv)
{
    INT64  i64Ns = ptv->tv_nsec;                                        /*  ns ʹ�� INT64 ���ͣ���Ϊ��  */
                                                                        /*  �������п��ܳ��� long ��Χ  */
    if ((ptv->tv_sec  != _K_tvTODCurrent.tv_sec) ||                     /*  _K_tvTODCurrent ƫ������ÿ  */
        (ptv->tv_nsec != _K_tvTODCurrent.tv_nsec)) {                    /*  �� Tick �б�У����          */
        ptv->tv_sec   += (_G_i64TscDiffNs / NSECS_PER_SEC);             /*  Tick ʱ��У���� TSC ʱ��    */
        i64Ns         += (_G_i64TscDiffNs % NSECS_PER_SEC);
    }

    i64Ns += (bspTscGetNsec() - _G_ui64PreTickTscNs);                   /*  ���� Tick �� TSC �߹���ʱ�� */
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
