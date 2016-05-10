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
** ��   ��   ��: pthread_rwlock.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: pthread ��д�����ݿ�. (ʹ����˫�ź������㷨, д������) (rwlock �������ᷢ�� EINTR)
** ע        ��: SylixOS pthread_rwlock_rdlock() ��֧��ͬһ�߳���������!!!

** BUG:
2010.01.12  ����ʱ����ʱ, ֻ�轫 pend ������һ������һ�μ���.
2010.01.13  ʹ�ü����ź����䵱ͬ����, ÿ��״̬�ĸı伤�����е��߳�, ���ǽ��Լ�������ռ.
2012.12.13  ���� SylixOS ֧�ֽ�����Դ����, ���￪ʼ֧�־�̬��ʼ��.
2013.05.01  If successful, the pthread_rwlockattr_*() and pthread_rwlock_*() functions shall return zero;
            otherwise, an error number shall be returned to indicate the error.
2016.04.13  ���� GJB7714 ��� API ֧��.
2016.05.09  ���γ�ʼ��ȷ�����̰߳�ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  ��
*********************************************************************************************************/
#define __PX_RWLOCKATTR_INIT            0x8000

#define __PX_RWLOCK_LOCK(prwlock)       \
        API_SemaphoreMPend(prwlock->PRWLOCK_ulMutex, LW_OPTION_WAIT_INFINITE)
#define __PX_RWLOCK_UNLOCK(prwlock)     \
        API_SemaphoreMPost(prwlock->PRWLOCK_ulMutex)
        
#define __PX_RWLOCK_STATUS_READING      0x0000
#define __PX_RWLOCK_STATUS_WRITING      0x0001
/*********************************************************************************************************
  ��ʼ����
*********************************************************************************************************/
static LW_OBJECT_HANDLE                 _G_ulPRWLockInitLock;
/*********************************************************************************************************
** ��������: _posixPRWLockInit
** ��������: ��ʼ�� ��д�� ����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _posixPRWLockInit (VOID)
{
    _G_ulPRWLockInitLock = API_SemaphoreMCreate("prwinit", LW_PRIO_DEF_CEILING, 
                                                LW_OPTION_INHERIT_PRIORITY | 
                                                LW_OPTION_WAIT_PRIORITY | 
                                                LW_OPTION_DELETE_SAFE, LW_NULL);
}
/*********************************************************************************************************
** ��������: __pthread_rwlock_init_invisible
** ��������: ��д�����δ���. (��̬��ʼ��)
** �䡡��  : prwlock        ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void  __pthread_rwlock_init_invisible (pthread_rwlock_t  *prwlock)
{
    if (prwlock) {
        if (prwlock->PRWLOCK_ulMutex == LW_OBJECT_HANDLE_INVALID) {
            API_SemaphoreMPend(_G_ulPRWLockInitLock, LW_OPTION_WAIT_INFINITE);
            if (prwlock->PRWLOCK_ulMutex == LW_OBJECT_HANDLE_INVALID) {
                pthread_rwlock_init(prwlock, LW_NULL);
            }
            API_SemaphoreMPost(_G_ulPRWLockInitLock);
        }
    }
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_init
** ��������: ��ʼ��һ����д�����Կ�.
** �䡡��  : prwlockattr    ���Կ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_init (pthread_rwlockattr_t  *prwlockattr)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    *prwlockattr = __PX_RWLOCKATTR_INIT;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_destroy
** ��������: ����һ����д�����Կ�.
** �䡡��  : prwlockattr    ���Կ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_destroy (pthread_rwlockattr_t  *prwlockattr)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *prwlockattr = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_setpshared
** ��������: ����һ����д�����Կ��Ƿ���̹���.
** �䡡��  : prwlockattr    ���Կ�
**           pshared        ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_setpshared (pthread_rwlockattr_t *prwlockattr, int  pshared)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlockattr_getpshared
** ��������: ��ȡһ����д�����Կ��Ƿ���̹���.
** �䡡��  : prwlockattr    ���Կ�
**           pshared        ����(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_getpshared (const pthread_rwlockattr_t *prwlockattr, int  *pshared)
{
    if (pshared) {
        *pshared = PTHREAD_PROCESS_PRIVATE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_init
** ��������: ����һ����д��.
** �䡡��  : prwlock        ���
**           prwlockattr    ���Կ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_init (pthread_rwlock_t  *prwlock, const pthread_rwlockattr_t  *prwlockattr)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    prwlock->PRWLOCK_ulRSemaphore = API_SemaphoreCCreate("prwrlock", 0, __ARCH_ULONG_MAX, 
                                                        LW_OPTION_WAIT_PRIORITY, LW_NULL);
    if (prwlock->PRWLOCK_ulRSemaphore == LW_OBJECT_HANDLE_INVALID) {
        errno = ENOSPC;
        return  (ENOSPC);
    }
    prwlock->PRWLOCK_ulWSemaphore = API_SemaphoreCCreate("prwwlock", 0, __ARCH_ULONG_MAX, 
                                                        LW_OPTION_WAIT_PRIORITY, LW_NULL);
    if (prwlock->PRWLOCK_ulWSemaphore == LW_OBJECT_HANDLE_INVALID) {
        API_SemaphoreCDelete(&prwlock->PRWLOCK_ulRSemaphore);
        errno = ENOSPC;
        return  (ENOSPC);
    }
    prwlock->PRWLOCK_ulMutex = API_SemaphoreMCreate("prwlock_m", LW_PRIO_DEF_CEILING, 
                                                    (LW_OPTION_INHERIT_PRIORITY |
                                                     LW_OPTION_DELETE_SAFE |
                                                     LW_OPTION_WAIT_PRIORITY), LW_NULL);
    if (prwlock->PRWLOCK_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        API_SemaphoreCDelete(&prwlock->PRWLOCK_ulRSemaphore);
        API_SemaphoreCDelete(&prwlock->PRWLOCK_ulWSemaphore);
        errno = ENOSPC;
        return  (ENOSPC);
    }
    
    prwlock->PRWLOCK_uiOpCounter    = 0;
    prwlock->PRWLOCK_uiRPendCounter = 0;
    prwlock->PRWLOCK_uiWPendCounter = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_destroy
** ��������: ����һ����д��.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_destroy (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SemaphoreCDelete(&prwlock->PRWLOCK_ulRSemaphore)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    API_SemaphoreCDelete(&prwlock->PRWLOCK_ulWSemaphore);
    API_SemaphoreMDelete(&prwlock->PRWLOCK_ulMutex);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_rdlock
** ��������: �ȴ�һ����д���ɶ�.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_rdlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    __PX_RWLOCK_LOCK(prwlock);                                          /*  ������д��                  */
    prwlock->PRWLOCK_uiRPendCounter++;                                  /*  �ȴ���������++              */
    do {
        if ((prwlock->PRWLOCK_uiStatus == __PX_RWLOCK_STATUS_READING) &&
            (prwlock->PRWLOCK_uiWPendCounter == 0)) {
            /*
             *  ���д����δ���ж���, ����û���κ�д�������ڸ�������. ������߳�������ȡ����.
             */
            prwlock->PRWLOCK_uiOpCounter++;                             /*  ���ڲ�������++              */
            prwlock->PRWLOCK_uiRPendCounter--;                          /*  �˳��ȴ�ģʽ                */
            break;                                                      /*  ֱ������                    */
        
        } else {
            /*
             *  ���д�������ڲ�������, �����ж��д�������ڵȴ�����, ������.
             */
            __PX_RWLOCK_UNLOCK(prwlock);                                /*  ������д��                  */
            API_SemaphoreCPend(prwlock->PRWLOCK_ulRSemaphore,
                               LW_OPTION_WAIT_INFINITE);                /*  �ȴ���д��״̬�任          */
            __PX_RWLOCK_LOCK(prwlock);                                  /*  ������д��                  */
        }
        
    } while (1);
    __PX_RWLOCK_UNLOCK(prwlock);                                        /*  ������д��                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_tryrdlock
** ��������: �������ȴ�һ����д���ɶ�.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_tryrdlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    __PX_RWLOCK_LOCK(prwlock);                                          /*  ������д��                  */
    if ((prwlock->PRWLOCK_uiStatus == __PX_RWLOCK_STATUS_READING) &&
        (prwlock->PRWLOCK_uiWPendCounter == 0)) {
        prwlock->PRWLOCK_uiOpCounter++;                                 /*  ���ڲ�������++              */
        __PX_RWLOCK_UNLOCK(prwlock);                                    /*  ������д��                  */
        return  (ERROR_NONE);
    
    } else {
        __PX_RWLOCK_UNLOCK(prwlock);                                    /*  ������д��                  */
        errno = EBUSY;
        return  (EBUSY);
    }
}
/*********************************************************************************************************
** ��������: pthread_rwlock_timedrdlock
** ��������: �ȴ�һ����д���ɶ� (���г�ʱ������).
** �䡡��  : prwlock        ���
**           abs_timeout    ���Գ�ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_timedrdlock (pthread_rwlock_t *prwlock,
                                 const struct timespec *abs_timeout)
{
    ULONG               ulTimeout;
    ULONG               ulOrgKernelTime;
    ULONG               ulError = ERROR_NONE;
    struct timespec     tvNow;
    struct timespec     tvWait  = {0, 0};
    
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((abs_timeout == LW_NULL)    || 
        (abs_timeout->tv_nsec <  0) ||
        (abs_timeout->tv_nsec >= __TIMEVAL_NSEC_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    lib_clock_gettime(CLOCK_REALTIME, &tvNow);                          /*  ��õ�ǰϵͳʱ��            */
    if (__timespecLeftTime(&tvNow, abs_timeout)) {
        tvWait = *abs_timeout;
        __timespecSub(&tvWait, &tvNow);                                 /*  �����뵱ǰ�ȴ���ʱ����    */
    }
    /*
     *  ע��: �� tvWait ����ulong tick��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.
     */
    ulTimeout = __timespecToTick(&tvWait);                              /*  �任Ϊ tick                 */
    
    __PX_RWLOCK_LOCK(prwlock);                                          /*  ������д��                  */
    prwlock->PRWLOCK_uiRPendCounter++;                                  /*  �ȴ���������++              */
    do {
        __KERNEL_TIME_GET(ulOrgKernelTime, ULONG);                      /*  ��¼ϵͳʱ��                */
        
        if ((prwlock->PRWLOCK_uiStatus == __PX_RWLOCK_STATUS_READING) &&
            (prwlock->PRWLOCK_uiWPendCounter == 0)) {
            /*
             *  ���д����δ���ж���, ����û���κ�д�������ڸ�������. ������߳�������ȡ����.
             */
            prwlock->PRWLOCK_uiOpCounter++;                             /*  ���ڲ�������++              */
            prwlock->PRWLOCK_uiRPendCounter--;                          /*  �˳��ȴ�ģʽ                */
            break;                                                      /*  ֱ������                    */
        
        } else {
            /*
             *  ���д�������ڲ�������, �����ж��д�������ڵȴ�����, ������.
             */
            __PX_RWLOCK_UNLOCK(prwlock);                                /*  ������д��                  */
            ulError = API_SemaphoreCPend(prwlock->PRWLOCK_ulRSemaphore,
                                         ulTimeout);                    /*  �ȴ���д��״̬�任          */
            __PX_RWLOCK_LOCK(prwlock);                                  /*  ������д��                  */
            if (ulError) {
                break;                                                  /*  ֱ������                    */
            }
        }
        
        ulTimeout = _sigTimeoutRecalc(ulOrgKernelTime, ulTimeout);      /*  ���¼���ȴ�ʱ��            */
    } while (1);
    __PX_RWLOCK_UNLOCK(prwlock);                                        /*  ������д��                  */
    
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        ulError = ETIMEDOUT;
        errno   = ETIMEDOUT;
    }
    
    return  ((INT)ulError);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_wrlock
** ��������: �ȴ�һ����д����д.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_wrlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    __PX_RWLOCK_LOCK(prwlock);                                          /*  ������д��                  */
    prwlock->PRWLOCK_uiWPendCounter++;                                  /*  �ȴ�д������++              */
    do {
        if (prwlock->PRWLOCK_uiOpCounter == 0) {
            /*
             *  û�����ڲ������߳�
             */
            prwlock->PRWLOCK_uiOpCounter++;                             /*  ���ڲ������߳�++            */
            prwlock->PRWLOCK_uiWPendCounter--;                          /*  �˳��ȴ�״̬                */
            prwlock->PRWLOCK_uiStatus = __PX_RWLOCK_STATUS_WRITING;     /*  ת��Ϊдģʽ                */
            break;
            
        } else {
            /*
             *  ����еȴ�д����̻߳��ߴ������ڶ�ȡ���߳�.
             */
            __PX_RWLOCK_UNLOCK(prwlock);                                /*  ������д��                  */
            API_SemaphoreCPend(prwlock->PRWLOCK_ulWSemaphore,
                               LW_OPTION_WAIT_INFINITE);                /*  �ȴ���д��״̬�任          */
            __PX_RWLOCK_LOCK(prwlock);                                  /*  ������д��                  */
        }
    } while (1);
    prwlock->PRWLOCK_ulOwner = API_ThreadIdSelf();                      /*  ��д��ӵ����                */
    __PX_RWLOCK_UNLOCK(prwlock);                                        /*  ������д��                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_trywrlock
** ��������: �������ȴ�һ����д����д.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_trywrlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    __PX_RWLOCK_LOCK(prwlock);                                          /*  ������д��                  */
    if (prwlock->PRWLOCK_uiOpCounter == 0) {
        prwlock->PRWLOCK_uiOpCounter++;                                 /*  ���ڲ������߳�++            */
        prwlock->PRWLOCK_uiStatus = __PX_RWLOCK_STATUS_WRITING;         /*  ת��Ϊдģʽ                */
        prwlock->PRWLOCK_ulOwner  = API_ThreadIdSelf();                 /*  ��д��ӵ����                */
        __PX_RWLOCK_UNLOCK(prwlock);                                    /*  ������д��                  */
        return  (ERROR_NONE);
    
    } else {
        __PX_RWLOCK_UNLOCK(prwlock);                                    /*  ������д��                  */
        errno = EBUSY;
        return  (EBUSY);
    }
}
/*********************************************************************************************************
** ��������: pthread_rwlock_wrlock
** ��������: �ȴ�һ����д����д (���г�ʱ������).
** �䡡��  : prwlock        ���
**           abs_timeout    ���Գ�ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_timedwrlock (pthread_rwlock_t *prwlock,
                                 const struct timespec *abs_timeout)
{
    ULONG               ulTimeout;
    ULONG               ulOrgKernelTime;
    ULONG               ulError = ERROR_NONE;
    struct timespec     tvNow;
    struct timespec     tvWait  = {0, 0};
    
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((abs_timeout == LW_NULL)    || 
        (abs_timeout->tv_nsec <  0) ||
        (abs_timeout->tv_nsec >= __TIMEVAL_NSEC_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    lib_clock_gettime(CLOCK_REALTIME, &tvNow);                          /*  ��õ�ǰϵͳʱ��            */
    if (__timespecLeftTime(&tvNow, abs_timeout)) {
        tvWait = *abs_timeout;
        __timespecSub(&tvWait, &tvNow);                                 /*  �����뵱ǰ�ȴ���ʱ����    */
    }
    /*
     *  ע��: �� tvWait ����ulong tick��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.
     */
    ulTimeout = __timespecToTick(&tvWait);                              /*  �任Ϊ tick                 */
    
    __PX_RWLOCK_LOCK(prwlock);                                          /*  ������д��                  */
    prwlock->PRWLOCK_uiWPendCounter++;                                  /*  �ȴ�д������++              */
    do {
        __KERNEL_TIME_GET(ulOrgKernelTime, ULONG);                      /*  ��¼ϵͳʱ��                */
        
        if (prwlock->PRWLOCK_uiOpCounter == 0) {
            /*
             *  û�����ڲ������߳�
             */
            prwlock->PRWLOCK_uiOpCounter++;                             /*  ���ڲ������߳�++            */
            prwlock->PRWLOCK_uiWPendCounter--;                          /*  �˳��ȴ�״̬                */
            prwlock->PRWLOCK_uiStatus = __PX_RWLOCK_STATUS_WRITING;     /*  ת��Ϊдģʽ                */
            break;
            
        } else {
            /*
             *  ����еȴ�д����̻߳��ߴ������ڶ�ȡ���߳�.
             */
            __PX_RWLOCK_UNLOCK(prwlock);                                /*  ������д��                  */
            ulError = API_SemaphoreCPend(prwlock->PRWLOCK_ulWSemaphore,
                                         ulTimeout);                    /*  �ȴ���д��״̬�任          */
            __PX_RWLOCK_LOCK(prwlock);                                  /*  ������д��                  */
            if (ulError) {
                break;                                                  /*  ֱ������                    */
            }
        }
        
        ulTimeout = _sigTimeoutRecalc(ulOrgKernelTime, ulTimeout);      /*  ���¼���ȴ�ʱ��            */
    } while (1);
    
    if (ulError == ERROR_NONE) {
        prwlock->PRWLOCK_ulOwner = API_ThreadIdSelf();                  /*  ��д��ӵ����                */
    }
    __PX_RWLOCK_UNLOCK(prwlock);                                        /*  ������д��                  */
    
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        ulError = ETIMEDOUT;
        errno   = ETIMEDOUT;
    }
    
    return  ((INT)ulError);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_unlock
** ��������: �ͷ�һ����д��.
** �䡡��  : prwlock        ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_unlock (pthread_rwlock_t  *prwlock)
{
    ULONG       ulReleasNum;

    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    __PX_RWLOCK_LOCK(prwlock);                                          /*  ������д��                  */
    if (prwlock->PRWLOCK_uiStatus == __PX_RWLOCK_STATUS_WRITING) {
        /*
         *  д״̬���ͷ�.
         */
        if (prwlock->PRWLOCK_ulOwner != API_ThreadIdSelf()) {           /*  ���Ƕ�д��ӵ����            */
            __PX_RWLOCK_UNLOCK(prwlock);                                /*  ������д��                  */
            errno = EPERM;
            return  (EPERM);
        }
        
        prwlock->PRWLOCK_uiStatus = __PX_RWLOCK_STATUS_READING;         /*  ���½���ɶ�ģʽ            */
        prwlock->PRWLOCK_ulOwner  = LW_OBJECT_HANDLE_INVALID;           /*  û��дӵ����                */
        
    }
    
    prwlock->PRWLOCK_uiOpCounter--;                                     /*  ��������--                  */
    
    /*
     *  SylixOS ʹ��д����ԭ��(���ŵ���� www.ibm.com/developerworks/cn/linux/l-rwlock_writing/index.html)
     */
    if (prwlock->PRWLOCK_uiOpCounter) {                                 /*  ����ռ�õ��߳�              */
        if (prwlock->PRWLOCK_uiWPendCounter == 0) {                     /*  û�еȴ�д���߳�            */
            ulReleasNum = (ULONG)prwlock->PRWLOCK_uiRPendCounter;
            if (ulReleasNum) {
                API_SemaphoreCRelease(prwlock->PRWLOCK_ulRSemaphore,
                                      ulReleasNum,
                                      LW_NULL);                         /*  �����еĵȴ����߳̽���      */
            }
        }
    
    } else {                                                            /*  û��ռ����                  */
        if (prwlock->PRWLOCK_uiWPendCounter) {                          /*  �����д�ȴ����߳�          */
            ulReleasNum = (ULONG)prwlock->PRWLOCK_uiWPendCounter;
            API_SemaphoreCRelease(prwlock->PRWLOCK_ulWSemaphore,
                                  ulReleasNum,
                                  LW_NULL);                             /*  �����еĵȴ�д�߳̽���      */
        
        } else if (prwlock->PRWLOCK_uiRPendCounter) {                   /*  ����еȴ������߳�          */
            ulReleasNum = (ULONG)prwlock->PRWLOCK_uiRPendCounter;
            API_SemaphoreCRelease(prwlock->PRWLOCK_ulRSemaphore,
                                  ulReleasNum,
                                  LW_NULL);                             /*  �����еĵȴ����߳̽���      */
        }
    }
    __PX_RWLOCK_UNLOCK(prwlock);                                        /*  ������д��                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_rwlock_getinfo
** ��������: ��ö�д����Ϣ
** �䡡��  : prwlock       ��д�����ƿ�
**           info          ��д����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_rwlock_getinfo (pthread_rwlock_t  *prwlock, pthread_rwlock_info_t  *info)
{
    if ((prwlock == LW_NULL) || (info == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __PX_RWLOCK_LOCK(prwlock);                                          /*  ������д��                  */
    info->owner = prwlock->PRWLOCK_ulOwner;
    info->opcnt = prwlock->PRWLOCK_uiOpCounter;
    info->rpend = prwlock->PRWLOCK_uiRPendCounter;
    info->wpend = prwlock->PRWLOCK_uiWPendCounter;
    __PX_RWLOCK_UNLOCK(prwlock);                                        /*  ������д��                  */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
