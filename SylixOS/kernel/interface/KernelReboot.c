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
** ��   ��   ��: KernelReboot.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 02 ��
**
** ��        ��: �ں�������������.

** BUG:
2009.05.05  ��Ҫ���� CACHE ����Ӧ bootloader �Ĺ���.
2009.05.31  ������ except �߳�ִ��, ������Ҫ��ס�ں�.
2009.06.24  API_KernelRebootEx() ������ַ�еľֲ��������ܻᱻһЩ���ĵ� BSP CACHE ������д, ��������ȫ��
            ��������.
2011.03.08  ���� c++ ����ʱж�غ���.
2012.03.26  ���� __LOGMESSAGE_LEVEL ��Ϣ��ӡ.
2012.11.09  ����ϵͳ�����������Ͷ���, ���������ش���ʱ�����ûص���������������.
2013.12.03  ���ϵͳ����ʱ, ��Ҫ�� LW_NCPUS - 1 �� idle �߳����ȼ��ᵽ���, ��ռ�����˵� CPU Ȼ��ʵ����
            ��Ϊ����ϵͳִ�и�λ�����Ĳ���.
2014.07.21  ����ʱ, �Զ��ر����� CPU.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ϵͳ��������
*********************************************************************************************************/
INT  _excJobAdd(VOIDFUNCPTR  pfunc, 
                PVOID        pvArg0,
                PVOID        pvArg1,
                PVOID        pvArg2,
                PVOID        pvArg3,
                PVOID        pvArg4,
                PVOID        pvArg5);
VOID _cppRtUninit(VOID);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static addr_t   _K_ulRebootStartAddress;                                /*  ����������ַ                */
/*********************************************************************************************************
** ��������: __makeOtherDown
** ��������: �ر����� CPU. (LW_NCPUS ������� 1)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#if LW_CFG_SMP_CPU_DOWN_EN > 0

static VOID  __makeOtherDown (VOID)
{
    ULONG   i;
    BOOL    bNeedWait;
    
    for (i = 1; i < LW_NCPUS; i++) {                                    /*  �� 0 ��������� CPU         */
        API_CpuDown(i);
    }
    
    do {
        bNeedWait = LW_FALSE;
        for (i = 1; i < LW_NCPUS; i++) {
            if (API_CpuIsUp(i)) {                                       /*  ȷ���� 0 ����, ���� CPU ȫ��*/
                bNeedWait = LW_TRUE;
            }
        }
        LW_SPINLOCK_DELAY();
    } while (bNeedWait);
}

#else

static VOID  __makeOtherDown (VOID)
{
    ULONG           i;
    PLW_CLASS_TCB   ptcbIdle;
    BOOL            bRunning;
    
    for (i = 1; i < LW_NCPUS; i++) {                                    /*  �� 0 ��������� CPU         */
        ptcbIdle = _K_ptcbIdle[i];
        if (!LW_PRIO_IS_EQU(ptcbIdle->TCB_ucPriority, LW_PRIO_HIGHEST)) {
            __KERNEL_ENTER();                                           /*  �����ں�                    */
            _SchedSetPrio(ptcbIdle, LW_PRIO_HIGHEST);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            
            do {
                API_TimeSleep(LW_OPTION_ONE_TICK);
                __KERNEL_ENTER();                                       /*  �����ں�                    */
                bRunning = __LW_THREAD_IS_RUNNING(ptcbIdle);
                __KERNEL_EXIT();                                        /*  �˳��ں�                    */
            } while (!bRunning);
        }
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN      */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: API_KernelReboot
** ��������: �ں�������������
** �䡡��  : iRebootType        �������� 
                                LW_REBOOT_FORCE
                                LW_REBOOT_WARM
                                LW_REBOOT_COLD
                                LW_REBOOT_SHUTDOWN
                                LW_REBOOT_POWEROFF
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID   API_KernelReboot (INT  iRebootType)
{
    API_KernelRebootEx(iRebootType, 0ull);
}
/*********************************************************************************************************
** ��������: API_KernelReboot
** ��������: �ں�������������
** �䡡��  : iRebootType        �������� 
                                LW_REBOOT_FORCE
                                LW_REBOOT_WARM
                                LW_REBOOT_COLD
                                LW_REBOOT_SHUTDOWN
                                LW_REBOOT_POWEROFF
**           ulStartAddress     ������ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID   API_KernelRebootEx (INT  iRebootType, addr_t  ulStartAddress)
{
    INTREG  iregInterLevel;
    ULONG   ulI;
    
    if (iRebootType == LW_REBOOT_FORCE) {
        archReboot(iRebootType, ulStartAddress);                        /*  ������ϵ�ܹ���������        */
        _BugHandle(LW_TRUE, LW_TRUE, "kernel reboot error!\r\n");       /*  �������е�����              */
    }

    if (LW_CPU_GET_CUR_NESTING() || (API_ThreadIdSelf() != API_KernelGetExc())) {
        _excJobAdd(API_KernelRebootEx, (PVOID)iRebootType, (PVOID)ulStartAddress, 0, 0, 0, 0);
        return;
    }

    _DebugHandle(__PRINTMESSAGE_LEVEL, "kernel rebooting...\r\n");
    
    _K_ulRebootStartAddress = ulStartAddress;                           /*  ��¼�ֲ�����, ��ֹ XXX      */
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {
        __makeOtherDown();                                              /*  ������ CPU ����Ϊ idle ģʽ */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    if (iRebootType != LW_REBOOT_FORCE) {
        __LW_KERNEL_REBOOT_HOOK(iRebootType);                           /*  ���ûص�����                */
        _cppRtUninit();                                                 /*  ж�� C++ ����ʱ             */
    }
    
    for (ulI = 0; ulI < LW_CFG_MAX_INTER_SRC; ulI++) {
        API_InterVectorDisable(ulI);                                    /*  �ر������ж�                */
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */

#if LW_CFG_CACHE_EN > 0
    if (LW_NCPUS <= 1) {
        API_CacheDisable(DATA_CACHE);                                   /*  ���� CACHE                  */
        API_CacheDisable(INSTRUCTION_CACHE);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

#if LW_CFG_VMM_EN > 0
    if (LW_NCPUS <= 1) {
        API_VmmMmuDisable();                                            /*  �ر� MMU                    */
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    _DebugHandle(__PRINTMESSAGE_LEVEL, "kernel rebooting down.\r\n");
    
    archReboot(iRebootType, _K_ulRebootStartAddress);                   /*  ������ϵ�ܹ���������        */
    
    _BugHandle(LW_TRUE, LW_TRUE, "kernel reboot error!\r\n");           /*  �������е�����              */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
