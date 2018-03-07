/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k_cpu.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 04 �� 07 ��
**
** ��        ��: CPU ��Ϣ����.
**
** BUG:
2013.07.18  ȥ�� cpu ��Ϣ�� ���ȼ��ļ�¼, 1.0.0-rc40 �Ժ�� SylixOS ����ʹ��. 
2015.04.24  �˼��жϼ���ר�е��������.
*********************************************************************************************************/

#ifndef __K_CPU_H
#define __K_CPU_H

/*********************************************************************************************************
  CPU ����״̬ (Ŀǰֻ֧�� ACTIVE ģʽ)
*********************************************************************************************************/

#define LW_CPU_STATUS_ACTIVE            0x80000000                      /*  CPU ����                    */
#define LW_CPU_STATUS_INACTIVE          0x00000000                      /*  CPU δ����                  */

/*********************************************************************************************************
  CPU �ṹ (Ҫ�� CPU ID ��Ŵ� 0 ��ʼ���������������)
*********************************************************************************************************/
#ifdef  __SYLIXOS_KERNEL

typedef struct __lw_cpu {
    /*
     *  �����߳����
     */
    volatile PLW_CLASS_TCB   CPU_ptcbTCBCur;                            /*  ��ǰ TCB                    */
    volatile PLW_CLASS_TCB   CPU_ptcbTCBHigh;                           /*  ��Ҫ���еĸ����� TCB        */
    
#if LW_CFG_COROUTINE_EN > 0
    /*
     *  Э���л���Ϣ
     */
    PLW_CLASS_COROUTINE      CPU_pcrcbCur;                              /*  ��ǰЭ��                    */
    PLW_CLASS_COROUTINE      CPU_pcrcbNext;                             /*  ��Ҫ�л���Ŀ��Э��          */
#else
    PVOID                    CPU_pvNull[2];                             /*  ��֤�����Ա��ַƫ����һ��  */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

    /*
     *  ��ǰ�������ȵĵ��ȷ�ʽ
     */
    BOOL                     CPU_bIsIntSwitch;                          /*  �Ƿ�Ϊ�жϵ���              */

    /*
     *  ��ѡ���нṹ
     */
    volatile LW_CLASS_CAND   CPU_cand;                                  /*  ��ѡ���е��߳�              */

    /*
     *  �ں�����״̬
     */
    volatile INT             CPU_iKernelCounter;                        /*  �ں�״̬������              */

    /*
     *  ��ǰ�˾�����
     */
#if LW_CFG_SMP_EN > 0
    LW_CLASS_PCBBMAP         CPU_pcbbmapReady;                          /*  ��ǰ CPU ������             */
    
    /*
     *  �˼��жϴ������־, ��������� ULONG λ�����˼��ж�����, �� CPU Ӳ���ж�����ԭ����ͬ
     */
    LW_SPINLOCK_DEFINE      (CPU_slIpi);                                /*  �˼��ж���                  */
    PLW_LIST_RING            CPU_pringMsg;                              /*  �Զ���˼��жϲ�����        */
    volatile UINT            CPU_uiMsgCnt;                              /*  �Զ���˼��ж�����          */
    
    ULONG                    CPU_ulIPIVector;                           /*  �˼��ж�����                */
    FUNCPTR                  CPU_pfuncIPIClear;                         /*  �˼��ж��������            */
    PVOID                    CPU_pvIPIArg;                              /*  �˼��ж��������            */
    
    INT64                    CPU_iIPICnt;                               /*  �˼��жϴ���                */
    volatile ULONG           CPU_ulIPIPend;                             /*  �˼��жϱ�־��              */

#define LW_IPI_NOP              0                                       /*  �����ú˼��ж�����          */
#define LW_IPI_SCHED            1                                       /*  ��������                    */
#define LW_IPI_DOWN             2                                       /*  CPU ֹͣ����                */
#define LW_IPI_CALL             3                                       /*  �Զ������ (�в�����ѡ�ȴ�) */

#define LW_IPI_NOP_MSK          (1 << LW_IPI_NOP)
#define LW_IPI_SCHED_MSK        (1 << LW_IPI_SCHED)
#define LW_IPI_DOWN_MSK         (1 << LW_IPI_DOWN)
#define LW_IPI_CALL_MSK         (1 << LW_IPI_CALL)

#ifdef __LW_SPINLOCK_BUG_TRACE_EN
    volatile ULONG           CPU_ulSpinNesting;                         /*  spinlock ��������           */
#endif                                                                  /*  __LW_SPINLOCK_BUG_TRACE_EN  */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
     
    /*
     *  spinlock �ȴ���
     */
    union {
        LW_LIST_LINE         CPUQ_lineSpinlock;                         /*  PRIORITY �ȴ���             */
        LW_LIST_RING         CPUQ_ringSpinlock;                         /*  FIFO �ȴ���                 */
    } CPU_cpuq;

#define CPU_lineSpinlock     CPU_cpuq.CPUQ_lineSpinlock
#define CPU_ringSpinlock     CPU_cpuq.CPUQ_ringSpinlock

    /*
     *  CPU ������Ϣ
     */
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
             ULONG           CPU_ulPhyId;                               /*  Physical CPU Id             */
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT         */
             ULONG           CPU_ulCPUId;                               /*  CPU ID ��                   */
    volatile ULONG           CPU_ulStatus;                              /*  CPU ����״̬                */

    /*
     *  �ж���Ϣ
     */
             PLW_STACK       CPU_pstkInterBase;                         /*  �ж϶�ջ��ַ                */
    volatile ULONG           CPU_ulInterNesting;                        /*  �ж�Ƕ�׼�����              */
    volatile ULONG           CPU_ulInterNestingMax;                     /*  �ж�Ƕ�����ֵ              */
    ULONG                    CPU_ulInterError[LW_CFG_MAX_INTER_SRC];    /*  �жϴ�����Ϣ                */
    
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)
    /*
     *  �ж�ʱʹ�õ� FPU ������. 
     *  ֻ�� LW_KERN_FPU_EN_GET() ��Чʱ�Ž����ж�״̬�� FPU �����Ĳ���.
     */
    LW_FPU_CONTEXT           CPU_fpuctxContext[LW_CFG_MAX_INTER_SRC];   /*  �ж�ʱʹ�õ� FPU ������     */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */

#if (LW_CFG_CPU_DSP_EN > 0) && (LW_CFG_INTER_DSP > 0)
    /*
     *  �ж�ʱʹ�õ� DSP ������.
     *  ֻ�� LW_KERN_DSP_EN_GET() ��Чʱ�Ž����ж�״̬�� DSP �����Ĳ���.
     */
    LW_DSP_CONTEXT           CPU_dspctxContext[LW_CFG_MAX_INTER_SRC];   /*  �ж�ʱʹ�õ� DSP ������     */
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
                                                                        /*  LW_CFG_INTER_DSP > 0        */
} LW_CLASS_CPU;
typedef LW_CLASS_CPU        *PLW_CLASS_CPU;

/*********************************************************************************************************
  Physical CPU (Ҫ������ CPU ID ��Ŵ� 0 ��ʼ���������������)
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)

typedef struct __lw_phycpu {
    UINT                    PHYCPU_uiLogic;                             /*  ��ǰӵ�ж��ٸ������߼� CPU  */
    UINT                    PHYCPU_uiNonIdle;                           /*  ��ǰ���е���Ч��������      */
} LW_CLASS_PHYCPU;
typedef LW_CLASS_PHYCPU    *PLW_CLASS_PHYCPU;

#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  CPU ����
*********************************************************************************************************/

#define LW_CPU_SETSIZE      2048
#define LW_NCPUBITS         (sizeof(ULONG) * 8)                         /*  ÿһ����λ�����λ��        */
#define LW_NCPUULONG        (LW_CPU_SETSIZE / LW_NCPUBITS)

typedef struct {
    ULONG                   cpus_bits[LW_NCPUULONG];
} LW_CLASS_CPUSET;
typedef LW_CLASS_CPUSET    *PLW_CLASS_CPUSET;

#define LW_CPU_SET(n, p)    ((p)->cpus_bits[(n) / LW_NCPUBITS] |= (ULONG)( (1u << ((n) % LW_NCPUBITS))))
#define LW_CPU_CLR(n, p)    ((p)->cpus_bits[(n) / LW_NCPUBITS] &= (ULONG)(~(1u << ((n) % LW_NCPUBITS))))
#define LW_CPU_ISSET(n, p)  ((p)->cpus_bits[(n) / LW_NCPUBITS] &  (ULONG)( (1u << ((n) % LW_NCPUBITS))))
#define LW_CPU_ZERO(p)      lib_bzero((PVOID)(p), sizeof(*(p)))

/*********************************************************************************************************
  ��ǰ CPU ��Ϣ LW_NCPUS �����ܴ��� LW_CFG_MAX_PROCESSORS
*********************************************************************************************************/
#ifdef  __SYLIXOS_KERNEL

#if LW_CFG_SMP_EN > 0
ULONG   archMpCur(VOID);
#define LW_CPU_GET_CUR_ID()  archMpCur()                                /*  ��õ�ǰ CPU ID             */
#define LW_NCPUS             (_K_ulNCpus)
#if LW_CFG_CPU_ARCH_SMT > 0
#define LW_NPHYCPUS          (_K_ulNPhyCpus)
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
#else
#define LW_CPU_GET_CUR_ID()  0
#define LW_NCPUS             1
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  CPU �����
*********************************************************************************************************/

extern LW_CLASS_CPU          _K_cpuTable[];                             /*  CPU ��                      */
#define LW_CPU_GET_CUR()     (&_K_cpuTable[LW_CPU_GET_CUR_ID()])        /*  ��õ�ǰ CPU �ṹ           */
#define LW_CPU_GET(id)       (&_K_cpuTable[(id)])                       /*  ���ָ�� CPU �ṹ           */

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
extern LW_CLASS_PHYCPU       _K_phycpuTable[];                          /*  ���� CPU ��                 */
#define LW_PHYCPU_GET_CUR()  (&_K_phycpuTable[LW_CPU_GET_CUR()->CPU_ulPhyId])
#define LW_PHYCPU_GET(phyid) (&_K_phycpuTable[(phyid)])
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */

/*********************************************************************************************************
  CPU ����
*********************************************************************************************************/

#define LW_CPU_FOREACH(i)                       \
        for (i = 0; i < LW_NCPUS; i++)
        
#define LW_CPU_FOREACH_ACTIVE(i)                \
        for (i = 0; i < LW_NCPUS; i++)          \
        if (LW_CPU_IS_ACTIVE(LW_CPU_GET(i)))
                                    
#define LW_CPU_FOREACH_EXCEPT(i, id)            \
        for (i = 0; i < LW_NCPUS; i++)          \
        if ((i) != (id))
        
#define LW_CPU_FOREACH_ACTIVE_EXCEPT(i, id)     \
        for (i = 0; i < LW_NCPUS; i++)          \
        if ((i) != (id))                        \
        if (LW_CPU_IS_ACTIVE(LW_CPU_GET(i)))
        
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
#define LW_PHYCPU_FOREACH(i)                    \
        for (i = 0; i < LW_NPHYCPUS; i++)
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */

/*********************************************************************************************************
  CPU ������
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
#define LW_CPU_RDY_PCBBMAP(pcpu)        (&(pcpu->CPU_pcbbmapReady))
#define LW_CPU_RDY_BMAP(pcpu)           (&(pcpu->CPU_pcbbmapReady.PCBM_bmap))
#define LW_CPU_RDY_PPCB(pcpu, prio)     (&(pcpu->CPU_pcbbmapReady.PCBM_pcb[prio]))
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  CPU spin nesting
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
#ifdef __LW_SPINLOCK_BUG_TRACE_EN
#define LW_CPU_SPIN_NESTING_GET(pcpu)   ((pcpu)->CPU_ulSpinNesting)
#define LW_CPU_SPIN_NESTING_INC(pcpu)   ((pcpu)->CPU_ulSpinNesting++)
#define LW_CPU_SPIN_NESTING_DEC(pcpu)   ((pcpu)->CPU_ulSpinNesting--)
#else
#define LW_CPU_SPIN_NESTING_GET(pcpu)
#define LW_CPU_SPIN_NESTING_INC(pcpu)
#define LW_CPU_SPIN_NESTING_DEC(pcpu)
#endif                                                                  /*  __LW_SPINLOCK_BUG_TRACE_EN  */
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  CPU ״̬
*********************************************************************************************************/

#define LW_CPU_IS_ACTIVE(pcpu)  \
        ((pcpu)->CPU_ulStatus & LW_CPU_STATUS_ACTIVE)

/*********************************************************************************************************
  CPU �˼��ж�
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

#define LW_CPU_ADD_IPI_PEND(id, ipi_msk)    \
        (_K_cpuTable[(id)].CPU_ulIPIPend |= (ipi_msk))                  /*  ����ָ�� CPU �˼��ж� pend  */
#define LW_CPU_CLR_IPI_PEND(id, ipi_msk)    \
        (_K_cpuTable[(id)].CPU_ulIPIPend &= ~(ipi_msk))                 /*  ���ָ�� CPU �˼��ж� pend  */
#define LW_CPU_GET_IPI_PEND(id)             \
        (_K_cpuTable[(id)].CPU_ulIPIPend)                               /*  ��ȡָ�� CPU �˼��ж� pend  */
        
#define LW_CPU_ADD_IPI_PEND2(pcpu, ipi_msk)    \
        (pcpu->CPU_ulIPIPend |= (ipi_msk))                              /*  ����ָ�� CPU �˼��ж� pend  */
#define LW_CPU_CLR_IPI_PEND2(pcpu, ipi_msk)    \
        (pcpu->CPU_ulIPIPend &= ~(ipi_msk))                             /*  ���ָ�� CPU �˼��ж� pend  */
#define LW_CPU_GET_IPI_PEND2(pcpu)             \
        (pcpu->CPU_ulIPIPend)                                           /*  ��ȡָ�� CPU �˼��ж� pend  */
        
/*********************************************************************************************************
  CPU ������������ж� (���ж�����±�����)
*********************************************************************************************************/

#define LW_CPU_CLR_SCHED_IPI_PEND(pcpu)                             \
        do {                                                        \
            if (LW_CPU_GET_IPI_PEND2(pcpu) & LW_IPI_SCHED_MSK) {    \
                LW_SPIN_LOCK_IGNIRQ(&pcpu->CPU_slIpi);              \
                LW_CPU_CLR_IPI_PEND2(pcpu, LW_IPI_SCHED_MSK);       \
                LW_SPIN_UNLOCK_IGNIRQ(&pcpu->CPU_slIpi);            \
            }                                                       \
            LW_SPINLOCK_NOTIFY();                                   \
        } while (0)

/*********************************************************************************************************
  CPU �˼��ж�����
*********************************************************************************************************/

#define LW_CPU_GET_IPI_CNT(id)          (_K_cpuTable[(id)].CPU_iIPICnt)

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  CPU �ж���Ϣ
*********************************************************************************************************/

#define LW_CPU_GET_NESTING(id)          (_K_cpuTable[(id)].CPU_ulInterNesting)
#define LW_CPU_GET_NESTING_MAX(id)      (_K_cpuTable[(id)].CPU_ulInterNestingMax)

ULONG  _CpuGetNesting(VOID);
ULONG  _CpuGetMaxNesting(VOID);

#define LW_CPU_GET_CUR_NESTING()        _CpuGetNesting()
#define LW_CPU_GET_CUR_NESTING_MAX()    _CpuGetMaxNesting()

#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  __K_CPU_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
