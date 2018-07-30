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
** ��   ��   ��: mipsSpinlock.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ�ܹ�����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  spinlock ״̬
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  L1 cache ͬ����ο�: http://www.cnblogs.com/jiayy/p/3246133.html
*********************************************************************************************************/
#ifdef LW_CFG_CPU_ARCH_MICROMIPS
#define GCC_OFF_SMALL_ASM()     "ZC"
#else
#define GCC_OFF_SMALL_ASM()     "R"
#endif
/*********************************************************************************************************
** ��������: mipsSpinLock
** ��������: MIPS spin lock
** �䡡��  : psld       spinlock data ָ��
**           pfuncPoll  ѭ���ȴ�ʱ���ú���
**           pvArg      �ص�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
*********************************************************************************************************/
static LW_INLINE VOID  mipsSpinLock (SPINLOCKTYPE *psld, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
    UINT32          uiNewVal;
    UINT32          uiInc = 1 << LW_SPINLOCK_TICKET_SHIFT;
    SPINLOCKTYPE    sldVal;

    __asm__ __volatile__(
        "   .set push                                   \n"
        "   .set noreorder                              \n"
        "1: ll      %[oldvalue], %[slock]               \n"
        "   addu    %[newvalue], %[oldvalue], %[inc]    \n"
        "   sc      %[newvalue], %[slock]               \n"
        "   beqz    %[newvalue], 1b                     \n"
        "   nop                                         \n"
        "   .set pop"
        : [slock] "+" GCC_OFF_SMALL_ASM() (psld->SLD_uiLock),
          [oldvalue] "=&r" (sldVal),
          [newvalue] "=&r" (uiNewVal)
        : [inc] "r" (uiInc));

    while (sldVal.SLD_usTicket != sldVal.SLD_usSvcNow) {
        if (pfuncPoll) {
            pfuncPoll(pvArg);
        }
        sldVal.SLD_usSvcNow = LW_ACCESS_ONCE(UINT16, psld->SLD_usSvcNow);
    }
}
/*********************************************************************************************************
** ��������: mipsSpinTryLock
** ��������: MIPS spin trylock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : 1: busy 0: ok
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
*********************************************************************************************************/
static LW_INLINE UINT32  mipsSpinTryLock (SPINLOCKTYPE *psld)
{
    UINT32  uiRes, uiTemp1, uiTemp2;
    UINT32  uiInc = 1 << LW_SPINLOCK_TICKET_SHIFT;

    __asm__ __volatile__ (
        "   .set push                                       \n"
        "   .set noreorder                                  \n"
        "                                                   \n"
        "1: ll      %[ticket],     %[slock]                 \n"
        "   srl     %[myticket],   %[ticket],     16        \n"
        "   andi    %[nowserving], %[ticket],     0xffff    \n"
        "   bne     %[myticket],   %[nowserving], 3f        \n"
        "   addu    %[ticket],     %[ticket],     %[inc]    \n"
        "   sc      %[ticket],     %[slock]                 \n"
        "   beqz    %[ticket],     1b                       \n"
        "   li      %[ticket],     0                        \n"
        "2:                                                 \n"
        "   .subsection 2                                   \n"
        "3: b       2b                                      \n"
        "   li      %[ticket],     1                        \n"
        "   .previous                                       \n"
        "   .set pop                                        \n"
        : [slock] "+" GCC_OFF_SMALL_ASM() (psld->SLD_uiLock),
          [ticket] "=&r" (uiRes),
          [myticket] "=&r" (uiTemp1),
          [nowserving] "=&r" (uiTemp2)
        : [inc] "r" (uiInc));

    return  (uiRes);
}
/*********************************************************************************************************
** ��������: mipsSpinUnlock
** ��������: MIPS spin unlock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  mipsSpinUnlock (SPINLOCKTYPE *psld)
{
    psld->SLD_usSvcNow++;
    KN_SMP_WMB();
}
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
    psl->SL_sltData.SLD_uiLock = 0;                                     /*  0: δ����״̬  1: ����״̬  */
    psl->SL_pcpuOwner          = LW_NULL;
    psl->SL_ulCounter          = 0;
    psl->SL_pvReserved         = LW_NULL;
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
**           pfuncPoll  ѭ���ȴ�ʱ���ú���
**           pvArg      �ص�����
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archSpinLock (spinlock_t  *psl, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
    if (psl->SL_pcpuOwner == LW_CPU_GET_CUR()) {
        psl->SL_ulCounter++;
        _BugFormat((psl->SL_ulCounter > 10), LW_TRUE,
                   "spinlock RECURSIVE %lu!\r\n", psl->SL_ulCounter);
        return  (1);                                                    /*  �ظ�����                    */
    }

    mipsSpinLock(&psl->SL_sltData, pfuncPoll, pvArg);

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

    if (mipsSpinTryLock(&psl->SL_sltData)) {                            /*  ���Լ���                    */
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

    mipsSpinUnlock(&psl->SL_sltData);                                   /*  ����                        */

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
    mipsSpinLock(&psl->SL_sltData, LW_NULL, LW_NULL);

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
    if (mipsSpinTryLock(&psl->SL_sltData)) {                            /*  ���Լ���                    */
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
    mipsSpinUnlock(&psl->SL_sltData);                                   /*  ����                        */

    return  (1);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
