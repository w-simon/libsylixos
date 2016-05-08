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
** ��   ��   ��: _KernelHighLevelInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ�ں˸߲��ʼ�������⡣

** BUG
2007.06.25  ��ϵͳ�߳����ָ�Ϊ t_??? ����ʽ��
2007.07.13  ���� _DebugHandle() ��Ϣ���ܡ�
2009.04.29  IDLE �̱߳���ʹ�� FIFO ����, �������ʱ��Ƭ���ں�û���κξ����߳�ʱ, seekthread ���� 0 �Ĵ���
2009.12.31  ����� posix ϵͳ֧�ְ��ĳ�ʼ��
            ��ʼ���ϵ��׼ʱ��.
2009.01.07  �� posix ϵͳ�����ⲿ��ʼ��.
2011.03.07  ϵͳ��ʼ���д���ʱ, ��ӡ��ش���˵��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
INT  _SysInit(VOID);                                                    /*  ϵͳ����ʼ��                */

#if LW_CFG_MPI_EN > 0
VOID _mpiInit(VOID);                                                    /*  MPI ϵͳ��ʼ��              */
#endif                                                                  /*  LW_CFG_MPI_EN               */
/*********************************************************************************************************
** ��������: _CreateIdleThread
** ��������: �ں˸߲��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���Ƚ�����һ��Ҫ�� idle �߳�, Ϊÿһ�� SMP CPU ������һ��.
*********************************************************************************************************/
static VOID  _CreateIdleThread (VOID)
{
#if LW_CFG_SMP_EN > 0
    REGISTER INT    i;
#endif

    LW_CLASS_THREADATTR     threadattr;
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_IDLE_STK_SIZE, 
                        LW_PRIO_IDLE, 
                        (LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL), 
                        LW_NULL);

#if LW_CFG_SMP_EN > 0
    for (i = 0; i < LW_NCPUS; i++) {
        CHAR    cIdle[LW_CFG_OBJECT_NAME_SIZE];

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
        snprintf(cIdle, LW_CFG_OBJECT_NAME_SIZE, "t_idle%d", i);
#else
        if (i == 0) {
            lib_strcpy(cIdle, "t_idle");
        }
#endif                                                                  /* LW_CFG_DEVICE_EN > 0         */
                                                                        /* LW_CFG_FIO_LIB_EN > 0        */
        _K_ulIdleId[i] = API_ThreadInit(cIdle, _IdleThread, &threadattr, LW_NULL);
        _K_ptcbIdle[i] = _K_ptcbTCBIdTable[_ObjectGetIndex(_K_ulIdleId[i])];
        _K_ptcbIdle[i]->TCB_ucSchedPolicy = LW_OPTION_SCHED_FIFO;       /* idle ������ FIFO ������      */
        API_ThreadStart(_K_ulIdleId[i]);
    }

#else
    _K_ulIdleId[0] = API_ThreadInit("t_idle", _IdleThread, &threadattr, LW_NULL);
    _K_ptcbIdle[0] = _K_ptcbTCBIdTable[_ObjectGetIndex(_K_ulIdleId[0])];
    _K_ptcbIdle[0]->TCB_ucSchedPolicy = LW_OPTION_SCHED_FIFO;           /* idle ������ FIFO ������      */
    API_ThreadStart(_K_ulIdleId[0]);
#endif                                                                  /* LW_CFG_SMP_EN > 0            */
}
/*********************************************************************************************************
** ��������: _CreateITimerThread
** ��������: �ں˸߲��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CreateITimerThread (VOID)
{
#if	(LW_CFG_ITIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)
    LW_CLASS_THREADATTR       threadattr;

    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_ITMR_STK_SIZE, 
                        LW_PRIO_T_TTIMER,
                        (LW_CFG_ITIMER_OPTION | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL), 
                        LW_NULL);

    _K_ulThreadItimerId = API_ThreadInit("t_itimer", _ITimerThread, &threadattr, LW_NULL);
   
    API_ThreadStart(_K_ulThreadItimerId);
#endif
}
/*********************************************************************************************************
** ��������: _KernelHighLevelInit
** ��������: �ں˸߲��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _KernelHighLevelInit (VOID)
{
    VOID  _HookListInit(VOID);
    
#if LW_CFG_SIGNAL_EN > 0
    VOID  _signalInit(VOID);
#endif                                                                  /*  LW_CFG_SIGNAL_EN            */
    
    REGISTER INT    iErr;
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel high level initialize...\r\n");
    
    _HookListInit();                                                    /*  ϵͳ��̬ HOOK ��ʼ��        */

#if LW_CFG_SIGNAL_EN > 0
    _signalInit();                                                      /*  �źų�ʼ��                  */
#endif                                                                  /*  LW_CFG_SIGNAL_EN            */

    _CreateIdleThread();                                                /*  ������������                */
    _CreateITimerThread();                                              /*  ITIMER ������             */
    
    iErr = _SysInit();
    if (iErr != ERROR_NONE) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "system initialized error!\r\n");
    
    } else {
        _DebugHandle(__LOGMESSAGE_LEVEL, "system initialized.\r\n");
    }

#if LW_CFG_MPI_EN > 0
    _mpiInit();
#endif                                                                  /*  LW_CFG_MPI_EN               */
    
    bspKernelInitHook();
    __LW_KERNEL_INIT_END_HOOK(iErr);                                    /*  ��ڲ���Ϊϵͳ��ʼ�����    */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
