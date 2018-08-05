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
** ��   ��   ��: armExcV7M.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 14 ��
**
** ��        ��: ARMv7M ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
#include "armSvcV7M.h"
/*********************************************************************************************************
  ����ʹ���������
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: archIntHandle
** ��������: bspIntHandle ��Ҫ���ô˺��������ж� (�ر��ж����������)
** �䡡��  : ulVector         �ж�����
**           bPreemptive      �ж��Ƿ����ռ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
LW_WEAK VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t irqret;

    if (_Inter_Vector_Invalid(ulVector)) {
        return;                                                         /*  �����Ų���ȷ                */
    }

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_PREEMPTIVE) {
        bPreemptive = LW_TRUE;
    }

    if (bPreemptive) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  ���� vector �ж�            */
        VECTOR_OP_UNLOCK();
        KN_INT_ENABLE_FORCE();                                          /*  �����ж�                    */
    }

    irqret = API_InterVectorIsr(ulVector);                              /*  �����жϷ������            */

    KN_INT_DISABLE();                                                   /*  �����ж�                    */

    if (bPreemptive) {
        if (irqret != LW_IRQ_HANDLED_DISV) {
            VECTOR_OP_LOCK();
            __ARCH_INT_VECTOR_ENABLE(ulVector);                         /*  ���� vector �ж�            */
            VECTOR_OP_UNLOCK();
        }

    } else if (irqret == LW_IRQ_HANDLED_DISV) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  ���� vector �ж�            */
        VECTOR_OP_UNLOCK();
    }
}
/*********************************************************************************************************
  Structure type to access the System Control Block (SCB)
*********************************************************************************************************/
typedef struct {
    UINT32      CPUID;          /*  0x000 (R/ )  CPUID Base Register                                    */
    UINT32      ICSR;           /*  0x004 (R/W)  Interrupt Control and State Register                   */
    UINT32      VTOR;           /*  0x008 (R/W)  Vector Table Offset Register                           */
    UINT32      AIRCR;          /*  0x00C (R/W)  Application Interrupt and Reset Control Register       */
    UINT32      SCR;            /*  0x010 (R/W)  System Control Register                                */
    UINT32      CCR;            /*  0x014 (R/W)  Configuration Control Register                         */
    UINT8       SHP[12];        /*  0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15)  */
    UINT32      SHCSR;          /*  0x024 (R/W)  System Handler Control and State Register              */
    UINT32      CFSR;           /*  0x028 (R/W)  Configurable Fault Status Register                     */
    UINT32      HFSR;           /*  0x02C (R/W)  HardFault Status Register                              */
    UINT32      DFSR;           /*  0x030 (R/W)  Debug Fault Status Register                            */
    UINT32      MMFAR;          /*  0x034 (R/W)  MemManage Fault Address Register                       */
    UINT32      BFAR;           /*  0x038 (R/W)  BusFault Address Register                              */
    UINT32      AFSR;           /*  0x03C (R/W)  Auxiliary Fault Status Register                        */
    UINT32      PFR[2];         /*  0x040 (R/ )  Processor Feature Register                             */
    UINT32      DFR;            /*  0x048 (R/ )  Debug Feature Register                                 */
    UINT32      ADR;            /*  0x04C (R/ )  Auxiliary Feature Register                             */
    UINT32      MMFR[4];        /*  0x050 (R/ )  Memory Model Feature Register                          */
    UINT32      ISAR[5];        /*  0x060 (R/ )  Instruction Set Attributes Register                    */
    UINT32      RESERVED0[5];
    UINT32      CPACR;          /*  0x088 (R/W)  Coprocessor Access Control Register                    */
} SCB_Type;
/*********************************************************************************************************
  Memory mapping of Cortex-M Hardware
*********************************************************************************************************/
#define SCS_BASE            (0xe000e000)                                /*  System Control Space Address*/
#define SCB_BASE            (SCS_BASE + 0x0d00)                         /*  System Control Block Address*/
#define SCB                 ((SCB_Type *)SCB_BASE)
/*********************************************************************************************************
  �������Ͷ���
*********************************************************************************************************/
typedef enum {
    UNALIGN_TRP = 0x00000008,
    DIV_0_TRP   = 0x00000010,
} CONFIG_CTRL_BITS;

typedef enum {
    BUSFAULTENA = 0x00020000,
    USGFAULTENA = 0x00040000,
} SYS_HANDLER_CSR_BITS;

typedef enum {
    IBUSERR     = 0x00000100,
    PRECISERR   = 0x00000200,
    IMPRECISERR = 0x00000400,
    UNSTKERR    = 0x00000800,
    STKERR      = 0x00001000,
    BFARVALID   = 0x00008000,
    UNDEFINSTR  = 0x00010000,
    INVSTATE    = 0x00020000,
    INVPC       = 0x00040000,
    NOCP        = 0x00080000,
    UNALIGNED   = 0x01000000,
    DIVBYZERO   = 0x02000000,
} LOCAL_FAULT_STATUS_BITS;

typedef enum {
    VECTTBL     = 0x00000002,
    FORCED      = 0x40000000,
} HARD_FAULT_STATUS_BITS;

typedef enum {
    HARDFAULT   = 3,
    BUSFAULT    = 5,
    USAGEFAULT  = 6,
} INTERRUPTS;

typedef struct {
    CHAR   *pcName;
    INT     iTestBit;
    INT     iHandler;
} TRAPS;

static TRAPS  _G_traps[] = {
    {"Vector Read error",        VECTTBL,     HARDFAULT},
    {"uCode stack push error",   STKERR,      BUSFAULT},
    {"uCode stack pop error",    UNSTKERR,    BUSFAULT},
    {"Escalated to Hard Fault",  FORCED,      HARDFAULT},
    {"Pre-fetch error",          IBUSERR,     BUSFAULT},
    {"Precise data bus error",   PRECISERR,   BUSFAULT},
    {"Imprecise data bus error", IMPRECISERR, BUSFAULT},
    {"No Coprocessor",           NOCP,        USAGEFAULT},
    {"Undefined Instruction",    UNDEFINSTR,  USAGEFAULT},
    {"Invalid ISA state",        INVSTATE,    USAGEFAULT},
    {"Return to invalid PC",     INVPC,       USAGEFAULT},
    {"Illegal unaligned access", UNALIGNED,   USAGEFAULT},
    {"Divide By 0",              DIVBYZERO,   USAGEFAULT},
    {NULL}
};
/*********************************************************************************************************
** ��������: armv7mTrapsInit
** ��������: The function initializes Bus fault and Usage fault exceptions,
**           forbids unaligned data access and division by 0.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mTrapsInit (VOID)
{
    write32(read32((addr_t)&SCB->SHCSR) | USGFAULTENA | BUSFAULTENA,
            (addr_t)&SCB->SHCSR);

    write32(DIV_0_TRP | read32((addr_t)&SCB->CCR),
            (addr_t)&SCB->CCR);
}
/*********************************************************************************************************
** ��������: armv7mSvcHandle
** ��������: SVC ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mSvcHandle (ARCH_HW_SAVE_REG_CTX  *pHwSaveCtx, ARCH_SW_SAVE_REG_CTX  *pSwSaveCtx)
{
    UINT32          uiCmd = pHwSaveCtx->REG_uiR1;
    PLW_CLASS_CPU   pcpuCur;

    switch (uiCmd) {

    case SVC_archTaskCtxStart:
        pcpuCur = (PLW_CLASS_CPU)pHwSaveCtx->REG_uiR0;
        return  (&pcpuCur->CPU_ptcbTCBCur->TCB_archRegCtx);

    case SVC_archTaskCtxSwitch:
        pcpuCur = (PLW_CLASS_CPU)pHwSaveCtx->REG_uiR0;
        archTaskCtxCopy(&pcpuCur->CPU_ptcbTCBCur->TCB_archRegCtx, pSwSaveCtx, pHwSaveCtx);
        _SchedSwp(pcpuCur);
        return  (&pcpuCur->CPU_ptcbTCBCur->TCB_archRegCtx);

#if LW_CFG_COROUTINE_EN > 0
    case SVC_archCrtCtxSwitch:
        pcpuCur = (PLW_CLASS_CPU)pregctx->REG_uiR0;
        archTaskCtxCopy(&pcpuCur->CPU_pcrcbCur->COROUTINE_archRegCtx, pSwSaveCtx, pHwSaveCtx);
        _SchedCrSwp(pcpuCur);
        return  (&pcpuCur->CPU_pcrcbCur->COROUTINE_archRegCtx));
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

    case SVC_archSigCtxLoad:
        return  ((ARCH_REG_CTX *)pHwSaveCtx->REG_uiR0);

    default:
        _BugHandle(LW_TRUE, LW_TRUE, "unknown SVC command!\r\n");
        break;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: armv7mIntHandle
** ��������: �жϴ���
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    archIntHandle((ULONG)uiVector, LW_FALSE);
}
/*********************************************************************************************************
** ��������: armv7mNMIIntHandle
** ��������: NMI ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  armv7mNMIIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    armv7mIntHandle(uiVector, pregctx);
}
/*********************************************************************************************************
** ��������: armv7mSysTickIntHandle
** ��������: SysTick �жϴ���
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mSysTickIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    armv7mIntHandle(uiVector, pregctx);
}
/*********************************************************************************************************
** ��������: armv7mFaultPrintInfo
** ��������: The function prints information about the reason of the exception
** �䡡��  : in        IPSR, the number of the exception
**           addr      address caused the interrupt, or current pc
**           hstatus   status register for hard fault
**           lstatus   status register for local fault
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armv7mFaultPrintInfo (INTERRUPTS  in,
                                   addr_t      ulAddr,
                                   UINT32      uiHStatus,
                                   UINT32      uiLStatus)
{
    INT  i;

    _PrintFormat("\r\n\r\nfault %d at 0x%08lx [hstatus=0x%08lx, lstatus=0x%08lx]\r\n",
                (INT)in, ulAddr, uiHStatus, uiLStatus);

    for (i = 0; _G_traps[i].pcName != NULL; ++i) {
        if ((_G_traps[i].iHandler == HARDFAULT ? uiHStatus : uiLStatus) & _G_traps[i].iTestBit) {
            _PrintFormat("%s\r\n", _G_traps[i].pcName);
        }
    }
    _PrintFormat("\r\n");
}
/*********************************************************************************************************
** ��������: armv7mFaultCommonHandle
** ��������: Common routine for high-level exception handlers.
** �䡡��  : pregctx   ������
**           ptcbCur   ��ǰ������ƿ�
**           in        �쳣��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armv7mFaultCommonHandle (ARCH_REG_CTX   *pregctx,
                                      PLW_CLASS_TCB   ptcbCur,
                                      INTERRUPTS      in)
{
    LW_VMM_ABORT    abtInfo;
    UINT32          uiHStatus;
    UINT32          uiLStatus;
    addr_t          ulAddr;

    uiHStatus = read32((addr_t)&SCB->HFSR);
    uiLStatus = read32((addr_t)&SCB->CFSR);

    if (uiLStatus & BFARVALID && (in == BUSFAULT ||
        (in == HARDFAULT && uiHStatus & FORCED))) {
        ulAddr = read32((addr_t)&SCB->BFAR);

    } else {
        ulAddr = pregctx->REG_uiPc;
    }

    write32(uiHStatus, (addr_t)&SCB->HFSR);
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    armv7mFaultPrintInfo(in, ulAddr, uiHStatus, uiLStatus);

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: armv7mHardFaultHandle
** ��������: Hard Fault ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mHardFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    armv7mFaultCommonHandle(pregctx, ptcbCur, HARDFAULT);
}
/*********************************************************************************************************
** ��������: armv7mBusFaultHandle
** ��������: Bus Fault ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mBusFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    armv7mFaultCommonHandle(pregctx, ptcbCur, BUSFAULT);
}
/*********************************************************************************************************
** ��������: armv7mUsageFaultHandle
** ��������: Usage Fault ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mUsageFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    armv7mFaultCommonHandle(pregctx, ptcbCur, USAGEFAULT);
}
/*********************************************************************************************************
** ��������: armv7mMemFaultHandle
** ��������: Mem Fault ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mMemFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    UINT32          uiLStatus;
    addr_t          ulAddr;
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    /*
     * Read the fault status register from the MPU hardware
     */
    uiLStatus = read32((addr_t)&SCB->CFSR);

    /*
     * Clean up the memory fault status register for a next exception
     */
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    if ((uiLStatus & 0xf0) == 0x80) {
        /*
         * Did we get a valid address in the memory fault address register?
         * If so, this is a data access failure (can't tell read or write).
         */
        ulAddr = read32((addr_t)&SCB->MMFAR);

        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;

    } else if (uiLStatus & 0x1) {
        /*
         * If there is no valid address, did we get an instuction access
         * failure? If so, this is a code access failure.
         */
        ulAddr = pregctx->REG_uiPc;

        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    } else {
        /*
         * Some error bit set in the memory fault address register.
         * This must be MUNSTKERR due to a stacking error, which
         * implies that we have gone beyond the low stack boundary
         * (or somehow SP got incorrect).
         * There is no recovery from that since no registers
         * were saved on the stack on this exception, that is,
         * we have no PC saved to return to user mode.
         */
        ulAddr = (addr_t)-1;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    }

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#endif

    API_VmmAbortIsr(pregctx->REG_uiPc, ulAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: armv7mDebugMonitorHandle
** ��������: Debug Monitor ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mDebugMonitorHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BREAK;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: armv7mPendSVHandle
** ��������: PendSV ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mPendSVHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_UNDEF;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: armv7mReservedIntHandle
** ��������: Reserved �жϴ���
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mReservedIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
