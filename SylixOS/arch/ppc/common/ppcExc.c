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
** ��   ��   ��: ppcExc.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "ppcSpr.h"
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/mm/mmu/common/ppcMmu.h"
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
  ���� Decrementer ���ȫ�ֱ���
*********************************************************************************************************/
static ULONG    _G_ulDecVector;
static BOOL     _G_bDecPreemptive;
static UINT32   _G_uiDecValue;
static BOOL     _G_bDecInited = LW_FALSE;
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
** ��������: archDataStorageExceptionHandle
** ��������: ���ݴ洢�쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archDataStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcGetDAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiDSISR = ppcMmuGetDSISR();

    /*
     * See << programming_environment_manual >> Figure 7-16
     */

    if (uiDSISR & (0x1 << (31 - 1))) {
        /*
         * Page fault (no PTE found)
         */
        abtInfo.VMABT_uiType   = ppcMmuPteMissHandle(ulAbortAddr);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiDSISR & (0x1 << (31 - 4))) {
        /*
         * Page protection violation
         */
        if (uiDSISR & (0x1 << (31 - 6))) {
            /*
             * If the access is a store
             */
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;

        } else {
            /*
             * �������
             */
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
        }

    } else {
        /*
         * dcbt/dcbtst Instruction
         */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    }
#endif

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archInstructionStorageExceptionHandle
** ��������: ָ������쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archInstructionStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcGetDAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    /*
     * See << programming_environment_manual >> Figure 7-16
     */

    UINT32  uiSRR1 = ppcMmuGetSRR1();

    if (uiSRR1 & (0x1 << (31 - 1))) {
        /*
         * Page fault (no PTE found)
         */
        abtInfo.VMABT_uiType   = ppcMmuPteMissHandle(ulAbortAddr);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiSRR1 & (0x1 << (31 - 4))) {
        /*
         * Page protection violation
         * ������Ԥȡ
         */
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiSRR1 & (0x1 << (31 - 3))) {
        /*
         * If the segment is designated as no-execute
         */
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    } else {
        /*
         * dcbt/dcbtst Instruction
         */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    }
#endif

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archAlignmentExceptionHandle
** ��������: �Ƕ����쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archAlignmentExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcGetDAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BUS;

    LW_TCB_GET_CUR(ptcbCur);

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archProgramExceptionHandle
** ��������: �����쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archProgramExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

#if LW_CFG_GDB_EN > 0
    UINT    uiBpType = archDbgTrapType(ulRetAddr, (PVOID)LW_NULL);      /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {   /*  ������Խӿڶϵ㴦��        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archFpuUnavailableExceptionHandle
** ��������: FPU �������쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archFpuUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FPE;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archSystemCallHandle
** ��������: ϵͳ���ô���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archSystemCallHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTraceHandle
** ��������: Trace ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archTraceHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archDecrementerInterruptHandle
** ��������: Decrementer �жϴ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archDecrementerInterruptHandle (VOID)
{
    if (_G_bDecInited) {
        ppcSetDEC(_G_uiDecValue);

        archIntHandle(_G_ulDecVector, _G_bDecPreemptive);
    } else {
        ppcSetDEC(0x7FFFFFFF);
    }
}
/*********************************************************************************************************
** ��������: archDecrementerInit
** ��������: ��ʼ�� Decrementer
** �䡡��  : ulVector          Decrementer �ж�����
**           bPreemptive       �Ƿ����ռ
**           uiDecValue        Decrementer ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  :
*********************************************************************************************************/
VOID  archDecrementerInit (ULONG    ulVector,
                           BOOL     bPreemptive,
                           UINT32   uiDecValue)
{
    _G_ulDecVector    = ulVector;
    _G_bDecPreemptive = bPreemptive;
    _G_uiDecValue     = uiDecValue;

    ppcSetDEC(uiDecValue);

    _G_bDecInited = LW_TRUE;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
