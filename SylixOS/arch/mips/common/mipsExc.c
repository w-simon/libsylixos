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
** ��������: archIntHandle
** ��������: bspIntHandle ��Ҫ���ô˺��������ж� (�ر��ж����������)
** �䡡��  : ulVector         �ж�����
**           bPreemptive      �ж��Ƿ����ռ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
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
    REGISTER UINT32  uiFiled  = 2 * sizeof(UINT32);
    REGISTER UINT32  uiRegVal;

    uiRegVal = mipsCp0ConfigRead();
    mipsCp0ConfigWrite((uiRegVal & ~M_ConfigK0) | MIPS_UNCACHED);

    _PrintFormat("Cache error exception:\r\n");
    _PrintFormat("cp0_errorepc == %lx\r\n", uiFiled, mipsCp0ERRPCRead());

    uiRegVal = mipsCp0CacheErrRead();

    _PrintFormat("cp0_cacheerr == 0x%08x\r\n", uiRegVal);

    _PrintFormat("Decoded cp0_cacheerr: %s cache fault in %s reference.\r\n",
                 (uiRegVal & M_CcaheLevel) ? "secondary" : "primary",
                 (uiRegVal & M_CcaheType)  ? "data"      : "insn");

    _PrintFormat("Error bits: %s%s%s%s%s%s%s\r\n",
                 (uiRegVal & M_CcaheData) ? "ED " : "",
                 (uiRegVal & M_CcaheTag)  ? "ET " : "",
                 (uiRegVal & M_CcaheECC)  ? "EE " : "",
                 (uiRegVal & M_CcaheBoth) ? "EB " : "",
                 (uiRegVal & M_CcaheEI)   ? "EI " : "",
                 (uiRegVal & M_CcaheE1)   ? "E1 " : "",
                 (uiRegVal & M_CcaheE0)   ? "E0 " : "");

    _PrintFormat("IDX: 0x%08x\r\n", uiRegVal & (M_CcaheE0 - 1));
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
VOID  archExceptionHandle (addr_t  ulRetAddr)
{
    REGISTER UINT32  uiCause     = mipsCp0CauseRead();
    REGISTER UINT32  uiExcCode   = ((uiCause & M_CauseExcCode) >> S_CauseExcCode);
    REGISTER addr_t  ulAbortAddr = mipsCp0BadVAddrRead();
#if LW_CFG_GDB_EN > 0
    REGISTER UINT    uiBpType;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    PLW_CLASS_TCB    ptcbCur;
    LW_VMM_ABORT     abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    switch (uiExcCode) {

    case EX_INT:                                                        /*  Interrupt                   */
        bspIntHandle();
        break;

#if LW_CFG_VMM_EN > 0
    case EX_MOD:                                                        /*  TLB modified                */
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
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
        abtInfo.VMABT_uiMethod = 0;
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
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FPE;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    case EX_CPU:                                                        /*  CoProcessor Unusable        */
#if LW_CFG_CPU_FPU_EN > 0
        if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                  /*  ���� FPU ָ��̽��           */
            break;
        }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
        break;

    case EX_OV:                                                         /*  OVerflow                    */
    case EX_C2E:                                                        /*  COP2 exception              */
    case EX_MDMX:                                                       /*  MDMX exception              */
    case EX_WATCH:                                                      /*  Watch exception             */
    case EX_MCHECK:                                                     /*  Machine check exception     */
    case EX_CacheErr:                                                   /*  Cache error caused re-entry */
                                                                        /*  to Debug Mode               */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
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
