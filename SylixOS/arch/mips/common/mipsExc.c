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
#include "arch/mips/common/unaligned/mipsUnaligned.h"
#if LW_CFG_VMM_EN > 0
#include "arch/mips/mm/mmu/mipsMmuCommon.h"
#endif
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
#if LW_CFG_CPU_FPU_EN > 0
ARCH_FPU_CTX   _G_mipsFpuCtx[LW_CFG_MAX_PROCESSORS];
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  MIPS �쳣����������
*********************************************************************************************************/
typedef VOID  (*MIPS_EXCEPT_HANDLE)(addr_t  ulRetAddr, addr_t  ulAbortAddr);
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
** ��������: bspCpuExcHook
** ��������: �������쳣�ص�
** �䡡��  : ptcb       �쳣������
**           ulRetAddr  �쳣���ص�ַ
**           ulExcAddr  �쳣��ַ
**           iExcType   �쳣����
**           iExcInfo   ��ϵ�ṹ����쳣��Ϣ
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CPU_EXC_HOOK_EN > 0

LW_WEAK INT  bspCpuExcHook (PLW_CLASS_TCB   ptcb,
                            addr_t          ulRetAddr,
                            addr_t          ulExcAddr,
                            INT             iExcType,
                            INT             iExcInfo)
{
    return  (0);
}

#endif                                                                  /*  LW_CFG_CPU_EXC_HOOK_EN      */
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
** ��������: archTlbModExceptHandle
** ��������: TLB modified �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archTlbModExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTlbLoadExceptHandle
** ��������: TLB load or ifetch �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archTlbLoadExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = mipsMmuTlbLoadStoreExcHandle(ulAbortAddr);
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archTlbStoreExceptHandle
** ��������: TLB store �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archTlbStoreExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = mipsMmuTlbLoadStoreExcHandle(ulAbortAddr);
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archAddrLoadExceptHandle
** ��������: Address load or ifetch �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archAddrLoadExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
    abtInfo.VMABT_uiType   = mipsUnalignedHandle((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                                 ulAbortAddr);
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archAddrStoreExceptHandle
** ��������: Address store �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archAddrStoreExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = mipsUnalignedHandle((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                                 ulAbortAddr);
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archInstBusExceptHandle
** ��������: Instruction Bus �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archInstBusExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_BUS_EXCEPTION, 0)) {
        return;
    }
#endif

    abtInfo.VMABT_uiMethod = BUS_ADRERR;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archDataBusExceptHandle
** ��������: Data Bus �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archDataBusExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_BUS_EXCEPTION, 1)) {
        return;
    }
#endif

    abtInfo.VMABT_uiMethod = BUS_ADRERR;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archSysCallHandle
** ��������: System Call ����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archSysCallHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archBreakPointHandle
** ��������: Break Point ����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archBreakPointHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_GDB_EN > 0
    UINT           uiBpType;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (*(MIPS_INSTRUCTION *)ulRetAddr == BRK_MEMU) {
        if (do_dsemulret(ptcbCur, (ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow) == ERROR_NONE) {
            return;                                                     /*  FPU ģ�ⷵ��                */
        }
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_GDB_EN > 0
    uiBpType = archDbgTrapType(ulRetAddr, LW_NULL);                     /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {
            return;                                                     /*  ������Խӿڶϵ㴦��        */
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTrapInstHandle
** ��������: Trap instruction ����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archTrapInstHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    archBreakPointHandle(ulRetAddr, ulRetAddr);
}
/*********************************************************************************************************
** ��������: archResvInstHandle
** ��������: Reserved instruction ����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archResvInstHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archFloatPointExceptHandle
** ��������: �����쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archFloatPointExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    UINT32         uiFEXR;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FPE;                       /*  ��ֹ����Ĭ��Ϊ FPU �쳣     */

#if LW_CFG_CPU_FPU_EN > 0
    uiFEXR = mipsVfp32GetFEXR();                                        /*  ��ø����쳣����            */
    if (uiFEXR & (1 << 17)) {                                           /*  δʵ���쳣                  */
        PVOID           pvFaultAddr;
        INT             iSignal;
        ARCH_FPU_CTX   *pFpuCtx;

        pFpuCtx = &_G_mipsFpuCtx[LW_CPU_GET_CUR_ID()];
        __ARCH_FPU_SAVE(pFpuCtx);                                       /*  ���浱ǰ FPU CTX            */

        iSignal = fpu_emulator_cop1Handler((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                           (ARCH_FPU_CTX *)pFpuCtx,
                                           LW_TRUE, &pvFaultAddr);      /*  FPU ģ��                    */
        switch (iSignal) {

        case 0:                                                         /*  �ɹ�ģ��                    */
            abtInfo.VMABT_uiType = 0;
            __ARCH_FPU_RESTORE(pFpuCtx);                                /*  �ָ���ǰ FPU CTX            */
            break;

        case SIGILL:                                                    /*  δ����ָ��                  */
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
            ulAbortAddr            = ulRetAddr;
            break;

        case SIGBUS:                                                    /*  ���ߴ���                    */
            abtInfo.VMABT_uiMethod = BUS_ADRERR;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
            break;

        case SIGSEGV:                                                   /*  ��ַ���Ϸ�                  */
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
            ulAbortAddr = (ULONG)pvFaultAddr;
            break;

        case SIGFPE:                                                    /*  FPU ����                    */
        default:
            abtInfo.VMABT_uiMethod = 0;
            break;
        }

    } else if (uiFEXR & (1 << 16)) {                                    /*  �Ƿ������쳣                */
        abtInfo.VMABT_uiMethod = FPE_FLTINV;

    } else if (uiFEXR & (1 << 15)) {                                    /*  �����쳣                    */
        abtInfo.VMABT_uiMethod = FPE_FLTDIV;

    } else if (uiFEXR & (1 << 14)) {                                    /*  �����쳣                    */
        abtInfo.VMABT_uiMethod = FPE_FLTOVF;

    } else if (uiFEXR & (1 << 13)) {
        abtInfo.VMABT_uiMethod = FPE_FLTUND;                            /*  �����쳣                    */

    } else if (uiFEXR & (1 << 12)) {                                    /*  ����ȷ�쳣                  */
        abtInfo.VMABT_uiMethod = FPE_FLTRES;

    } else {
        abtInfo.VMABT_uiMethod = 0;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }

#if LW_CFG_CPU_FPU_EN > 0
    mipsVfp32ClearFEXR();                                               /*  ��������쳣                */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
}
/*********************************************************************************************************
** ��������: archCoProcUnusableExceptHandle
** ��������: Э�������������쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archCoProcUnusableExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archDefaultExceptHandle
** ��������: ȱʡ���쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archDefaultExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archMachineCheckExceptHandle
** ��������: ��������쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archMachineCheckExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
  MIPS �쳣����������
*********************************************************************************************************/
static MIPS_EXCEPT_HANDLE   _G_mipsExceptHandle[32] = {
    [EX_INT]      = (MIPS_EXCEPT_HANDLE)bspIntHandle,                   /*  Interrupt                   */
    [EX_MOD]      = archTlbModExceptHandle,                             /*  TLB modified                */
    [EX_TLBL]     = archTlbLoadExceptHandle,                            /*  TLB exception(load or ifetch*/
    [EX_TLBS]     = archTlbStoreExceptHandle,                           /*  TLB exception (store)       */
    [EX_ADEL]     = archAddrLoadExceptHandle,                           /*  Address error(load or ifetch*/
    [EX_ADES]     = archAddrStoreExceptHandle,                          /*  Address error (store)       */
    [EX_IBE]      = archInstBusExceptHandle,                            /*  Instruction Bus Error       */
    [EX_DBE]      = archDataBusExceptHandle,                            /*  Data Bus Error              */
    [EX_SYS]      = archSysCallHandle,                                  /*  Syscall                     */
    [EX_BP]       = archBreakPointHandle,                               /*  Breakpoint                  */
    [EX_TR]       = archTrapInstHandle,                                 /*  Trap instruction            */
    [EX_RI]       = archResvInstHandle,                                 /*  Reserved instruction        */
    [EX_FPE]      = archFloatPointExceptHandle,                         /*  floating point exception    */
    [EX_CPU]      = archCoProcUnusableExceptHandle,                     /*  CoProcessor Unusable        */
    [EX_OV]       = archDefaultExceptHandle,                            /*  Overflow                    */
    [EX_C2E]      = archDefaultExceptHandle,                            /*  COP2 exception              */
    [EX_MDMX]     = archDefaultExceptHandle,                            /*  MDMX exception              */
    [EX_WATCH]    = archDefaultExceptHandle,                            /*  Watch exception             */
    [EX_MCHECK]   = archMachineCheckExceptHandle,                       /*  Machine check exception     */
    [EX_THREAD]   = archDefaultExceptHandle,                            /*  Thread exception            */
    [EX_DSP]      = archDefaultExceptHandle,                            /*  DSP Unusable                */
    [EX_CacheErr] = (MIPS_EXCEPT_HANDLE)archCacheErrorHandle,           /*  Cache error caused re-entry */
                                                                        /*  to Debug Mode               */
};
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
    UINT32              uiExcCode = ((uiCause & M_CauseExcCode) >> S_CauseExcCode);
    ULONG               ulNesting;
    MIPS_EXCEPT_HANDLE  pfuncExceptHandle;

    if (uiExcCode == EX_INT) {                                          /*  �Ż���ͨ�жϴ���            */
        bspIntHandle();
        return;
    }

    ulNesting = LW_CPU_GET_CUR_NESTING();
    if (ulNesting > 1) {                                                /*  �쳣����Ƕ��                */
        archTaskCtxPrint((PLW_STACK)uiSp);                              /*  ֱ�Ӵ�ӡ�Ĵ���              */
    }

    pfuncExceptHandle = _G_mipsExceptHandle[uiExcCode];
    if (pfuncExceptHandle == LW_NULL) {
        _BugFormat(LW_TRUE, LW_TRUE, "Unknow exception: %d\r\n", uiExcCode);
    } else {
        pfuncExceptHandle(ulRetAddr, ulAbortAddr);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
