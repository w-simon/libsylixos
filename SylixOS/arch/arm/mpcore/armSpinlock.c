/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: armSpinlock.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ��������������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  spinlock ״̬
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#include "armMpCore.h"
/*********************************************************************************************************
  L1 cache ͬ����ο�: http://www.cnblogs.com/jiayy/p/3246133.html
*********************************************************************************************************/
/*********************************************************************************************************
  spin lock cache ��������
*********************************************************************************************************/
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
static VOID                   armSpinLockDummy(volatile SPINLOCKTYPE  *psl);
static volatile SPINLOCKTYPE  armSpinTryLockDummy(volatile SPINLOCKTYPE  *psl);
static VOID                   armSpinUnlockDummy(volatile SPINLOCKTYPE  *psl);

static VOID                   (*pfuncArmSpinLock)(volatile SPINLOCKTYPE)    = armSpinLockDummy;
static volatile SPINLOCKTYPE  (*pfuncArmSpinTryLock)(volatile SPINLOCKTYPE) = armSpinTryLockDummy;
static VOID                   (*pfuncArmSpinUnlock)(volatile SPINLOCKTYPE)  = armSpinUnlockDummy;
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
/*********************************************************************************************************
** ��������: armSpinLockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0

static VOID  armSpinLockDummy (volatile SPINLOCKTYPE  *psl)
{
}
/*********************************************************************************************************
** ��������: armSpinTryLockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static volatile SPINLOCKTYPE  armSpinTryLockDummy (volatile SPINLOCKTYPE  *psl)
{
    return  (0);
}
/*********************************************************************************************************
** ��������: armSpinUnlockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armSpinUnlockDummy (volatile SPINLOCKTYPE  *psl)
{
}
/*********************************************************************************************************
** ��������: archSpinWork
** ��������: spinlock ������Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���˿��� CACHE ��, BSP Ӧ�������ô˺���, ʹ spinlock ��Ч, 
             �Ӻ����������� CACHE ������, ���ò��� spinlock.
*********************************************************************************************************/
VOID  archSpinWork (VOID)
{
    pfuncArmSpinUnlock  = armSpinUnlock;
    pfuncArmSpinTryLock = armSpinTryLock;
    pfuncArmSpinLock    = armSpinLock;
}

#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
/*********************************************************************************************************
** ��������: archSpinInit
** ��������: ��ʼ��һ�� spinlock
** �䡡��  : psl        spinlock ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archSpinInit (spinlock_t  *psl)
{
    psl->SL_sltData   = 0;                                              /*  0: δ����״̬  1: ����״̬  */
    psl->SL_pcpuOwner = LW_NULL;
    psl->SL_ulCounter = 0ul;                                            /*  ����������                  */
    KN_SMP_WMB();
}
/*********************************************************************************************************
** ��������: archSpinDelay
** ��������: �ȴ��¼�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archSpinDelay (VOID)
{
    volatile INT  i;

    for (i = 0; i < 3; i++) {
    }
}
/*********************************************************************************************************
** ��������: archSpinNotify
** ��������: ���� spin �¼�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archSpinNotify (VOID)
{
}
/*********************************************************************************************************
** ��������: archSpinLock
** ��������: spinlock ����
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinLock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner == LW_CPU_GET_CUR()) {
        psl->SL_ulCounter++;
        _BugFormat((psl->SL_ulCounter > 10), LW_TRUE, 
                   "spinlock RECURSIVE %lu!\r\n", psl->SL_ulCounter);
        return  (1);                                                    /*  �ظ�����                    */
    }
    
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    pfuncArmSpinLock(&psl->SL_sltData);
#else
    armSpinLock(&psl->SL_sltData);
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
    
    psl->SL_pcpuOwner = LW_CPU_GET_CUR();                               /*  ���浱ǰ CPU                */
    
    return  (1);                                                        /*  �����ɹ�                    */
}
/*********************************************************************************************************
** ��������: archSpinTryLock
** ��������: spinlock ��ͼ����
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinTryLock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner == LW_CPU_GET_CUR()) {
        psl->SL_ulCounter++;
        _BugFormat((psl->SL_ulCounter > 10), LW_TRUE, 
                   "spinlock RECURSIVE %lu!\r\n", psl->SL_ulCounter);
        return  (1);                                                    /*  �ظ�����                    */
    }
    
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    if (pfuncArmSpinTryLock(&psl->SL_sltData)) 
#else
    if (armSpinTryLock(&psl->SL_sltData)) 
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
    {                                                                   /*  ���Լ���                    */
        return  (0);
    }
    
    psl->SL_pcpuOwner = LW_CPU_GET_CUR();                               /*  ���浱ǰ CPU                */
    
    return  (1);
}
/*********************************************************************************************************
** ��������: archSpinUnlock
** ��������: spinlock ����
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinUnlock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner != LW_CPU_GET_CUR()) {
        return  (0);                                                    /*  û��Ȩ���ͷ�                */
    }
    
    if (psl->SL_ulCounter) {
        psl->SL_ulCounter--;                                            /*  �����ظ����ô���            */
        return  (1);
    }

    psl->SL_pcpuOwner = LW_NULL;                                        /*  û�� CPU ��ȡ               */
    KN_SMP_WMB();
    
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    pfuncArmSpinUnlock(&psl->SL_sltData);                               /*  ����                        */
#else
    armSpinUnlock(&psl->SL_sltData);
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/

    return  (1);
}
/*********************************************************************************************************
** ��������: archSpinLockRaw
** ��������: spinlock ���� (�����������ж�)
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinLockRaw (spinlock_t  *psl)
{
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    pfuncArmSpinLock(&psl->SL_sltData);
#else
    armSpinLock(&psl->SL_sltData);
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/

    return  (1);                                                        /*  �����ɹ�                    */
}
/*********************************************************************************************************
** ��������: archSpinTryLockRaw
** ��������: spinlock ��ͼ���� (�����������ж�)
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinTryLockRaw (spinlock_t  *psl)
{
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    if (pfuncArmSpinTryLock(&psl->SL_sltData)) 
#else
    if (armSpinTryLock(&psl->SL_sltData)) 
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
    {                                                                   /*  ���Լ���                    */
        return  (0);
    }
    
    return  (1);                                                        /*  �����ɹ�                    */
}
/*********************************************************************************************************
** ��������: archSpinUnlockRaw
** ��������: spinlock ����
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinUnlockRaw (spinlock_t  *psl)
{
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    pfuncArmSpinUnlock(&psl->SL_sltData);
#else
    armSpinUnlock(&psl->SL_sltData);                                    /*  ����                        */
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
    
    return  (1);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
