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
** ��   ��   ��: KernelIpi.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 11 �� 08 ��
**
** ��        ��: SMP ϵͳ�˼��ж�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_KernelSmpCall
** ��������: ָ�� CPU ����ָ������
** �䡡��  : ulCPUId       CPU ID
**           pfunc         ͬ��ִ�к���
**           pvArg         ͬ������
**           pfuncAsync    �첽ִ�к���
**           pvAsync       �첽ִ�в���
**           iOpt          ѡ�� IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** �䡡��  : ͬ�����÷���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API  
INT  API_KernelSmpCall (ULONG        ulCPUId, 
                        FUNCPTR      pfunc, 
                        PVOID        pvArg,
                        VOIDFUNCPTR  pfuncAsync,
                        PVOID        pvAsync, 
                        INT          iOpt)
{
    BOOL    bLock;
    INT     iRet;
    
    bLock = __SMP_CPU_LOCK();                                           /*  ������ǰ CPU ִ��           */
    
    if (ulCPUId == LW_CPU_GET_CUR_ID()) {                               /*  �����Լ������Լ�            */
        iRet = PX_ERROR;
    
    } else {
        iRet = _SmpCallFunc(ulCPUId, pfunc, pvArg, pfuncAsync, pvAsync, iOpt);
    }
    
    __SMP_CPU_UNLOCK(bLock);                                            /*  ������ǰ CPU ִ��           */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_KernelSmpCallAllOther
** ��������: �������м���� CPU ����ָ������
** �䡡��  : pfunc         ͬ��ִ�к���
**           pvArg         ͬ������
**           pfuncAsync    �첽ִ�к���
**           pvAsync       �첽ִ�в���
**           iOpt          ѡ�� IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** �䡡��  : NONE (�޷�ȷ������ֵ)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_KernelSmpCallAllOther (FUNCPTR      pfunc, 
                                 PVOID        pvArg,
                                 VOIDFUNCPTR  pfuncAsync,
                                 PVOID        pvAsync,
                                 INT          iOpt)
{
    BOOL    bLock;
    
    bLock = __SMP_CPU_LOCK();                                           /*  ������ǰ CPU ִ��           */
    
    _SmpCallFuncAllOther(pfunc, pvArg, pfuncAsync, pvAsync, iOpt);
    
    __SMP_CPU_UNLOCK(bLock);                                            /*  ������ǰ CPU ִ��           */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
