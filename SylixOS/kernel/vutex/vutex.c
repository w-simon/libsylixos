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
** ��   ��   ��: vutex.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2020 �� 12 �� 26 ��
**
** ��        ��: �ȴ�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "vutexLib.h"
/*********************************************************************************************************
** ��������: API_VutexPend
** ��������: �ȴ�һ����������ĳ��ֵ
** �䡡��  : piVar     �ȴ��ı�����ַ
**           iExpect   ��������ֵ
**           iFlags    ����ѡ��
**           ulTimeout �ȴ�ʱ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
INT  API_VutexPend (INT  *piVar, INT  iExpect, INT  iFlags, ULONG  ulTimeout)
{
    phys_addr_t    phyaddr;
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulTimeSave;                                          /*  ϵͳ�¼���¼                */
    INT            iSchedRet;

    if (!piVar) {
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

__wait_again:
    if (*(volatile INT *)piVar == iExpect) {
        return  (ERROR_NONE);

    } else if (ulTimeout == LW_OPTION_NOT_WAIT) {                       /*  ���ȴ�                      */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (PX_ERROR);
    }

    if (iFlags & LW_OPTION_VUTEX_GLOBAL) {                              /*  �Ƿ�Ϊȫ�ֵ�ַ              */
#if LW_CFG_VMM_EN > 0
        if (vmmVirtualToPhysical((addr_t)piVar, &phyaddr)) {            /*  ת��Ϊ�����ַ              */
            return  (PX_ERROR);
        }
#else                                                                   /*  LW_CFG_VMM_EN > 0           */
        phyaddr = (phys_addr_t)piVar;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
    } else {
        phyaddr = (phys_addr_t)piVar;
    }

    if (phyaddr == LW_PHY_ADDR_INVALID) {                               /*  ��ַ��Ч                    */
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (*(volatile INT *)piVar == iExpect) {
        __KERNEL_EXIT();
        return  (ERROR_NONE);
    }

    LW_TCB_GET_CUR(ptcbCur);

    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_VUTEX;               /*  д״̬λ����ʼ�ȴ�          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  ��յȴ�ʱ��                */

    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  �Ƿ�������ȴ�              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  ���ó�ʱʱ��                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  ��¼ϵͳʱ��                */

    _VutexWaitQueue(ptcbCur, phyaddr, iExpect, iFlags);                 /*  ����ȴ���                  */

    iSchedRet = __KERNEL_EXIT();                                        /*  �˳��ں�                    */
    if (*(volatile INT *)piVar == iExpect) {
        return  (ERROR_NONE);
    }

    if (iSchedRet == LW_SIGNAL_EINTR) {
        _ErrorHandle(EINTR);                                            /*  ���źŴ��                  */
        return  (PX_ERROR);

    } else if (iSchedRet == LW_SIGNAL_RESTART) {
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);

    } else {
        if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {           /*  �����ѻ�ʱ��              */
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                    /*  �ȴ���ʱ                    */
            return  (PX_ERROR);

        } else {
            ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);
        }
    }

    if (ulTimeout != LW_OPTION_NOT_WAIT) {
        goto    __wait_again;                                           /*  ���µȴ�                    */
    }

    _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                            /*  �ȴ���ʱ                    */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_VutexPost
** ��������: �ı�һ������Ϊĳ��ֵ
** �䡡��  : piVar     ������ַ
**           iValue    Ҫ���õ�ֵ
**           iFlags    ����ѡ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
INT  API_VutexPost (INT  *piVar, INT  iValue, INT  iFlags)
{
    phys_addr_t    phyaddr;
    PLW_CLASS_TCB  ptcbCur;

    if (!piVar) {
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

    if (*(volatile INT *)piVar == iValue) {
        return  (ERROR_NONE);
    }

    if (iFlags & LW_OPTION_VUTEX_GLOBAL) {                              /*  �Ƿ�Ϊȫ�ֵ�ַ              */
#if LW_CFG_VMM_EN > 0
        if (vmmVirtualToPhysical((addr_t)piVar, &phyaddr)) {            /*  ת��Ϊ�����ַ              */
            return  (PX_ERROR);
        }
#else                                                                   /*  LW_CFG_VMM_EN > 0           */
        phyaddr = (phys_addr_t)piVar;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
    } else {
        phyaddr = (phys_addr_t)piVar;
    }

    if (phyaddr == LW_PHY_ADDR_INVALID) {                               /*  ��ַ��Ч                    */
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (*(volatile INT *)piVar == iValue) {
        __KERNEL_EXIT();
        return  (ERROR_NONE);
    }

    if (iValue != __ARCH_INT_MAX) {                                     /*  __ARCH_INT_MAX ������       */
        *piVar = iValue;
        KN_SMP_WMB();
    }

    LW_TCB_GET_CUR(ptcbCur);

    _VutexWakeQueue(ptcbCur, phyaddr, iValue, iFlags);                  /*  ���Ѻ��ʵ�����              */

    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
