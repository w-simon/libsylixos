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
** ��   ��   ��: ThreadOnce.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 02 �� 03 ��
**
** ��        ��: ����ϵͳ�� pthread_once ֧��.

** BUG:
2010.01.09  ʹ�� ATOMIC �����л���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadOnce
** ��������: �̰߳�ȫ�Ľ�ִ��һ��ָ������.
** �䡡��  : pbOnce            once ��� BOOL ȫ�ֱ���, �����ʼ��Ϊ LW_FALSE.
**           pfuncRoutine      ��Ҫִ�еĺ���.
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_THREAD_EXT_EN > 0

LW_API  
INT  API_ThreadOnce (BOOL  *pbOnce, VOIDFUNCPTR  pfuncRoutine)
{
             INTREG     iregInterLevel;
    REGISTER INT        iOk = LW_FALSE;
    
    if (!pbOnce) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_ATOMIC_LOCK(iregInterLevel);                                   /*  ����                        */
    if (*pbOnce == LW_FALSE) {                                          /*  ������ж��Ƿ�ִ��          */
        *pbOnce =  LW_TRUE;
        iOk     =  LW_TRUE;                                             /*  ���ö�����־                */
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);                                 /*  ����                        */
    
    if (pfuncRoutine && iOk) {
        LW_SOFUNC_PREPARE(pfuncRoutine);
        pfuncRoutine();                                                 /*  ִ��                        */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadOnce2
** ��������: �̰߳�ȫ�Ľ�ִ��һ��ָ������.
** �䡡��  : pbOnce            once ��� BOOL ȫ�ֱ���, �����ʼ��Ϊ LW_FALSE.
**           pfuncRoutine      ��Ҫִ�еĺ���.
**           pvArg             ִ�в���
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_ThreadOnce2 (BOOL  *pbOnce, VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg)
{
             INTREG     iregInterLevel;
    REGISTER INT        iOk = LW_FALSE;
    
    if (!pbOnce) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_ATOMIC_LOCK(iregInterLevel);                                   /*  ����                        */
    if (*pbOnce == LW_FALSE) {                                          /*  ������ж��Ƿ�ִ��          */
        *pbOnce =  LW_TRUE;
        iOk     =  LW_TRUE;                                             /*  ���ö�����־                */
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);                                 /*  ����                        */
    
    if (pfuncRoutine && iOk) {
        LW_SOFUNC_PREPARE(pfuncRoutine);
        pfuncRoutine(pvArg);                                            /*  ִ��                        */
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
