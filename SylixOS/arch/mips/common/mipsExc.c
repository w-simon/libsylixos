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
** ��   ��   ��: mipsExc.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "arch/mips/common/cp0/mipsCp0.h"
#include "mipsUnaligned.h"
#if LW_CFG_VMM_EN > 0
#include "arch/mips/mm/mmu/mipsMmuCommon.h"
#endif
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
  �ⲿ��������
*********************************************************************************************************/
extern VOID    archTaskCtxPrint(PLW_STACK  pstkTop);
#if LW_CFG_CPU_FPU_EN > 0
extern UINT32  mipsVfp32GetFEXR(VOID);
extern VOID    mipsVfp32ClearFEXR(VOID);
extern int     fpu_emulator_cop1Handler(ARCH_REG_CTX *xcp, ARCH_FPU_CTX *ctx,
                                        int has_fpu, void **fault_addr);
extern int     do_dsemulret(PLW_CLASS_TCB ptcbCur, ARCH_REG_CTX *xcp);
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  FPU ������(���� FPU ģ��)
*********************************************************************************************************/
ARCH_FPU_CTX   _G_mipsFpuCtx[LW_CFG_MAX_PROCESSORS];
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
__attribute__((weak)) VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
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
** ��������: archCacheErrorHandle
** ��������: Cache ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

VOID  archCacheErrorHandle (addr_t  ulRetAddr)
{
    REGISTER UINT32         uiFiled  = 2 * sizeof(UINT32);
    REGISTER UINT32         uiRegVal;
             PLW_CLASS_TCB  ptcbCur;
             LW_VMM_ABORT   abtInfo;

    uiRegVal = mipsCp0ConfigRead();
    mipsCp0ConfigWrite((uiRegVal & ~M_ConfigK0) | MIPS_UNCACHED);

    _PrintFormat("Cache error exception:\r\n");
    _PrintFormat("cp0_error epc == %lx\r\n", uiFiled, mipsCp0ERRPCRead());

    uiRegVal = mipsCp0CacheErrRead();

    _PrintFormat("cp0_cache error == 0x%08x\r\n", uiRegVal);
    _PrintFormat("Decoded cp0_cache error: %s cache fault in %s reference.\r\n",
                 (uiRegVal & M_CacheLevel) ? "secondary" : "primary",
                 (uiRegVal & M_CacheType)  ? "d-cache"   : "i-cache");

    _PrintFormat("Error bits: %s%s%s%s%s%s%s\r\n",
                 (uiRegVal & M_CacheData) ? "ED " : "",
                 (uiRegVal & M_CacheTag)  ? "ET " : "",
                 (uiRegVal & M_CacheECC)  ? "EE " : "",
                 (uiRegVal & M_CacheBoth) ? "EB " : "",
                 (uiRegVal & M_CacheEI)   ? "EI " : "",
                 (uiRegVal & M_CacheE1)   ? "E1 " : "",
                 (uiRegVal & M_CacheE0)   ? "E0 " : "");

    _PrintFormat("IDX: 0x%08x\r\n", uiRegVal & (M_CacheE0 - 1));

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: archExceptionHandle
** ��������: ͨ���쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archExceptionHandle (addr_t  ulRetAddr, UINT32  uiCause, addr_t  ulAbortAddr, UINT32  uiSp)
{
    REGISTER UINT32         uiExcCode = ((uiCause & M_CauseExcCode) >> S_CauseExcCode);
#if LW_CFG_GDB_EN > 0
    REGISTER UINT           uiBpType;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
             PLW_CLASS_TCB  ptcbCur;
             LW_VMM_ABORT   abtInfo;
             ULONG          ulNesting;
             UINT32         uiFEXR;

    if (uiExcCode == EX_INT) {                                          /*  �Ż���ͨ�жϴ���            */
        bspIntHandle();
        return;
    }

    ulNesting = LW_CPU_GET_CUR_NESTING();
    if (ulNesting > 1) {                                                /*  �쳣����Ƕ��                */
        archTaskCtxPrint((PLW_STACK)uiSp);                              /*  ֱ�Ӵ�ӡ�Ĵ���              */
    }

    LW_TCB_GET_CUR(ptcbCur);

    switch (uiExcCode) {

#if LW_CFG_VMM_EN > 0
    case EX_MOD:                                                        /*  TLB modified                */
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    case EX_TLBL:                                                       /*  TLB exc(load or ifetch)     */
    case EX_TLBS:                                                       /*  TLB exception (store)       */
        abtInfo.VMABT_uiMethod = (uiExcCode == EX_TLBL)
                               ? LW_VMM_ABORT_METHOD_READ
                               : LW_VMM_ABORT_METHOD_WRITE;
        abtInfo.VMABT_uiType   = mipsMmuTlbLoadStoreExcHandle(ulAbortAddr);
        if (abtInfo.VMABT_uiType) {
            API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        }
        break;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    case EX_ADEL:                                                       /*  Address err(load or ifetch) */
    case EX_ADES:                                                       /*  Address error (store)       */
        abtInfo.VMABT_uiMethod = (uiExcCode == EX_ADEL)
                               ? LW_VMM_ABORT_METHOD_READ
                               : LW_VMM_ABORT_METHOD_WRITE;
        abtInfo.VMABT_uiType   = mipsUnalignedHandle((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                                     ulAbortAddr);
        if (abtInfo.VMABT_uiType) {
            API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        }
        break;

    case EX_IBE:                                                        /*  Instruction Bus Error       */
    case EX_DBE:                                                        /*  Data Bus Error              */
        abtInfo.VMABT_uiMethod = BUS_ADRERR;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    case EX_SYS:                                                        /*  Syscall                     */
        abtInfo.VMABT_uiMethod = 0;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    case EX_BP:                                                         /*  Breakpoint                  */
    case EX_TR:                                                         /*  Trap instruction            */
#if LW_CFG_CPU_FPU_EN > 0
        if (*(MIPS_INSTRUCTION *)ulRetAddr == BRK_MEMU) {
            if (do_dsemulret(ptcbCur, (ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow) == ERROR_NONE) {
                break;                                                  /*  FPU ģ�ⷵ��                */
            }
        }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_GDB_EN > 0
        uiBpType = archDbgTrapType(ulRetAddr, LW_NULL);                 /*  �ϵ�ָ��̽��                */
        if (uiBpType) {
            if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {
                break;                                                  /*  ������Խӿڶϵ㴦��        */
            }
        }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
        abtInfo.VMABT_uiMethod = 0;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    case EX_RI:                                                         /*  Reserved instruction        */
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    case EX_FPE:                                                        /*  floating point exception    */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FPE;                   /*  ��ֹ����Ĭ��Ϊ FPU �쳣     */

#if LW_CFG_CPU_FPU_EN > 0
        uiFEXR = mipsVfp32GetFEXR();                                    /*  ��ø����쳣����            */
        if (uiFEXR & (1 << 17)) {                                       /*  δʵ���쳣                  */
            PVOID           pvFaultAddr;
            INT             iSignal;
            ARCH_FPU_CTX   *pFpuCtx;

            pFpuCtx = &_G_mipsFpuCtx[LW_CPU_GET_CUR_ID()];
            __ARCH_FPU_SAVE(pFpuCtx);                                   /*  ���浱ǰ FPU CTX           */

            iSignal = fpu_emulator_cop1Handler((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                               (ARCH_FPU_CTX *)pFpuCtx,
                                               LW_TRUE, &pvFaultAddr);  /*  FPU ģ��                    */
            switch (iSignal) {

            case 0:                                                     /*  �ɹ�ģ��                    */
                abtInfo.VMABT_uiType = 0;
                __ARCH_FPU_RESTORE(pFpuCtx);                            /*  �ָ���ǰ FPU CTX            */
                break;

            case SIGILL:                                                /*  δ����ָ��                  */
                abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
                abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
                break;

            case SIGBUS:                                                /*  ���ߴ���                    */
                abtInfo.VMABT_uiMethod = BUS_ADRERR;
                abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
                break;

            case SIGSEGV:                                               /*  ��ַ���Ϸ�                  */
                abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
                abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
                ulAbortAddr = (ULONG)pvFaultAddr;
                break;

            case SIGFPE:                                                /*  FPU ����                    */
            default:
                abtInfo.VMABT_uiMethod = 0;
                break;
            }

        } else if (uiFEXR & (1 << 16)) {                                /*  �Ƿ������쳣                */
            abtInfo.VMABT_uiMethod = FPE_FLTINV;

        } else if (uiFEXR & (1 << 15)) {                                /*  �����쳣                    */
            abtInfo.VMABT_uiMethod = FPE_FLTDIV;

        } else if (uiFEXR & (1 << 14)) {                                /*  �����쳣                    */
            abtInfo.VMABT_uiMethod = FPE_FLTOVF;

        } else if (uiFEXR & (1 << 13)) {
            abtInfo.VMABT_uiMethod = FPE_FLTUND;                        /*  �����쳣                    */

        } else if (uiFEXR & (1 << 12)) {                                /*  ����ȷ�쳣                  */
            abtInfo.VMABT_uiMethod = FPE_FLTRES;

        } else {
            abtInfo.VMABT_uiMethod = 0;
        }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

        if (abtInfo.VMABT_uiType) {
            API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        }
#if LW_CFG_CPU_FPU_EN > 0
        mipsVfp32ClearFEXR();                                           /*  ��������쳣                */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        break;

    case EX_CPU:                                                        /*  CoProcessor Unusable        */
#if LW_CFG_CPU_FPU_EN > 0
        if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                  /*  ���� FPU ָ��̽��           */
            break;
        }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        abtInfo.VMABT_uiMethod = 0;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    case EX_OV:                                                         /*  OVerflow                    */
    case EX_C2E:                                                        /*  COP2 exception              */
    case EX_MDMX:                                                       /*  MDMX exception              */
    case EX_WATCH:                                                      /*  Watch exception             */
    case EX_MCHECK:                                                     /*  Machine check exception     */
    case EX_CacheErr:                                                   /*  Cache error caused re-entry */
                                                                        /*  to Debug Mode               */
        abtInfo.VMABT_uiMethod = 0;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    default:
        _BugFormat(LW_TRUE, LW_TRUE, "Unknow exception: %d\r\n", uiExcCode);
        break;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
