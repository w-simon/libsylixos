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
** ��   ��   ��: armExc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "../mm/mmu/armMmuCommon.h"
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
** ��������: archAbtHandle
** ��������: ϵͳ���� data abort ���� prefetch_abort �쳣ʱ����ô˺���
** �䡡��  : ulRetAddr     �쳣���ص�ַ.
**           uiArmExcType  ARM �쳣����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archAbtHandle (addr_t  ulRetAddr, UINT32  uiArmExcType)
{
#define ARM_EXC_TYPE_ABT    8
#define ARM_EXC_TYPE_PRE    4
    
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulAbortAddr;
    
    if (uiArmExcType == ARM_EXC_TYPE_ABT) {
        ulAbortAddr = armGetAbtAddr();
        armGetAbtType(&abtInfo);
    
    } else {
        ulAbortAddr = armGetPreAddr(ulRetAddr);
        armGetPreType(&abtInfo);
    }
    
    LW_TCB_GET_CUR(ptcbCur);

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archUndHandle
** ��������: archUndEntry ��Ҫ���ô˺�������δ����ָ��
** �䡡��  : ulAddr           ��Ӧ�ĵ�ַ
**           uiCpsr           �����쳣ʱ�� CPSR
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archUndHandle (addr_t  ulAddr, UINT32  uiCpsr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    
#if LW_CFG_GDB_EN > 0
    UINT    uiBpType = archDbgTrapType(ulAddr, (PVOID)uiCpsr);          /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulAddr, uiBpType) == ERROR_NONE) {      /*  ������Խӿڶϵ㴦��        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
    LW_TCB_GET_CUR(ptcbCur);
    
#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
    
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = 0;
    
    API_VmmAbortIsr(ulAddr, ulAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archSwiHandle
** ��������: archSwiEntry ��Ҫ���ô˺����������ж�
** �䡡��  : uiSwiNo       ���жϺ�
**           puiRegs       �Ĵ�����ָ��, ǰ 14 ��Ϊ R0-R12 LR �������Ϊ���� 4 ��������ѹջ��.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���´���Ϊ��������, SylixOS Ŀǰδʹ��.
*********************************************************************************************************/
VOID  archSwiHandle (UINT32  uiSwiNo, UINT32  *puiRegs)
{
#ifdef __SYLIXOS_DEBUG
    UINT32  uiArg[10];
    
    uiArg[0] = puiRegs[0];                                              /*  ǰ�ĸ�����ʹ�� R0-R4 ����   */
    uiArg[1] = puiRegs[1];
    uiArg[2] = puiRegs[2];
    uiArg[3] = puiRegs[3];
    
    uiArg[4] = puiRegs[0 + 14];                                         /*  ����Ĳ���Ϊ��ջ����        */
    uiArg[5] = puiRegs[1 + 14];
    uiArg[6] = puiRegs[2 + 14];
    uiArg[7] = puiRegs[3 + 14];
    uiArg[8] = puiRegs[4 + 14];
    uiArg[9] = puiRegs[5 + 14];
#endif
    
    (VOID)uiSwiNo;
    puiRegs[0] = 0x0;                                                   /*  R0 Ϊ����ֵ                 */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
