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
** ��   ��   ��: _CoroutineLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 19 ��
**
** ��        ��: ����Э�̹����(Э����һ���������Ĳ���ִ�е�λ). 
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** ��������: _CoroutineDeleteAll
** ��������: �ͷ�ָ���߳����е�Э�̿ռ�.
** �䡡��  : ptcb                     �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _CoroutineFreeAll (PLW_CLASS_TCB    ptcb)
{
    REGISTER PLW_CLASS_COROUTINE    pcrcbTemp;
    REGISTER PLW_LIST_RING          pringTemp;
    
    pringTemp = ptcb->TCB_pringCoroutineHeader;
    
    while (pringTemp) {
        _List_Ring_Del(pringTemp,
                       &ptcb->TCB_pringCoroutineHeader);                /*  ��Э�̱���ɾ��              */
        pcrcbTemp = _LIST_ENTRY(pringTemp, 
                                LW_CLASS_COROUTINE,
                                COROUTINE_ringRoutine);
        if (pcrcbTemp->COROUTINE_bIsNeedFree) {
            _StackFree(ptcb, pcrcbTemp->COROUTINE_pstkStackLowAddr, LW_TRUE);
        }
        pringTemp = ptcb->TCB_pringCoroutineHeader;
    }
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
