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
** ��   ��   ��: ThreadIdSelf.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: �̻߳���Լ��ľ��

** BUG
2007.07.18  ���� _DebugHandle() ����
2009.10.12  ������ʾע��.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2020.09.02  ���� Fast ��ȡ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadIdSelf
** ��������: ��ǰ�߳� ID
** �䡡��  : NONE
** �䡡��  : ID
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
LW_OBJECT_HANDLE  API_ThreadIdSelf (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (0);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur->TCB_ulId);
}
/*********************************************************************************************************
** ��������: API_ThreadIdSelfFast
** ��������: ��ǰ�߳� ID
** �䡡��  : NONE
** �䡡��  : ID
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
LW_OBJECT_HANDLE  API_ThreadIdSelfFast (VOID)
{
#if (defined(LW_CFG_CPU_ARCH_ARM64) && (LW_CFG_ARM64_FAST_TCB_CUR > 0))
    REGISTER PLW_CLASS_TCB   ptcbCur asm("x18");                        /*  x18 saved current tcb       */

    return  (ptcbCur->TCB_ulId);

#elif (defined(LW_CFG_CPU_ARCH_X86) && (LW_CFG_X64_FAST_TCB_CUR > 0))
    REGISTER LW_OBJECT_HANDLE  ulId;

    __asm__ __volatile__("movq %%fs:%P1, %q0"
                         : "=r" (ulId)
                         : "i" (offsetof(LW_CLASS_TCB, TCB_ulId)));
    return  (ulId);
#else
    REGISTER PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur->TCB_ulId);
#endif
}
/*********************************************************************************************************
** ��������: API_ThreadTcbSelf
** ��������: ��ǰ�߳� TCB (Σ��...)
** �䡡��  : NONE
** �䡡��  : TCB
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
PLW_CLASS_TCB  API_ThreadTcbSelf (VOID)
{
    PLW_CLASS_TCB   ptcbCur;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_NULL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  (ptcbCur);
}
/*********************************************************************************************************
** ��������: API_ThreadTcbSelfFast
** ��������: ��ǰ�߳� TCB (Σ��...)
** �䡡��  : NONE
** �䡡��  : TCB
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
PLW_CLASS_TCB  API_ThreadTcbSelfFast (VOID)
{
#if (defined(LW_CFG_CPU_ARCH_ARM64) && (LW_CFG_ARM64_FAST_TCB_CUR > 0))
    REGISTER PLW_CLASS_TCB   ptcbCur asm("x18");                        /*  x18 saved current tcb       */

    return  (ptcbCur);

#elif (defined(LW_CFG_CPU_ARCH_X86) && (LW_CFG_X64_FAST_TCB_CUR > 0))
    REGISTER UINT16  usIndex;

    __asm__ __volatile__("movw %%fs:%P1, %q0"
                         : "=r" (usIndex)
                         : "i" (offsetof(LW_CLASS_TCB, TCB_usIndex)));

    return  (_K_ptcbTCBIdTable[usIndex]);
#else
    REGISTER PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur);
#endif
}
/*********************************************************************************************************
** ��������: API_ThreadIdInter
** ��������: ��ǰ���ж��߳� ID, BSP �жϳ���ʹ��
** �䡡��  : NONE
** �䡡��  : ID
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ֻ�����жϷ�������ʹ��
                                           API ����
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ThreadIdInter (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);

    return  (ptcbCur->TCB_ulId);
}
/*********************************************************************************************************
** ��������: API_ThreadTcbInter
** ��������: ��ǰ���ж��߳� TCB, BSP �жϳ���ʹ��
** �䡡��  : NONE
** �䡡��  : TCB
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ֻ�����жϷ�������ʹ��
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_CLASS_TCB  API_ThreadTcbInter (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);

    return  (ptcbCur);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
