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
** ��   ��   ��: bspLib.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 7 �� 31 ��
**
** ��        ��: ��������ҪΪ SylixOS �ṩ�Ĺ���֧��.
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
  BSP ��Ϣ
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: bspInfoCpu
** ��������: BSP CPU ��Ϣ
** �䡡��  : NONE
** �䡡��  : CPU ��Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC CPCHAR  bspInfoCpu (VOID)
{
    return  (_G_pcX86CpuInfo);
}
/*********************************************************************************************************
** ��������: bspInfoCache
** ��������: BSP CACHE ��Ϣ
** �䡡��  : NONE
** �䡡��  : CACHE ��Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC CPCHAR  bspInfoCache (VOID)
{
    return  (_G_pcX86CacheInfo);
}
/*********************************************************************************************************
** ��������: bspInfoHwcap
** ��������: BSP Ӳ������
** �䡡��  : NONE
** �䡡��  : Ӳ������ (���֧��Ӳ����, ���Լ��� HWCAP_FPU, HWCAP_MMX, HWCAP_SSE, HWCAP_SSE2, HWCAP_AVX)
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC ULONG  bspInfoHwcap (VOID)
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
  �ж����

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
  i8259A ƽ̨����
*********************************************************************************************************/
static I8259A_CTL _G_i8259aData = {
    .iobase_master  = X86_8259A_MASTER_IO_BASE,
    .iobase_slave   = X86_8259A_SLAVE_IO_BASE,
    .trigger        = 0,
    .vector_base    = 32,
};
/*********************************************************************************************************
** ��������: bspIntInit
** ��������: �ж�ϵͳ��ʼ��
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID  bspIntInit (VOID)
{
    /*
     * ���ĳ�ж�Ϊ��ʽ�жϣ����������:
     * API_InterVectorSetFlag(LW_IRQ_4, LW_IRQ_FLAG_QUEUE);
     * �Ĵ���.
     *
     * ���ĳ�жϿ�������ʼ����������ӣ����������:
     * API_InterVectorSetFlag(LW_IRQ_0, LW_IRQ_FLAG_SAMPLE_RAND);
     * �Ĵ���.
     */
    i8259aInit(&_G_i8259aData);                                         /*  ��ʼ�� 8259A                */
    i8259aIrqEnable(&_G_i8259aData, LW_IRQ_2);                          /*  ʹ�� IRQ2                   */

    if (_G_bX86HasAPIC) {
        /*
         * 8 �� PCI �ж�����Ϊ��ʽ�ж�
         */
        API_InterVectorSetFlag(LW_IRQ_16, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_17, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_18, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_19, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_20, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_21, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_22, LW_IRQ_FLAG_QUEUE);
        API_InterVectorSetFlag(LW_IRQ_23, LW_IRQ_FLAG_QUEUE);

        x86LocalApicInit();                                             /*  ��ʼ�� Local APIC           */
        x86IoApicInit();                                                /*  ��ʼ�� IO APIC              */
    }
}
/*********************************************************************************************************
  ����ʹ���������
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slVectorTable)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slVectorTable)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: bspIntHandle
** ��������: �жϴ�����
** ��  ��  : ulVector     �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID  bspIntHandle (ULONG  ulVector)
{
    archIntHandle(ulVector, LW_FALSE);                                  /*  �������ж�Ƕ��(MSI �ж��޷� */
                                                                        /*  ����)                       */
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer ��������ģʽ          */
        VECTOR_OP_LOCK();
        x86IoApicIrqEoi(ulVector);                                      /*  SylixOS vector �� ioapic irq*/
        VECTOR_OP_UNLOCK();                                             /*  һ��                        */

    } else {
        /*
         * 8259A ������Ϊ�Զ� EOI
         */
    }
}
/*********************************************************************************************************
** ��������: bspIntVectorEnable
** ��������: ʹ��ָ�����ж�����
** ��  ��  : ulVector     �ж�����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID  bspIntVectorEnable (ULONG  ulVector)
{
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer ��������ģʽ          */
        x86IoApicIrqEnable(ulVector);
        x86LocalApicIrqEnable(ulVector);

    } else {
        if (ulVector < X86_8259A_VECTOR_NR) {
            i8259aIrqEnable(&_G_i8259aData, ulVector);
        }
    }
}
/*********************************************************************************************************
** ��������: bspIntVectorDisable
** ��������: ����ָ�����ж�����
** ��  ��  : ulVector     �ж�����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID  bspIntVectorDisable (ULONG  ulVector)
{
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer ��������ģʽ          */
        x86LocalApicIrqDisable(ulVector);
        x86IoApicIrqDisable(ulVector);

    } else {
        if (ulVector < X86_8259A_VECTOR_NR) {
            i8259aIrqDisable(&_G_i8259aData, ulVector);
        }
    }
}
/*********************************************************************************************************
** ��������: bspIntVectorIsEnable
** ��������: ���ָ�����ж������Ƿ�ʹ��
** ��  ��  : ulVector     �ж�����
** ��  ��  : LW_FALSE �� LW_TRUE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC BOOL  bspIntVectorIsEnable (ULONG  ulVector)
{
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer ��������ģʽ          */
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
** ��������: bspIntVectorSetPriority
** ��������: ����ָ�����ж����������ȼ�
** ��  ��  : ulVector     �ж�����
**           uiPrio       ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_INTER_PRIO > 0

WEAK_FUNC ULONG   bspIntVectorSetPriority (ULONG  ulVector, UINT  uiPrio)
{
    /*
     * �����������ȼ�
     */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: bspIntVectorGetPriority
** ��������: ��ȡָ�����ж����������ȼ�
** ��  ��  : ulVector     �ж�����
**           puiPrio      ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC ULONG   bspIntVectorGetPriority (ULONG  ulVector, UINT  *puiPrio)
{
    *puiPrio = ulVector;                                                /*  Vector Խ�����ȼ�Խ��     */
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
** ��������: bspIntVectorSetTarget
** ��������: ����ָ�����ж�������Ŀ�� CPU
** �䡡��  : ulVector      �ж�����
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_INTER_TARGET > 0

WEAK_FUNC ULONG   bspIntVectorSetTarget (ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer ��������ģʽ          */
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
** ��������: bspIntVectorGetTarget
** ��������: ��ȡָ�����ж�������Ŀ�� CPU
** �䡡��  : ulVector      �ж�����
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC ULONG   bspIntVectorGetTarget (ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    LW_CPU_ZERO(pcpuset);

    if (_G_bX86HasAPIC && (ulVector != X86_IRQ_TIMER)) {                /*  Timer ��������ģʽ          */
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
  MMU ��ؽӿ�
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: bspMmuPgdMaxNum
** ��������: ��� PGD �ص�����
** ��  ��  : NONE
** ��  ��  : PGD �ص����� (1 ���ؿ�ӳ�� 4GB �ռ�, �Ƽ����� 1)
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC ULONG  bspMmuPgdMaxNum (VOID)
{
    return  (1);
}
/*********************************************************************************************************
** ��������: bspMmuPgdMaxNum
** ��������: ��� PTE �ص�����
** ��  ��  : NONE
** ��  ��  : PTE �ص����� (ӳ�� 4GB �ռ�, ��Ҫ 4096 �� PTE ��)
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC ULONG  bspMmuPteMaxNum (VOID)
{
    return  (4096);
}
/*********************************************************************************************************
  �����ؽӿ�
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define X86_AP_BOOT_STACK_SIZE      (32 * LW_CFG_KB_SIZE)
#define X86_AP_BOOT_ENTRY_ADDR      (0x7000)
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern PLW_STACK    _K_pstkInterruptBase[LW_CFG_MAX_PROCESSORS];        /*  �жϴ���ʱ�Ķ�ջ����ַ      */
/*********************************************************************************************************
  AP ��������
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
  AP ������
*********************************************************************************************************/
static SPINLOCKTYPE _G_slX86ApLock = 0;
/*********************************************************************************************************
** ��������: bspCpuUp
** ��������: ����һ�� CPU
** ��  ��  : ulCPUId      Ŀ�� CPU
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID   bspCpuUp (ULONG  ulCPUId)
{
    UINT32     *puiEntryAddr;
    PLW_STACK   pstkStack;

    if (_G_iX86ProcNr <= 1) {
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
** ��������: bspCpuUpDone
** ��������: һ�� CPU �������
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID   bspCpuUpDone (VOID)
{
    x86SpinUnlock(&_G_slX86ApLock);
}
/*********************************************************************************************************
** ��������: bspSecondaryCpusUp
** ��������: �������е� Secondary CPU
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID  bspSecondaryCpusUp (VOID)
{
    UINT8  ucLocalApicId;
    INT    i;

    if (_G_iX86ProcNr <= 1) {
        return;
    }

    ucLocalApicId = x86LocalApicId();

    for (i = 0; i < (2 * LW_CFG_MAX_PROCESSORS); i++) {
        if (_G_x86ProcInfo[i].PROC_bPresent) {
            if (ucLocalApicId != _G_x86ProcInfo[i].PROC_ucLocalApicId) {
                API_CpuUp(X86_APICID_TO_CPUID(i));
            }
        }
    }
}
/*********************************************************************************************************
** ��������: bspCpuDown
** ��������: ֹͣһ�� CPU
** ��  ��  : ulCPUId      Ŀ�� CPU
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

WEAK_FUNC VOID   bspCpuDown (ULONG  ulCPUId)
{
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: bspCpuIpiVectorInstall
** ��������: ��װ IPI ����
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID  bspCpuIpiVectorInstall (VOID)
{
    ULONG  ulCPUId;

    ulCPUId = archMpCur();

    API_InterVectorIpi(ulCPUId, 32 + ulCPUId);                          /*  SylixOS IPI ������ 32 ��ʼ  */
}
/*********************************************************************************************************
  ����ϵͳʱ����غ���
*********************************************************************************************************/
/*********************************************************************************************************
  8254 TIMER
*********************************************************************************************************/
#define X86_8254_IO_BASE            0x40
#define X86_8254_BUZZER_IO_BASE     0x61                                /*  8255 buzzer                 */
#define X86_8254_MAX_FREQ           1193182
/*********************************************************************************************************
  TICK �����������
*********************************************************************************************************/
#define TICK_IN_THREAD  0

#if TICK_IN_THREAD > 0
static LW_HANDLE    htKernelTicks;                                      /*  ����ϵͳʱ�ӷ����߳̾��    */
#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
  i8254 ƽ̨����
*********************************************************************************************************/
static I8254_CTL _G_i8254Data = {
    .iobase   = X86_8254_IO_BASE,
    .iobuzzer = X86_8254_BUZZER_IO_BASE,
    .qcofreq  = X86_8254_MAX_FREQ,
};
/*********************************************************************************************************
  ��ȷʱ�任�����
*********************************************************************************************************/
static UINT32   _G_uiFullCnt;
static UINT64   _G_ui64NSecPerCnt7;                                     /*  ��� 7bit ����              */
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
        API_ThreadSuspend(htKernelTicks);
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
    API_ThreadResume(htKernelTicks);
#else
    API_KernelTicks();                                                  /*  �ں� TICKS ֪ͨ             */
    API_TimerHTicks();                                                  /*  ���� TIMER TICKS ֪ͨ       */
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: bspTickInit
** ��������: ��ʼ�� tick ʱ��
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID  bspTickInit (VOID)
{
#if TICK_IN_THREAD > 0
    LW_CLASS_THREADATTR  threakattr;
#endif
    ULONG                ulVector = X86_IRQ_TIMER;

#if TICK_IN_THREAD > 0
    API_ThreadAttrBuild(&threakattr, (8 * LW_CFG_KB_SIZE),
                        LW_PRIO_T_TICK,
                        LW_OPTION_THREAD_STK_CHK |
                        LW_OPTION_THREAD_UNSELECT |
                        LW_OPTION_OBJECT_GLOBAL |
                        LW_OPTION_THREAD_SAFE, LW_NULL);

    htKernelTicks = API_ThreadCreate("t_tick", (PTHREAD_START_ROUTINE)__tickThread,
                                     &threakattr, LW_NULL);
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    _G_uiFullCnt          = (_G_i8254Data.qcofreq / LW_TICK_HZ);
    _G_ui64NSecPerCnt7    = ((1000 * 1000 * 1000 / LW_TICK_HZ) << 7) / _G_uiFullCnt;

    i8254InitAsTick(&_G_i8254Data);

    API_InterVectorConnect(ulVector,
                           (PINT_SVR_ROUTINE)__tickTimerIsr,
                           LW_NULL,
                           "tick_timer");

    API_InterVectorEnable(ulVector);
}
/*********************************************************************************************************
** ��������: bspTickHighResolution
** ��������: ���������һ�� tick ����ǰ�ľ�ȷʱ��.
** �䡡��  : ptv       ��Ҫ������ʱ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID  bspTickHighResolution (struct timespec  *ptv)
{
    REGISTER UINT32  uiCntCur, uiDone;

    uiCntCur = (UINT32)i8254GetCnt(&_G_i8254Data);
    uiDone   = _G_uiFullCnt - uiCntCur;

    /*
     *  ����Ƿ��� TICK �ж�����
     */
    if (i8259aIrqIsPending(&_G_i8259aData, X86_IRQ_TIMER)) {
        /*
         *  �������� TICK û�м�ʱ����, ������Ҫ���»�ȡ���Ҽ���һ�� TICK ��ʱ��
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
}
/*********************************************************************************************************
** ��������: bspSysBusClkGet
** ��������: ���ϵͳ����ʱ��
** ��  ��  : NONE
** ��  ��  : ϵͳ����ʱ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC ULONG   bspSysBusClkGet (VOID)
{
    X86_PARAM   *pparam;

    pparam = archKernelParamGet();

    return  (pparam->X86_ulSysBusClk);
}
/*********************************************************************************************************
  ��Դ��ؽӿ�
*********************************************************************************************************/
#define KBD_DATAP           0x60                                        /*  kbd data port               */
#define KBD_STATP           0x64                                        /*  kbd controller status port  */

#define KBD_KDIB            0x01                                        /*  kbd data in buffer          */
#define KBD_UDIB            0x02                                        /*  user data in buffer         */

#define KBRD_RESET          0xFE                                        /*  reset CPU command           */
/*********************************************************************************************************
** ��������: bspResetByKeyboard
** ��������: ͨ��������������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: bspReboot
** ��������: ϵͳ��������
** �䡡��  : iRebootType       ��������
**           ulStartAddress    ������ʼ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
WEAK_FUNC VOID    bspReboot (INT  iRebootType, addr_t  ulStartAddress)
{
    (VOID)iRebootType;
    (VOID)ulStartAddress;

    if (AcpiAvailable() == ACPI_AVAILABLE) {                            /*  ACPI ����                   */
        if (iRebootType == LW_REBOOT_SHUTDOWN) {                        /*  �ػ�����������              */
            AcpiEnterSleepStatePrep(ACPI_STATE_S5);
            AcpiEnterSleepState(ACPI_STATE_S5);                         /*  ����Դ���ڵ������豸ȫ���ر�*/

        } else {
            AcpiReset();                                                /*  ACPI ����                   */
        }
        bspResetByKeyboard();                                           /*  ����ʧ���ˣ�ʹ�ü�������    */

    } else {
        bspResetByKeyboard();                                           /*  ACPI �����ã�ʹ�ü�������   */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
