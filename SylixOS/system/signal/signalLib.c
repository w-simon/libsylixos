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
** ��   ��   ��: signalLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 06 �� 03 ��
**
** ��        ��: ϵͳ�źŴ����ڲ�������.
**
** ע        ��: ϵͳ��ֲ��ʱ��, ���뽫�����жϿ��Ʒ���������������. 
                 
** BUG
2007.09.08  һ������ _SignalShell �͹��ж�,Ȼ�������µ�������.
2008.01.16  ��ʼ�������Ϊ SIG_IGN ������.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.08.11  _doPendKill ����Ծ�����͵��ж�, ���� shell ��һ����������·����źŽ� idle ɾ����, ������Ϊ
            û�м��������.
2009.03.05  ����ں˲ü����봦���.
2009.04.14  ���� SMP ֧��.
2009.05.26  �߳�������ʱ�յ��ź�, ���߳���ִ���źž��ʱ�� resume, ��ô�źŷ���ʱ��Ӧ���� suspend.
2010.01.22  __sigMakeReady() ��Ҫ�����ں�.
2011.02.22  �ع����� signal ϵͳ, ʵ�� POSIX Ҫ����źŹ���.
2011.02.24  __sigMakeReady() �� suspend ״̬���߳�����Ϊ��! ������źŷ���;�б�ǿռ, ��ǰ�����ջ�������
            Ŀǰֻ�ܵ� resume �����ִ���źž��.
2011.02.26  ����� SIGEV_NONE SIGEV_SIGNAL SIGEV_THREAD ��֧��.
2011.05.17  ���յ���Ҫ�����̵߳��ź�ʱ, ���Ͻ����߳�.
2011.08.16  __sigRunHandle() ��Ϊ default ����ʱ, ��Ҫ�ж��Ƿ�Ϊ��ֹ�ź�, �������Ҫ��ֹ����.
2011.11.17  ���� SIGSTOP SIGTSTP ������.
2012.04.09  ϵͳ��׼�źŷ��������� psiginfo ����.
2012.04.22  SIGKILL Ĭ�ϴ���ɵ����̱߳���ֹ.
2012.05.07  ʹ�� OSStkSetFp() ������ͬ�� fp ָ��, ��֤ unwind ��ջ����ȷ��.
2012.09.14  _doSigEvent() ֧�ֽ��̺�.
2012.10.23  ��� SMP ϵͳ��, �����ź�ʱ, ���Ŀ���߳�����һ�� CPU ִ��ʱ�Ĵ���.
2012.12.09  ������ͣʱ, ��Ҫ�Ը��׽���֪ͨ.
2012.12.25  ��� SIGSEGV �ź�, Ĭ���źž����ʾ������������Ϣ.
            �źŽ���֮ǰ��¼����Ŀ��������ں�״̬, Ȼ���źž������Ҫ���û�����������, �˳��ź�ʱ��Ҫ�ظ�
            ֮ǰ��״̬.
2013.01.15  �����쳣�ź��˳�ʱ��Ҫʹ�� _exit() api.
            __sigRunHandle() �����ǿ������ź�, �����Ƿ����û�����ִ�ж�����ɱ���Լ�.
2013.05.05  ֧�� SA_RESTART �ź�.
2013.06.17  �����ź������Ĳ����޸ĵ�ǰ���� errno
2013.09.04  ������������� SA_NOCLDSTOP ��ʾ, �򲻽����ӽ�����ͣ�ź�.
2013.09.17  ����� SIGCNCL �źŵĴ���.
2013.11.24  ����� signalfd ���ܵ�֧��.
2013.12.12  _sigGetLsb() ʹ�� archFindLsb() Ѱ������Ҫ���͵��źű����С���ź�.
2014.06.01  ��������ֹͣ�ź�, ���̽��ᱻǿ��ֹͣ.
2014.09.30  SA_SIGINFO ������ź������Ĳ����Ĵ���, ���ڽ���������е��ź�, ���ź������Ĳ���Ϊ LW_NULL.
2014.10.31  ֧�� POSIX �����ָ����ջ���ź������Ĳ���.
2014.11.04  ֧�� SA_NOCLDWAIT �Զ������ӽ���.
2015.11.16  __sigMakeReady() ����Ҫ�ر��жϼ���.
            _sigPendAlloc() �� _sigPendFree() ����Ҫ�ر��ж�.
2016.04.15  �ź���������Ҫ���� FPU ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0
#include "signalPrivate.h"
/*********************************************************************************************************
  ������ش���
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "sys/vproc.h"
#include "sys/wait.h"
#include "unistd.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
#define __tcb_pid(ptcb)     __lw_vp_get_tcb_pid(ptcb)
#else
#define __tcb_pid(ptcb)     0
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  LW_CFG_MAX_SIGQUEUE_NODES ������� 2
*********************************************************************************************************/
#if     LW_CFG_MAX_SIGQUEUE_NODES < 2
#undef  LW_CFG_MAX_SIGQUEUE_NODES
#define LW_CFG_MAX_SIGQUEUE_NODES   2
#endif                                                                  /*  LW_CFG_MAX_SIGQUEUE_NODES   */
/*********************************************************************************************************
  �ź��ڲ�ȫ�ֱ���
*********************************************************************************************************/
static LW_CLASS_SIGCONTEXT      _K_sigctxTable[LW_CFG_MAX_THREADS];
static LW_CLASS_SIGPEND         _K_sigpendBuffer[LW_CFG_MAX_SIGQUEUE_NODES];
static LW_LIST_RING_HEADER      _K_pringSigPendFreeHeader;
#if LW_CFG_SIGNALFD_EN > 0
static LW_OBJECT_HANDLE         _K_hSigfdSelMutex;
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
/*********************************************************************************************************
  �ź��ڲ�����
*********************************************************************************************************/
extern VOID                     __sigEventArgInit(VOID);
static VOID                     __sigShell(PLW_CLASS_SIGCTLMSG  psigctlmsg);
       PLW_CLASS_SIGCONTEXT     _signalGetCtx(PLW_CLASS_TCB  ptcb);
static PLW_CLASS_SIGPEND        _sigPendAlloc(VOID);
       VOID                     _sigPendFree(PLW_CLASS_SIGPEND  psigpendFree);
       BOOL                     _sigPendRun(PLW_CLASS_TCB  ptcb);
static BOOL                     _sigPendRunSelf(VOID);
/*********************************************************************************************************
** ��������: __signalCnclHandle
** ��������: SIGCNCL �źŵķ�����
** �䡡��  : iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalCnclHandle (INT  iSigNo, struct siginfo *psiginfo)
{
    LW_OBJECT_HANDLE    ulId;
    PLW_CLASS_TCB       ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulId = ptcbCur->TCB_ulId;
    
    if (ptcbCur->TCB_iCancelState == LW_THREAD_CANCEL_ENABLE   &&
        ptcbCur->TCB_iCancelType  == LW_THREAD_CANCEL_DEFERRED &&
        (ptcbCur->TCB_bCancelRequest)) {
#if LW_CFG_THREAD_DEL_EN > 0
        API_ThreadDelete(&ulId, LW_THREAD_CANCELED);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
    } else {
        ptcbCur->TCB_bCancelRequest = LW_TRUE;
    }
}
/*********************************************************************************************************
** ��������: __signalExitHandle
** ��������: SIGCANCEL �źŵķ�����
** �䡡��  : iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalExitHandle (INT  iSigNo, struct siginfo *psiginfo)
{
    LW_OBJECT_HANDLE    ulId;
    PLW_CLASS_TCB       ptcbCur;
#if LW_CFG_MODULELOADER_EN > 0
    pid_t               pid = getpid();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulId = ptcbCur->TCB_ulId;

#if LW_CFG_MODULELOADER_EN > 0
    if (iSigNo != SIGTERM) {
        vprocExitModeSet(pid, LW_VPROC_EXIT_FORCE);                     /*  ǿ�ƽ����˳�                */
        vprocSetForceTerm(pid);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if ((iSigNo == SIGBUS)  ||
        (iSigNo == SIGSEGV) || 
        (iSigNo == SIGILL)  ||
        (iSigNo == SIGFPE)  ||
        (iSigNo == SIGSYS)) {
        __LW_FATAL_ERROR_HOOK(pid, ulId, psiginfo);                     /*  �ؼ����쳣                  */
        _exit(psiginfo->si_int);
    
    } else {                                                            /*  �ǹؼ����쳣                */
        API_ThreadDelete(&ulId, (PVOID)psiginfo->si_int);               /*  ɾ���Լ�                    */
    }                                                                   /*  ����ڰ�ȫģʽ, ���˳���ȫ  */
}                                                                       /*  ģʽ��, �Զ���ɾ��          */
/*********************************************************************************************************
** ��������: __signalWaitHandle
** ��������: �����ӽ�����Դ
** �䡡��  : iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalWaitHandle (INT  iSigNo, struct siginfo *psiginfo)
{
#if LW_CFG_MODULELOADER_EN > 0
    reclaimchild();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** ��������: __signalStopHandle
** ��������: SIGSTOP / SIGTSTP �źŵķ�����
** �䡡��  : iSigNo        �ź���ֵ
**           psiginfo      �ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __signalStopHandle (INT  iSigNo, struct siginfo *psiginfo)
{
    sigset_t            sigsetMask;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvproc = __LW_VP_GET_CUR_PROC();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    sigsetMask = ~__SIGNO_UNMASK;
    
    sigdelset(&sigsetMask, SIGCONT);                                    /*  unmask SIGCONT              */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvproc) {
        vprocNotifyParent(pvproc, CLD_STOPPED, LW_TRUE);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    sigsuspend(&sigsetMask);                                            /*  �����߳�                    */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvproc) {
        vprocNotifyParent(pvproc, CLD_CONTINUED, LW_TRUE);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** ��������: __sigTaskCreateHook
** ��������: �߳̽���ʱ����ʼ���߳̿��ƿ��е��źſ��Ʋ���
** �䡡��  : ulId                  �߳� ID ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigTaskCreateHook (LW_OBJECT_HANDLE  ulId)
{
             PLW_CLASS_TCB          ptcb    = __GET_TCB_FROM_INDEX(_ObjectGetIndex(ulId));
    REGISTER PLW_CLASS_SIGCONTEXT   psigctx = _signalGetCtx(ptcb);
    
    lib_bzero(psigctx, sizeof(LW_CLASS_SIGCONTEXT));                    /*  �����ź� DEFAULT ����       */
    
    psigctx->SIGCTX_stack.ss_flags = SS_DISABLE;                        /*  ��ʹ���Զ����ջ            */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (_K_hSigfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _K_hSigfdSelMutex =  API_SemaphoreMCreate("sigfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
    }
    psigctx->SIGCTX_selwulist.SELWUL_hListLock = _K_hSigfdSelMutex;
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
}
/*********************************************************************************************************
** ��������: __sigTaskDeleteHook
** ��������: �߳�ɾ��ʱ���õĻص�����
** �䡡��  : ulId                   �߳� ID ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_THREAD_DEL_EN > 0

static VOID    __sigTaskDeleteHook (LW_OBJECT_HANDLE  ulId)
{
    REGISTER INT                    iI;
             PLW_CLASS_SIGCONTEXT   psigctx;
             PLW_CLASS_TCB          ptcb = __GET_TCB_FROM_INDEX(_ObjectGetIndex(ulId));
    REGISTER PLW_CLASS_SIGPEND      psigpend;
    
    psigctx = _signalGetCtx(ptcb);                                      /*  ��� sig context            */

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (iI = 0; iI < NSIG; iI++) {
        if (psigctx->SIGCTX_pringSigQ[iI]) {                            /*  ����û�д�����Ŷ��ź�      */
                     PLW_LIST_RING  pringHead = psigctx->SIGCTX_pringSigQ[iI];
            REGISTER PLW_LIST_RING  pringSigP = pringHead;
            
            do {
                psigpend  = _LIST_ENTRY(pringSigP, 
                                        LW_CLASS_SIGPEND, 
                                        SIGPEND_ringSigQ);              /*  ��� sigpend ���ƿ��ַ     */
                pringSigP = _list_ring_get_next(pringSigP);             /*  ��һ���ڵ�                  */
                
                if ((psigpend->SIGPEND_siginfo.si_code != SI_KILL) &&
                    (psigpend->SIGPEND_iNotify         == SIGEV_SIGNAL)) {
                    _sigPendFree(psigpend);                             /*  ��Ҫ�������ж���            */
                }
            } while (pringSigP != pringHead);
        
            psigctx->SIGCTX_pringSigQ[iI] = LW_NULL;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    psigctx->SIGCTX_bRead      = LW_FALSE;
    psigctx->SIGCTX_sigsetWait = 0ull;
    SEL_WAKE_UP_TERM(&psigctx->SIGCTX_selwulist);
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
}

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
/*********************************************************************************************************
** ��������: __sigMakeReady
** ��������: ��ָ���߳�����Ϊ����״̬�������õ������ķ���ֵΪ LW_SIGNAL_RESTART (�˺������ں�״̬������)
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           iSigNo                 �ź�ֵ
**           piSchedRet             �˳��źž����, ����������ֵ.
**           iSaType                �ź����� (LW_SIGNAL_EINTR or LW_SIGNAL_RESTART)
** �䡡��  : ���ڵȴ�ʱ��ʣ��ʱ��.
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��Ŀ���̴߳��ڵȴ��¼�,�����ӳ�ʱ,��Ҫ���̵߳ĵ���������ֵ����Ϊ LW_SIGNAL_RESTART.
*********************************************************************************************************/
static VOID  __sigMakeReady (PLW_CLASS_TCB  ptcb, 
                             INT            iSigNo,
                             INT           *piSchedRet,
                             INT            iSaType)
{
             INTREG                  iregInterLevel;
    REGISTER PLW_CLASS_PCB           ppcb;

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    REGISTER PLW_CLASS_EVENTSETNODE  pesnNode = LW_NULL;
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)... */
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    REGISTER PLW_CLASS_EVENT         pevent = LW_NULL;
#endif                                                                  /*  (LW_CFG_EVENT_EN > 0) &&    */
             BOOL                    bInWakeupQ = LW_FALSE;
    
    *piSchedRet = ERROR_NONE;                                           /*  Ĭ��Ϊ����״̬              */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  ���ھ���״̬, ֱ���˳�      */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        return;
    }
    
    ppcb = _GetPcb(ptcb);                                               /*  ������ȼ����ƿ�            */
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {                  /*  �����ڻ��Ѷ�����            */
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  �ӵȴ�����ɾ��              */
        ptcb->TCB_ulDelay = 0ul;
        bInWakeupQ = LW_TRUE;
    }
    
    if (__SIGNO_MUST_EXIT & __sigmask(iSigNo)) {                        /*  �����˳��ź�                */
        if (ptcb->TCB_ptcbJoin) {
            _ThreadDisjoin(ptcb->TCB_ptcbJoin, ptcb);                   /*  �˳� join ״̬, ������������*/
        }                                                               /*  ������ܲ����ظ��Ĳ���������*/
    }
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_PEND_ANY) {               /*  ����Ƿ��ڵȴ��¼�          */
        *piSchedRet = iSaType;                                          /*  ���õ���������ֵ            */
        if (ptcb->TCB_usStatus & 
            (LW_THREAD_STATUS_SEM | LW_THREAD_STATUS_MSGQUEUE)) {       /*  ���������Ŀ��ƿ�          */
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
            pevent   = ptcb->TCB_peventPtr;
#endif                                                                  /*  (LW_CFG_EVENT_EN > 0) &&    */
        } else if (ptcb->TCB_usStatus & LW_THREAD_STATUS_EVENTSET) {
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
            pesnNode = ptcb->TCB_pesnPtr;
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)... */
        }
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_PEND_ANY);             /*  �ȴ���ʱ����¼��ȴ�λ      */
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_OUT;                     /*  �ȴ���ʱ                    */
    } else {
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                   /*  û�еȴ��¼�                */
    }
    ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
    
    if (__LW_THREAD_IS_READY(ptcb)) {
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  ���������                  */
    }
    
    if ((ptcb->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) ||
        (bInWakeupQ)) {                                                 /*  �����ȴ��¼�����˯��        */
        *piSchedRet = iSaType;                                          /*  ���õ���������ֵ            */
        if (ptcb->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {              /*  �ӵȴ��������˳�            */
            ptcb->TCB_ucWaitTimeout =  LW_WAIT_TIME_CLEAR;
            if (pevent) {
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
                _EventUnlink(ptcb);
#endif                                                                  /*  (LW_CFG_EVENT_EN > 0) &&    */
            } else if (pesnNode) {
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
                _EventSetUnlink(pesnNode);
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)... */
            }
        }
    }
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
}
/*********************************************************************************************************
** ��������: __sigCtlCreate
** ��������: �ڶ�ջ�д���һ������ִ���źź��ź��˳��Ļ���
** �䡡��  : ptcb                   ������ƿ�
**           psigctx                �ź����������Ϣ
**           psiginfo               �ź���Ϣ
**           ulSuspendNesting       ����Ƕ����
**           iSchedRet              �����ĵ���������ֵ
**           psigsetMask            ִ�����źž������Ҫ�������õ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  VOID  __sigCtlCreate (PLW_CLASS_TCB         ptcb,
                              PLW_CLASS_SIGCONTEXT  psigctx,
                              struct siginfo       *psiginfo,
                              INT                   iSchedRet,
                              sigset_t             *psigsetMask)
{
    PLW_CLASS_SIGCTLMSG  psigctlmsg;
    PLW_STACK            pstkSignalShell;                               /*  ����signalshell�Ķ�ջ��     */
    BYTE                *pucStkNow;
    stack_t             *pstack;

    if (psigctx && (psigctx->SIGCTX_stack.ss_flags & SS_ONSTACK)) {     /*  ʹ���û�ָ�����źŶ�ջ      */
        pstack = &psigctx->SIGCTX_stack;
        if ((ptcb->TCB_pstkStackNow >= (PLW_STACK)pstack->ss_sp) && 
            (ptcb->TCB_pstkStackNow < (PLW_STACK)((size_t)pstack->ss_sp + pstack->ss_size))) {
            pucStkNow = (BYTE *)ptcb->TCB_pstkStackNow;                 /*  �Ѿ����û�ָ�����źŶ�ջ��  */
        
        } else {
#if	CPU_STK_GROWTH == 0
            pucStkNow = (BYTE *)pstack->ss_sp;
#else
            pucStkNow = (BYTE *)pstack->ss_sp + pstack->ss_size;
#endif                                                                  /*  CPU_STK_GROWTH == 0         */
        }
    } else {
        pucStkNow = (BYTE *)ptcb->TCB_pstkStackNow;
    }

#if	CPU_STK_GROWTH == 0
    pucStkNow  += sizeof(LW_STACK);                                     /*  ���ջ�����ƶ�һ����ջ�ռ�  */
    psigctlmsg  = (PLW_CLASS_SIGCTLMSG)pucStkNow;                       /*  ��¼ signal contrl msg λ�� */
    pucStkNow  += __SIGCTLMSG_SIZE_ALIGN;                               /*  �ó� signal contrl msg �ռ� */
#else
    pucStkNow  -= __SIGCTLMSG_SIZE_ALIGN;                               /*  �ó� signal contrl msg �ռ� */
    psigctlmsg  = (PLW_CLASS_SIGCTLMSG)pucStkNow;                       /*  ��¼ signal contrl msg λ�� */
    pucStkNow  -= sizeof(LW_STACK);                                     /*  ���ջ�����ƶ�һ����ջ�ռ�  */
#endif

    psigctlmsg->SIGCTLMSG_iSchedRet    = iSchedRet;
    psigctlmsg->SIGCTLMSG_iKernelSpace = __KERNEL_SPACE_GET2(ptcb);
    psigctlmsg->SIGCTLMSG_pvStackRet   = ptcb->TCB_pstkStackNow;
    psigctlmsg->SIGCTLMSG_siginfo      = *psiginfo;
    psigctlmsg->SIGCTLMSG_sigsetMask   = *psigsetMask;
    psigctlmsg->SIGCTLMSG_iLastErrno   = (errno_t)ptcb->TCB_ulLastError;/*  ��¼���һ�δ���            */
    
    pstkSignalShell = archTaskCtxCreate((PTHREAD_START_ROUTINE)__sigShell, 
                                        (PVOID)psigctlmsg,
                                        (PLW_STACK)pucStkNow,
                                        0);                             /*  �����ź���ǻ���            */
    
    archTaskCtxSetFp(pstkSignalShell, ptcb->TCB_pstkStackNow);          /*  ���� fp, ʹ callstack ����  */
    
    ptcb->TCB_pstkStackNow = pstkSignalShell;                           /*  ���潨���õ��ź���Ƕ�ջ    */
    
    _StackCheckGuard(ptcb);                                             /*  ��ջ������                */
    
    __KERNEL_SPACE_SET2(ptcb, 0);                                       /*  �źž������������״̬��    */
}
/*********************************************************************************************************
** ��������: __sigReturn
** ��������: �źž������Ǻ������ô˺������ź��������з�������������.
** �䡡��  : psigctx                 �ź����������Ϣ
**           ptcbCur                 ��ǰ����������
**           psigctlmsg              �źſ�����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigReturn (PLW_CLASS_SIGCONTEXT  psigctx, 
                          PLW_CLASS_TCB         ptcbCur, 
                          PLW_CLASS_SIGCTLMSG   psigctlmsg)
{
             INTREG   iregInterLevel;
#if LW_CFG_CPU_FPU_EN > 0
    REGISTER PVOID    pvStackFP;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    psigctx->SIGCTX_sigsetSigBlockMask = psigctlmsg->SIGCTLMSG_sigsetMask;
                                                                        /*  �ָ�ԭ�ȵ�����              */
    _sigPendRunSelf();                                                  /*  ��鲢������Ҫ���е��ź�    */
    __KERNEL_SPACE_SET(psigctlmsg->SIGCTLMSG_iKernelSpace);             /*  �ָ��ɽ����ź�ǰ��״̬      */
    _SchedSetRet(psigctlmsg->SIGCTLMSG_iSchedRet);                      /*  ֪ͨ���������ص����        */
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    /*
     *  �����ﵽ���뷵�������ջ�����ļ�϶����������ռ, Ҳ������� TCB_iSchedRet ������,
     *  ��Ϊ����ֻ�������Ĵӵ������˳�ʱ, ���������Ļ�õ������ķ���ֵ, ��;�κ�ʱ�򱻴�϶�û��ϵ
     */
    errno = psigctlmsg->SIGCTLMSG_iLastErrno;                           /*  �ָ������                  */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �رյ�ǰ CPU �ж�           */
#if LW_CFG_CPU_FPU_EN > 0
    if (psigctlmsg->SIGCTLMSG_bFpuRestore &&
        (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP)) {
        pvStackFP = &psigctlmsg->SIGCTLMSG_fpuctxContext;
        __ARCH_FPU_RESTORE(pvStackFP);                                  /*  �ָ����ź��ж�ǰ FPU ������ */
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
    archSigCtxLoad(psigctlmsg->SIGCTLMSG_pvStackRet);                   /*  ���ź��������з���          */
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���в�������                */
}
/*********************************************************************************************************
** ��������: __sigRunHandle
** ��������: �ź������Ѿ���װ�ľ��
** �䡡��  : psigctx               �ź����������Ϣ
**           iSigNo                �źŵ�ֵ
**           psiginfo              ��Ҫ���е��ź���Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigRunHandle (PLW_CLASS_SIGCONTEXT  psigctx, 
                             INT                   iSigNo, 
                             struct siginfo       *psiginfo, 
                             PLW_CLASS_SIGCTLMSG   psigctlmsg)
{
    REGISTER struct sigaction     *psigaction;
    
    REGISTER VOIDFUNCPTR           pfuncHandle;
             PVOID                 pvCtx;
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    psigaction  = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
    pfuncHandle = (VOIDFUNCPTR)psigaction->sa_handler;                  /*  ����ź�ִ�к������        */
    
    if (psigaction->sa_flags & SA_ONESHOT) {                            /*  �����ػ���һ���ź�          */
        psigaction->sa_handler = SIG_DFL;                               /*  ����Ĭ�ϴ���                */
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if ((pfuncHandle != SIG_IGN)   && 
        (pfuncHandle != SIG_ERR)   &&
        (pfuncHandle != SIG_DFL)   &&
        (pfuncHandle != SIG_CATCH) &&
        (pfuncHandle != SIG_HOLD)) {
        pvCtx = (psigctlmsg) 
              ? psigctlmsg->SIGCTLMSG_pvStackRet
              : LW_NULL;
              
        if (psigaction->sa_flags & SA_SIGINFO) {                        /*  ��Ҫ siginfo_t ��Ϣ         */
            LW_SOFUNC_PREPARE(pfuncHandle);
            pfuncHandle(iSigNo, psiginfo, pvCtx);                       /*  ִ���źž��                */
        
        } else {
            LW_SOFUNC_PREPARE(pfuncHandle);
            pfuncHandle(iSigNo, pvCtx);                                 /*  XXX �Ƿ��� pvCtx ���� ?   */
        }
    
        if (__SIGNO_MUST_EXIT & __sigmask(iSigNo)) {                    /*  �����˳�                    */
            __signalExitHandle(iSigNo, psiginfo);
        
        } else if (iSigNo == SIGCNCL) {                                 /*  �߳�ȡ���ź�                */
            __signalCnclHandle(iSigNo, psiginfo);
        }
    
    } else {
        switch (iSigNo) {                                               /*  Ĭ�ϴ�����                */
        
        case SIGINT:
        case SIGQUIT:
        case SIGFPE:
        case SIGKILL:
        case SIGBUS:
        case SIGTERM:
        case SIGABRT:
        case SIGILL:
        case SIGSEGV:
        case SIGSYS:
            __signalExitHandle(iSigNo, psiginfo);
            break;
            
        case SIGSTOP:
        case SIGTSTP:
            __signalStopHandle(iSigNo, psiginfo);
            break;
            
        case SIGCHLD:
            if (((psiginfo->si_code == CLD_EXITED) ||
                 (psiginfo->si_code == CLD_KILLED) ||
                 (psiginfo->si_code == CLD_DUMPED)) &&
                (psigaction->sa_flags & SA_NOCLDWAIT)) {                /*  �����ӽ�����Դ              */
                __signalWaitHandle(iSigNo, psiginfo);
            }
            break;
            
        case SIGCNCL:
            __signalCnclHandle(iSigNo, psiginfo);
            break;
            
        default:
            break;
        }
    }
}
/*********************************************************************************************************
** ��������: __sigShell
** ��������: �ź����е���Ǻ���
** �䡡��  : psigctlmsg              �źſ�����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __sigShell (PLW_CLASS_SIGCTLMSG  psigctlmsg)
{
             INTREG                iregInterLevel;
             PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_SIGCONTEXT  psigctx;
    REGISTER struct siginfo       *psiginfo = &psigctlmsg->SIGCTLMSG_siginfo;
    REGISTER INT                   iSigNo   = psiginfo->si_signo;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = &_K_sigctxTable[_ObjectGetIndex(ptcbCur->TCB_ulId)];
    
#if LW_CFG_CPU_FPU_EN > 0
    iregInterLevel = KN_INT_DISABLE();                                  /*  �رյ�ǰ CPU �ж�           */
    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {             /*  ��ǰ�߳�ʹ�� FPU            */
        psigctlmsg->SIGCTLMSG_bFpuRestore   = LW_TRUE;                  /*  ���� TCB ����������         */
        psigctlmsg->SIGCTLMSG_fpuctxContext = ptcbCur->TCB_fpuctxContext;
        
    } else {
        psigctlmsg->SIGCTLMSG_bFpuRestore   = LW_FALSE;                 /*  ����Ҫ�ָ� FPU ������       */
    }
    KN_INT_ENABLE(iregInterLevel);                                      /*  �򿪵�ǰ CPU �ж�           */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGRUN, 
                      ptcbCur->TCB_ulId, iSigNo, psiginfo->si_code, LW_NULL);
    
    __sigRunHandle(psigctx, iSigNo, psiginfo, psigctlmsg);              /*  �����źž��                */
    
    __sigReturn(psigctx, ptcbCur, psigctlmsg);                          /*  �źŷ���                    */
}
/*********************************************************************************************************
** ��������: _signalInit
** ��������: ȫ�ֳ�ʼ���źŶ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _signalInit (VOID)
{
    REGISTER INT    i;
    
    __sigEventArgInit();
    
    for (i = 0; i < LW_CFG_MAX_SIGQUEUE_NODES; i++) {
        _List_Ring_Add_Last(&_K_sigpendBuffer[i].SIGPEND_ringSigQ, 
                            &_K_pringSigPendFreeHeader);                /*  �����������                */
    }
    
    API_SystemHookAdd(__sigTaskCreateHook, 
                      LW_OPTION_THREAD_CREATE_HOOK);                    /*  ��Ӵ����ص�����            */

#if LW_CFG_THREAD_DEL_EN > 0
    API_SystemHookAdd(__sigTaskDeleteHook, 
                      LW_OPTION_THREAD_DELETE_HOOK);                    /*  ���ɾ���ص�����            */
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
}
/*********************************************************************************************************
** ��������: _sigGetMsb
** ��������: ��һ���źż��л�ȡ�ź���ֵ. (���ȵ����ź�ֵС���ź�)
** �䡡��  : psigset        �źż�
** �䡡��  : �ź���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _sigGetLsb (sigset_t  *psigset)
{
    UINT32  uiHigh = (UINT32)((*psigset) >> 32);
    UINT32  uiLow  = (UINT32)((*psigset) & 0xffffffff);
    
    if (uiHigh) {
        return  (archFindLsb(uiHigh) + 32);
    
    } else {
        return  (archFindLsb(uiLow));
    }
}
/*********************************************************************************************************
** ��������: _signalGetCtx
** ��������: ���ָ���̵߳� sig context �ṹ
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
** �䡡��  : sig context �ṹ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_SIGCONTEXT  _signalGetCtx (PLW_CLASS_TCB  ptcb)
{
    return  (&_K_sigctxTable[_ObjectGetIndex(ptcb->TCB_ulId)]);
}
/*********************************************************************************************************
** ��������: _sigPendAlloc
** ��������: ���һ�����е��źŶ��нڵ� (�˺����ڽ����ں˺����)
** �䡡��  : NONE
** �䡡��  : ������ڿ��нڵ�, �򷵻ص�ַ, ���򷵻� LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_CLASS_SIGPEND   _sigPendAlloc (VOID)
{
    PLW_CLASS_SIGPEND   psigpendNew = LW_NULL;
    
    if (_K_pringSigPendFreeHeader) {
        psigpendNew = _LIST_ENTRY(_K_pringSigPendFreeHeader, LW_CLASS_SIGPEND, 
                                  SIGPEND_ringSigQ);                    /*  ��ÿ��п��ƿ�ĵ�ַ        */
        _List_Ring_Del(_K_pringSigPendFreeHeader, 
                       &_K_pringSigPendFreeHeader);                     /*  �ӿ��ж�����ɾ��            */
    }
    
    return  (psigpendNew);
}
/*********************************************************************************************************
** ��������: _sigPendFree
** ��������: �ͷ�һ���źŶ��нڵ�. (�˺����ڽ����ں˺����)
** �䡡��  : psigpendFree      ��Ҫ�ͷŵĽڵ��ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _sigPendFree (PLW_CLASS_SIGPEND  psigpendFree)
{
    _List_Ring_Add_Last(&psigpendFree->SIGPEND_ringSigQ, 
                        &_K_pringSigPendFreeHeader);                    /*  �黹�����ж�����            */
}
/*********************************************************************************************************
** ��������: _sigPendInit
** ��������: ��ʼ��һ���źŶ��нڵ�.
** �䡡��  : psigpend               ��Ҫ��ʼ���Ľڵ��ַ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _sigPendInit (PLW_CLASS_SIGPEND  psigpend)
{
    _LIST_RING_INIT_IN_CODE(psigpend->SIGPEND_ringSigQ);
    
    psigpend->SIGPEND_uiTimes = 0;
    psigpend->SIGPEND_iNotify = SIGEV_SIGNAL;
    psigpend->SIGPEND_psigctx = LW_NULL;
}
/*********************************************************************************************************
** ��������: _sigPendGet
** ��������: ��ȡһ����Ҫ�����е��ź�, (�˺����ڽ����ں˺����)
** �䡡��  : psigctx               ������ƿ����ź�������
**           psigset               ��Ҫ�����źż�
**           psiginfo              ��Ҫ���е��ź���Ϣ
** �䡡��  : �ź���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _sigPendGet (PLW_CLASS_SIGCONTEXT  psigctx, const sigset_t  *psigset, struct siginfo *psiginfo)
{
    INT                 iSigNo;
    INT                 iSigIndex;
    sigset_t            sigsetNeedRun;
    PLW_CLASS_SIGPEND   psigpend;
    
    sigsetNeedRun = *psigset & psigctx->SIGCTX_sigsetPending;
    if (sigsetNeedRun == 0ull) {                                        /*  ��Ҫ�����źż�������Ҫ����*/
        return  (0);
    }
    
    sigsetNeedRun &= (-sigsetNeedRun);                                  /*  ȡ sigsetNeedRun �����λ   */
    iSigNo         = _sigGetLsb(&sigsetNeedRun);
    iSigIndex      = __sigindex(iSigNo);
    
    if (sigsetNeedRun & psigctx->SIGCTX_sigsetKill) {                   /*  �� kill ���ź���Ҫ������    */
        psigctx->SIGCTX_sigsetKill  &= ~sigsetNeedRun;
        
        psiginfo->si_signo           = iSigNo;
        psiginfo->si_errno           = ERROR_NONE;
        psiginfo->si_code            = SI_KILL;
        psiginfo->si_value.sival_ptr = LW_NULL;
    
    } else {                                                            /*  û�� kill, ��һ�����Ŷ��ź� */
        psigpend = _LIST_ENTRY(psigctx->SIGCTX_pringSigQ[iSigIndex], 
                               LW_CLASS_SIGPEND, SIGPEND_ringSigQ);

        if (psigpend->SIGPEND_uiTimes == 0) {                           /*  �� pend ������ɾ��          */
            _List_Ring_Del(&psigpend->SIGPEND_ringSigQ, 
                           &psigctx->SIGCTX_pringSigQ[iSigIndex]);
        } else {
            psigpend->SIGPEND_uiTimes--;
        }
        
        *psiginfo = psigpend->SIGPEND_siginfo;
        
        if ((psigpend->SIGPEND_siginfo.si_code != SI_KILL) &&
            (psigpend->SIGPEND_iNotify         == SIGEV_SIGNAL)) {      /*  ʹ�� queue �����ź�         */
            _sigPendFree(psigpend);                                     /*  ��Ҫ�������ж���            */
        }
    }
    
    if (psigctx->SIGCTX_pringSigQ[iSigIndex] == LW_NULL) {              /*  ���źŶ��������� pend       */
        psigctx->SIGCTX_sigsetPending &= ~sigsetNeedRun;
    }
    
    return  (iSigNo);
}
/*********************************************************************************************************
** ��������: _sigPendRunSelf
** ��������: ��ǰ�߳��������еȴ����ź�. (�˺����ڽ����ں˺����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
static BOOL _sigPendRunSelf (VOID)
{
    PLW_CLASS_TCB           ptcbCur;
    PLW_CLASS_SIGCONTEXT    psigctx;
    sigset_t                sigset;                                     /*  �����Ҫ�����źż�        */
    INT                     iSigNo;
    struct siginfo          siginfo;
    
    sigset_t                sigsetOld;                                  /*  �ź�ִ�������Ҫ�ظ�������  */
    BOOL                    bIsRun = LW_FALSE;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    psigctx = _signalGetCtx(ptcbCur);
    sigset  = ~psigctx->SIGCTX_sigsetSigBlockMask;                      /*  û�б����ε��ź�            */
    if (sigset == 0) {
        return  (LW_FALSE);                                             /*  û����Ҫ�����е��ź�        */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetSigBlockMask;                     /*  ��¼��ǰ������              */
    
    do {
        iSigNo = _sigPendGet(psigctx, &sigset, &siginfo);               /*  �����Ҫ���е��ź�          */
        if (__issig(iSigNo)) {
            struct sigaction     *psigaction;
                
            psigaction = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
            
            psigctx->SIGCTX_sigsetSigBlockMask |= psigaction->sa_mask;
            if ((psigaction->sa_flags & SA_NOMASK) == 0) {              /*  ��ֹ��ͬ�ź�Ƕ��            */
                psigctx->SIGCTX_sigsetSigBlockMask |= __sigmask(iSigNo);
            }
            
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            __sigRunHandle(psigctx, iSigNo, &siginfo, LW_NULL);         /*  ֱ�������źž��            */
            __KERNEL_ENTER();                                           /*  ���½����ں�                */
            
            psigctx->SIGCTX_sigsetSigBlockMask = sigsetOld;
            bIsRun = LW_TRUE;
        
        } else {
            break;
        }
    } while (1);
    
    return  (bIsRun);
}
/*********************************************************************************************************
** ��������: _sigPendRun
** ��������: ���еȴ����ź�. (�˺����ڽ����ں˺����)
** �䡡��  : ptcb                  ������ƿ�
** �䡡��  : �Ƿ��������źž��
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �� ptcb == CPU_ptcbTCBCur ʱ, ϵͳ��ֱ�������źž��, ���ҽ����п������е� pend �ź�, ȫ������
                ptcb != CPU_ptcbTCBCur ʱ, ϵͳ����ָ�����߳��ڹ����źŻ���, �����ź�ִ��ʱ, β�����Զ�ִ��
                                           ���п������е� pend �ź�.
*********************************************************************************************************/
BOOL  _sigPendRun (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_TCB           ptcbCur;
    PLW_CLASS_SIGCONTEXT    psigctx;
    sigset_t                sigset;                                     /*  �����Ҫ�����źż�        */
    INT                     iSigNo;
    struct siginfo          siginfo;
    
    sigset_t                sigsetOld;                                  /*  �ź�ִ�������Ҫ�ظ�������  */
    INT                     iSchedRet;                                  /*  ��Ҫ���µȴ��¼�            */

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    if (ptcb == ptcbCur) {
        return  (_sigPendRunSelf());
    }

    psigctx = _signalGetCtx(ptcb);
    sigset  = ~psigctx->SIGCTX_sigsetSigBlockMask;                      /*  û�б����ε��ź�            */
    if (sigset == 0) {
        return  (LW_FALSE);                                             /*  û����Ҫ�����е��ź�        */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetSigBlockMask;                     /*  ��¼��ǰ������              */
    
    iSigNo = _sigPendGet(psigctx, &sigset, &siginfo);                   /*  �����Ҫ���е��ź�          */
    if (__issig(iSigNo)) {
        struct sigaction *psigaction = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
        INT iSaType;
        
        if (psigaction->sa_flags & SA_RESTART) {
            iSaType = LW_SIGNAL_RESTART;                                /*  ��������                    */
        } else {
            iSaType = LW_SIGNAL_EINTR;                                  /*  ���� EINTR                  */
        }
        
        __sigMakeReady(ptcb, iSigNo, &iSchedRet, iSaType);              /*  ǿ�ƽ������״̬            */
        __sigCtlCreate(ptcb, psigctx, &siginfo, iSchedRet, &sigsetOld); /*  �����ź������Ļ���          */
        
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: _sigTimeoutRecalc
** ��������: ��Ҫ���µȴ��¼�ʱ, ���¼���ȴ�ʱ��.
** �䡡��  : ulOrgKernelTime        ��ʼ�ȴ�ʱ��ϵͳʱ��
**           ulOrgTimeout           ��ʼ�ȴ�ʱ�ĳ�ʱѡ��
** �䡡��  : �µĵȴ�ʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _sigTimeoutRecalc (ULONG  ulOrgKernelTime, ULONG  ulOrgTimeout)
{
             INTREG     iregInterLevel;
    REGISTER ULONG      ulTimeRun;
             ULONG      ulKernelTime;
    
    if (ulOrgTimeout == LW_OPTION_WAIT_INFINITE) {                      /*  ���޵ȴ�                    */
        return  (ulOrgTimeout);
    }
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);
    ulTimeRun = (ulKernelTime >= ulOrgKernelTime) ?
                (ulKernelTime -  ulOrgKernelTime) :
                (ulKernelTime + (__ARCH_ULONG_MAX - ulOrgKernelTime) + 1);
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    
    if (ulTimeRun >= ulOrgTimeout) {                                    /*  �Ѿ������˳�ʱ              */
        return  (0);
    }
    
    return  (ulOrgTimeout - ulTimeRun);
}
/*********************************************************************************************************
** ��������: _doSignal
** ��������: ��ָ���źž����Ƕ��ָ�����̣߳��������Լ������ź�. (�˺������ں�״̬������)
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           psigpend               �źŵȴ�������Ϣ
** �䡡��  : ���ͽ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_SEND_VAL  _doSignal (PLW_CLASS_TCB  ptcb, PLW_CLASS_SIGPEND   psigpend)
{
    REGISTER struct siginfo      *psiginfo = &psigpend->SIGPEND_siginfo;
    REGISTER INT                  iSigNo   = psiginfo->si_signo;

    REGISTER struct sigaction    *psigaction;
    REGISTER sigset_t             sigsetSigMaskBit  = __sigmask(iSigNo);/*  �ź�����                    */
    REGISTER INT                  iSigIndex = __sigindex(iSigNo);       /*  TCB_sigaction �±�          */
    
             PLW_CLASS_SIGCONTEXT psigctx;
             sigset_t             sigsetOld;                            /*  �ź�ִ�������Ҫ�ظ�������  */
             INT                  iSchedRet;                            /*  ��Ҫ���µȴ��¼�            */
             INT                  iSaType;
             
    if (psigpend->SIGPEND_iNotify == SIGEV_NONE) {                      /*  �������ź�                  */
        return  (SEND_IGN);
    }
    
    psigctx = _signalGetCtx(ptcb);
    if (psigctx->SIGCTX_sigwait) {                                      /*  Ŀ���߳��ڵȴ��ź�          */
        if (psigctx->SIGCTX_sigwait->SIGWT_sigset & __sigmask(iSigNo)) {/*  ���ڹ��ĵ��ź�              */
            psigctx->SIGCTX_sigwait->SIGWT_siginfo = psigpend->SIGPEND_siginfo;
            __sigMakeReady(ptcb, iSigNo, &iSchedRet, LW_SIGNAL_EINTR);  /*  ��������                    */
            psigctx->SIGCTX_sigwait = LW_NULL;                          /*  ɾ���ȴ���Ϣ                */
            return  (SEND_INFO);
        }
    }
    
    psigaction = &psigctx->SIGCTX_sigaction[iSigIndex];                 /*  ���Ŀ���̵߳�����źſ��ƿ�*/
    if ((psigaction->sa_handler == SIG_ERR) ||
        (psigaction->sa_handler == SIG_IGN)) {                          /*  �������Ч��              */
        return  (SEND_IGN);
    }
    
    if ((psigaction->sa_flags & SA_NOCLDSTOP) &&
        (psigpend->SIGPEND_siginfo.si_signo == SIGCHLD) &&
        (__SI_CODE_STOP(psigpend->SIGPEND_siginfo.si_code))) {          /*  �����̲������ӽ�����ͣ�ź�  */
        return  (SEND_IGN);
    }
    
    if (sigsetSigMaskBit & psigctx->SIGCTX_sigsetSigBlockMask) {        /*  ��������                    */
        if (psiginfo->si_code == SI_KILL) {                             /*  kill �������ź�, �����Ŷ�   */
            psigctx->SIGCTX_sigsetKill    |= sigsetSigMaskBit;          /*  �� kill ���źű�������      */
            psigctx->SIGCTX_sigsetPending |= sigsetSigMaskBit;          /*  iSigNo �������εȴ�����     */
        
        } else if (psigpend->SIGPEND_ringSigQ.RING_plistNext) {         /*  ���� kill �������ź�, ���  */
            psigpend->SIGPEND_uiTimes++;                                /*  �ڶ�����, ����Ҫ���л����ź�*/
                                                                        /*  �Ѿ������ڶ�����            */
        } else {
            if (psigpend->SIGPEND_iNotify == SIGEV_SIGNAL) {            /*  ��Ҫ�Ŷ��ź�                */
                PLW_CLASS_SIGPEND   psigpendNew = _sigPendAlloc();      /*  �ӻ������л�ȡһ�����е�    */
                LW_LIST_RING_HEADER *ppringHeader = 
                                    &psigctx->SIGCTX_pringSigQ[iSigIndex];
                
                if (psigpendNew == LW_NULL) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, 
                    "no node can allocate from free sigqueue.\r\n");
                    _ErrorHandle(ERROR_SIGNAL_SIGQUEUE_NODES_NULL);
                    return  (SEND_ERROR);
                }
                
                *psigpendNew = *psigpend;                               /*  ������Ϣ                    */
                _List_Ring_Add_Last(&psigpendNew->SIGPEND_ringSigQ, 
                                    ppringHeader);                      /*  �����������                */
                psigpendNew->SIGPEND_psigctx   = psigctx;
                psigctx->SIGCTX_sigsetPending |= sigsetSigMaskBit;      /*  iSigNo �������εȴ�����     */
            }
        }
        return  (SEND_BLOCK);                                           /*  �� mask �Ķ���ִ��          */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetSigBlockMask;                     /*  �ź�ִ�������Ҫ��������Ϊ  */
                                                                        /*  �������                    */
    psigctx->SIGCTX_sigsetSigBlockMask |= psigaction->sa_mask;
    if ((psigaction->sa_flags & SA_NOMASK) == 0) {                      /*  ��ֹ��ͬ�ź�Ƕ��            */
        psigctx->SIGCTX_sigsetSigBlockMask |= sigsetSigMaskBit;
    }
    if (psigaction->sa_flags & SA_RESTART) {
        iSaType = LW_SIGNAL_RESTART;                                    /*  ��Ҫ��������                */
    } else {
        iSaType = LW_SIGNAL_EINTR;                                      /*  ���� EINTR                  */
    }
    
    __sigMakeReady(ptcb, iSigNo, &iSchedRet, iSaType);                  /*  ǿ�ƽ������״̬            */
    __sigCtlCreate(ptcb, psigctx, psiginfo, iSchedRet, &sigsetOld);     /*  �����ź������Ļ���          */
    
    return  (SEND_OK);
}
/*********************************************************************************************************
** ��������: _doKill
** ��������: kill ����������������������һ�� kill ���ź�, Ȼ�󽫻���� doSignal ��������ź�.
             (�˺������ں�״̬������)
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           iSigNo                 �ź�
** �䡡��  : �����źŽ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_SEND_VAL  _doKill (PLW_CLASS_TCB  ptcb, INT  iSigNo)
{
    struct siginfo    *psiginfo;
    PLW_CLASS_TCB      ptcbCur;
    LW_CLASS_SIGPEND   sigpend;                                         /*  ������ kill ����, ���Ծ���  */
                                                                        /*  �����������, ���ﲻ�ó�ʼ��*/
                                                                        /*  ��ص�����                  */
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psiginfo = &sigpend.SIGPEND_siginfo;
    psiginfo->si_signo = iSigNo;
    psiginfo->si_errno = errno;
    psiginfo->si_code  = SI_KILL;                                       /*  �����Ŷ�                    */
    psiginfo->si_pid   = __tcb_pid(ptcbCur);
    psiginfo->si_uid   = ptcbCur->TCB_uid;
    psiginfo->si_int   = EXIT_FAILURE;                                  /*  Ĭ���źŲ���                */

    sigpend.SIGPEND_iNotify = SIGEV_SIGNAL;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_KILL, 
                      ptcb->TCB_ulId, iSigNo, LW_NULL);
    
    return  (_doSignal(ptcb, &sigpend));                                /*  �����ź�                    */
}
/*********************************************************************************************************
** ��������: _doSigQueue
** ��������: sigqueue ����������������������һ�� queue ���ź�, Ȼ�󽫻���� doSignal ��������ź�.
             (�˺������ں�״̬������)
** �䡡��  : ptcb                   Ŀ���߳̿��ƿ�
**           iSigNo                 �ź�
**           sigvalue               �ź� value
** �䡡��  : �����źŽ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_SEND_VAL  _doSigQueue (PLW_CLASS_TCB  ptcb, INT  iSigNo, const union sigval sigvalue)
{
    struct siginfo    *psiginfo;
    PLW_CLASS_TCB      ptcbCur;
    LW_CLASS_SIGPEND   sigpend;

    _sigPendInit(&sigpend);                                             /*  ��ʼ���ɲ����Ŷ��ź�        */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psiginfo = &sigpend.SIGPEND_siginfo;
    psiginfo->si_signo = iSigNo;
    psiginfo->si_errno = errno;
    psiginfo->si_code  = SI_QUEUE;                                       /*  �Ŷ��ź�                   */
    psiginfo->si_pid   = __tcb_pid(ptcbCur);
    psiginfo->si_uid   = ptcbCur->TCB_uid;
    psiginfo->si_value = sigvalue;
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGQUEUE, 
                      ptcb->TCB_ulId, iSigNo, sigvalue.sival_ptr, LW_NULL);
    
    return  (_doSignal(ptcb, &sigpend));                                /*  �����ź�                    */
}
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
