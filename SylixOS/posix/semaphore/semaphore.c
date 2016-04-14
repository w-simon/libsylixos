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
** ��   ��   ��: semaphore.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: posix �ź�������. 

** BUG:
2010.01.14  �ж� O_EXCL ��Ҫ��� O_CREAT.
2010.10.06  ����� cancel type �Ĳ���, ���� POSIX ��׼.
            sem_open() ���������ź�����, ϵͳ��ӡ��������Ϣ.
2011.02.23  ���� EINTR �ļ��.
2011.05.30  sem_init() �����ж� psem ���ź�������Ч��.
2012.12.07  ������Դ������.
2012.12.13  ���� SylixOS ֧�ֽ�����Դ����, ���￪ʼ֧�־�̬��ʼ��.
2013.04.01  ���봴�� mode �ı���, Ϊδ��Ȩ�޲����ṩ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_KERNEL
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
#include "../include/px_semaphore.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  macro
*********************************************************************************************************/
#define __PX_UNNAME_SEM_NAME        "?psem_unname"
#define __PX_NAMED_SEM_NAME         "psem_named"
/*********************************************************************************************************
  create option (������� LW_OPTION_OBJECT_GLOBAL ����Ϊ sem ͨ��ԭʼ��Դ���л���)
*********************************************************************************************************/
#if LW_CFG_POSIX_INTER_EN > 0
#define __PX_SEM_OPTION             (LW_OPTION_DEFAULT | LW_OPTION_SIGNAL_INTER | LW_OPTION_OBJECT_GLOBAL)
#else
#define __PX_SEM_OPTION             (LW_OPTION_DEFAULT | LW_OPTION_OBJECT_GLOBAL)
#endif                                                                  /*  LW_CFG_POSIX_INTER_EN > 0   */
/*********************************************************************************************************
  internal sem
*********************************************************************************************************/
typedef struct {
    LW_OBJECT_HANDLE        PSEM_ulSemaphore;                           /*  �ź������                  */
    mode_t                  PSEM_mode;                                  /*  ���� mode                   */
    __PX_NAME_NODE          PSEM_pxnode;                                /*  ���ֽڵ�                    */
} __PX_SEM;
/*********************************************************************************************************
** ��������: __sem_init_invisible
** ��������: posix �ź������δ���. (��̬��ʼ��)
** �䡡��  : psem          �ź������ (����)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void  __sem_init_invisible (sem_t  *psem)
{
    if (psem && (psem->SEM_pvPxSem == LW_NULL)) {
        sem_init(psem, 0, 0);
    }
}
/*********************************************************************************************************
** ��������: sem_init
** ��������: ����һ�������� posix �ź���.
** �䡡��  : psem          �ź������ (����)
**           share         �Ƿ���̹���
**           value         ��ʼֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_init (sem_t  *psem, int  pshared, unsigned int  value)
{
    __PX_SEM    *pxsem;

    if ((psem == LW_NULL) || (value > SEM_VALUE_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)__SHEAP_ALLOC(sizeof(__PX_SEM) + sizeof(LW_RESOURCE_RAW));
    if (pxsem == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        errno = ENOMEM;                                                 /*  ȱ��ϵͳ�ڴ�                */
        return  (PX_ERROR);
    }
    lib_bzero(&pxsem->PSEM_pxnode, sizeof(__PX_NAME_NODE));
    
    pxsem->PSEM_ulSemaphore = API_SemaphoreCCreate(__PX_UNNAME_SEM_NAME, (ULONG)value, 
                                 __ARCH_ULONG_MAX, __PX_SEM_OPTION, 
                                 LW_NULL);                              /*  �����ź���                  */
    if (pxsem->PSEM_ulSemaphore == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pxsem);
        errno = ENOSPC;
        return  (PX_ERROR);
    }
    
    pxsem->PSEM_mode = 0600;                                            /*  ֻ�б����̿���              */
    
    psem->SEM_pvPxSem = (PVOID)pxsem;
    psem->SEM_presraw = (PLW_RESOURCE_RAW)((CHAR *)pxsem + sizeof(__PX_SEM));
    
    __resAddRawHook(psem->SEM_presraw, (VOIDFUNCPTR)sem_destroy, 
                    psem, 0, 0, 0, 0, 0);                               /*  ������Դ������              */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_destroy
** ��������: ɾ��һ�������� posix �ź���.
** �䡡��  : psem          �ź������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_destroy (sem_t  *psem)
{
    __PX_SEM    *pxsem;

    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    if (pxsem->PSEM_pxnode.PXNODE_pcName != LW_NULL) {                  /*  �Ƿ�Ϊ�����ź���            */
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    psem->SEM_pvPxSem = LW_NULL;                                        /*  �����Ч                    */
    
    if (API_SemaphoreCDelete(&pxsem->PSEM_ulSemaphore)) {               /*  ɾ���ź���                  */
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    __resDelRawHook(psem->SEM_presraw);
    
    psem->SEM_presraw = LW_NULL;
    
    __SHEAP_FREE(pxsem);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_open
** ��������: ��һ�������� posix �ź���.
** �䡡��  : name          �ź���������
**           flag          ��ѡ�� (O_CREAT, O_EXCL...)
**           ...
** �䡡��  : �ź������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
sem_t  *sem_open (const char  *name, int  flag, ...)
{
    sem_t              *psem;
    __PX_SEM           *pxsem;
    
    __PX_NAME_NODE     *pxnode;
    va_list             valist;
    size_t              stNameLen;
    
    if (name == LW_NULL) {
        errno = EINVAL;
        return  (SEM_FAILED);
    }
    
    stNameLen = lib_strnlen(name, (NAME_MAX + 1));
    if (stNameLen > NAME_MAX) {
        errno = ENAMETOOLONG;
        return  (SEM_FAILED);
    }
    
    __PX_LOCK();                                                        /*  ��ס posix                  */
    pxnode = __pxnameSeach(name, -1);
    if (pxnode) {                                                       /*  �ҵ�                        */
        if ((flag & O_EXCL) && (flag & O_CREAT)) {
            __PX_UNLOCK();                                              /*  ���� posix                  */
            errno = EEXIST;
            return  (SEM_FAILED);                                       /*  ����ͬ���½�                */
        
        } else {
            pxsem = (__PX_SEM *)pxnode->PXNODE_pvData;                  /*  ����ź������ָ��          */
            
            if ((pxnode->PXNODE_iType != __PX_NAMED_OBJECT_SEM) ||
                !_ObjectClassOK(pxsem->PSEM_ulSemaphore, 
                                _OBJECT_SEM_C)) {                       /*  �Ƿ�Ϊ�����ź���            */
                __PX_UNLOCK();                                          /*  ���� posix                  */
                errno = EEXIST;
                return  (SEM_FAILED);
            }
            
            psem = (sem_t *)__SHEAP_ALLOC(sizeof(sem_t) + sizeof(LW_RESOURCE_RAW));
            if (psem == LW_NULL) {
                __PX_UNLOCK();                                          /*  ���� posix                  */
                errno = ENOMEM;
                return  (SEM_FAILED);
            }
            
            psem->SEM_pvPxSem = pxnode->PXNODE_pvData;                  /*  ����ź������ָ��          */
            psem->SEM_presraw = (PLW_RESOURCE_RAW)((CHAR *)psem + sizeof(sem_t));
            
            API_AtomicInc(&pxnode->PXNODE_atomic);
            __PX_UNLOCK();                                              /*  ���� posix                  */
            
            __resAddRawHook(psem->SEM_presraw, (VOIDFUNCPTR)sem_close, 
                            psem, 0, 0, 0, 0, 0);                       /*  ������Դ������              */
                            
            return  (psem);                                             /*  ���ؾ����ַ                */
        }
    
    } else {                                                            /*  û���ҵ�                    */
        if (flag & O_CREAT) {                                           /*  �½��ź���                  */
            mode_t  mode;
            uint_t  value;
            
            va_start(valist, flag);
            mode  = va_arg(valist, mode_t);
            value = va_arg(valist, uint_t);
            va_end(valist);
            
            if (value > SEM_VALUE_MAX) {                                /*  �Ƿ�ֵ                      */
                __PX_UNLOCK();                                          /*  ���� posix                  */
                errno = EINVAL;
                return  (SEM_FAILED);
            }
            
            pxsem = (__PX_SEM *)__SHEAP_ALLOC(sizeof(__PX_SEM) + stNameLen + 1);
            if (pxsem == LW_NULL) {
                __PX_UNLOCK();                                          /*  ���� posix                  */
                _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
                errno = ENOMEM;                                         /*  ȱ��ϵͳ�ڴ�                */
                return  (SEM_FAILED);
            }
            
            psem = (sem_t *)__SHEAP_ALLOC(sizeof(sem_t) + sizeof(LW_RESOURCE_RAW));
            if (psem == LW_NULL) {
                __SHEAP_FREE(pxsem);
                __PX_UNLOCK();                                          /*  ���� posix                  */
                _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
                errno = ENOMEM;                                         /*  ȱ��ϵͳ�ڴ�                */
                return  (SEM_FAILED);
            }
            
            pxsem->PSEM_ulSemaphore = API_SemaphoreCCreate(__PX_NAMED_SEM_NAME,
                                        (ULONG)value, 
                                        __ARCH_ULONG_MAX, 
                                        __PX_SEM_OPTION, 
                                        LW_NULL);                       /*  �����ź���                  */
            if (pxsem->PSEM_ulSemaphore == LW_OBJECT_HANDLE_INVALID) {
                __SHEAP_FREE(psem);
                __SHEAP_FREE(pxsem);
                __PX_UNLOCK();                                          /*  ���� posix                  */
                errno = ENOSPC;
                return  (SEM_FAILED);
            }
            
            pxsem->PSEM_mode = mode;
            
            pxsem->PSEM_pxnode.PXNODE_pcName = (char *)pxsem + sizeof(__PX_SEM);
            pxsem->PSEM_pxnode.PXNODE_iType  = __PX_NAMED_OBJECT_SEM;
            
            lib_strcpy(pxsem->PSEM_pxnode.PXNODE_pcName, name);
            pxsem->PSEM_pxnode.PXNODE_pvData = (void *)pxsem;           /*  ��������Ϣ                */
            __pxnameAdd(&pxsem->PSEM_pxnode);                           /*  �������ֽڵ��              */
            
            psem->SEM_pvPxSem = (void *)pxsem;                          /*  ����ź������ָ��          */
            psem->SEM_presraw = (PLW_RESOURCE_RAW)((CHAR *)psem + sizeof(sem_t));
            
            API_AtomicInc(&pxsem->PSEM_pxnode.PXNODE_atomic);
            __PX_UNLOCK();                                              /*  ���� posix                  */
            
            __resAddRawHook(psem->SEM_presraw, (VOIDFUNCPTR)sem_close, 
                            psem, 0, 0, 0, 0, 0);                       /*  ������Դ������              */
            
            _DebugFormat(__LOGMESSAGE_LEVEL, "posix semaphore \"%s\" has been create.\r\n", name);
            
            return  (psem);                                             /*  ���ؾ����ַ                */
        
        } else {
            __PX_UNLOCK();                                              /*  ���� posix                  */
            errno = ENOENT;
            return  (SEM_FAILED);
        }
    }
}
/*********************************************************************************************************
** ��������: sem_close
** ��������: �ر�һ�������� posix �ź���.
** �䡡��  : psem          �ź������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_close (sem_t  *psem)
{
    __PX_SEM    *pxsem;

    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    if (pxsem->PSEM_pxnode.PXNODE_pcName == LW_NULL) {                  /*  �Ƿ�Ϊ�����ź���            */
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (!_ObjectClassOK(pxsem->PSEM_ulSemaphore, _OBJECT_SEM_C)) {      /*  ����Ƿ�Ϊ�����ź���        */
        errno = EINVAL;
        return  (PX_ERROR);
    }

    __PX_LOCK();                                                        /*  ��ס posix                  */
    if (API_AtomicGet(&pxsem->PSEM_pxnode.PXNODE_atomic) > 0) {
        API_AtomicDec(&pxsem->PSEM_pxnode.PXNODE_atomic);               /*  ��������ʹ�ü���            */
    }
    __PX_UNLOCK();                                                      /*  ���� posix                  */

    __resDelRawHook(psem->SEM_presraw);

    __SHEAP_FREE(psem);                                                 /*  �ͷž��                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_unlink
** ��������: ɾ��һ�������� posix �ź���.
** �䡡��  : psem          �ź������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_unlink (const char *name)
{
    __PX_NAME_NODE *pxnode;
    __PX_SEM       *pxsem;
    
    if (name == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    __PX_LOCK();                                                        /*  ��ס posix                  */
    pxnode = __pxnameSeach(name, -1);
    if (pxnode) {
        pxsem = (__PX_SEM *)pxnode->PXNODE_pvData;
        if (!_ObjectClassOK(pxsem->PSEM_ulSemaphore, _OBJECT_SEM_C)) {  /*  ����Ƿ�Ϊ�����ź���        */
            __PX_UNLOCK();                                              /*  ���� posix                  */
            errno = EINVAL;
            return  (PX_ERROR);
        }
        if (API_AtomicGet(&pxnode->PXNODE_atomic) > 0) {
            __PX_UNLOCK();                                              /*  ���� posix                  */
            errno = EBUSY;
            return  (PX_ERROR);
        }
        if (API_SemaphoreDelete(&pxsem->PSEM_ulSemaphore)) {
            __PX_UNLOCK();                                              /*  ���� posix                  */
            return  (PX_ERROR);                                         /*  errno set by SemaphoreDelete*/
        }
        __pxnameDel(name);
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "posix semaphore \"%s\" has been delete.\r\n", name);
        
        __SHEAP_FREE(pxsem);                                            /*  �ͷŻ���                    */
        
        __PX_UNLOCK();                                                  /*  ���� posix                  */
        return  (ERROR_NONE);
    
    } else {
        __PX_UNLOCK();                                                  /*  ���� posix                  */
        errno = ENOENT;
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: sem_wait
** ��������: �ȴ�һ�� posix �ź���.
** �䡡��  : psem          �ź������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_wait (sem_t  *psem)
{
    __PX_SEM   *pxsem;
    
    __sem_init_invisible(psem);

    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */

    if (API_SemaphoreCPend(pxsem->PSEM_ulSemaphore, LW_OPTION_WAIT_INFINITE)) {
        if (errno != EINTR) { 
            errno = EINVAL;
        }
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_trywait
** ��������: �ȴ�һ�� posix �ź��� (������).
** �䡡��  : psem          �ź������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_trywait (sem_t  *psem)
{
    ULONG       ulError;
    __PX_SEM   *pxsem;
    
    __sem_init_invisible(psem);
    
    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;

    ulError = API_SemaphoreCTryPend(pxsem->PSEM_ulSemaphore);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = EAGAIN;
        return  (PX_ERROR);
    
    } else if (ulError) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_timedwait
** ��������: �ȴ�һ�� posix �ź��� (���г�ʱ������).
** �䡡��  : psem          �ź������
**           abs_timeout   ��ʱʱ�� (ע��: �����Ǿ���ʱ��, ��һ��ȷ������ʷʱ������: 2009.12.31 15:36:04)
                           ���߾���������Ǻ�ˬ, Ӧ���ټ�һ���������Եȴ����ʱ��.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_timedwait (sem_t  *psem, const struct timespec *abs_timeout)
{
    ULONG               ulTimeout;
    ULONG               ulError;
    struct timespec     tvNow;
    struct timespec     tvWait = {0, 0};
    __PX_SEM           *pxsem;
    
    __sem_init_invisible(psem);
    
    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((abs_timeout == LW_NULL)    || 
        (abs_timeout->tv_nsec <  0) ||
        (abs_timeout->tv_nsec >= __TIMEVAL_NSEC_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    lib_clock_gettime(CLOCK_REALTIME, &tvNow);                          /*  ��õ�ǰϵͳʱ��            */
    if (__timespecLeftTime(&tvNow, abs_timeout)) {
        tvWait = *abs_timeout;
        __timespecSub(&tvWait, &tvNow);                                 /*  �����뵱ǰ�ȴ���ʱ����    */
    }
    /*
     *  ע��: �� tvWait ����ulong tick��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.
     */
    ulTimeout = __timespecToTick(&tvWait);                              /*  �任Ϊ tick                 */
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    ulError = API_SemaphoreCPend(pxsem->PSEM_ulSemaphore, ulTimeout);   /*  �ȴ��ź���                  */
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = ETIMEDOUT;
        return  (PX_ERROR);
        
    } else if (ulError == EINTR) {
        return  (PX_ERROR);
        
    } else if (ulError) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_reltimedwait_np
** ��������: �ȴ�һ�� posix �ź��� (���г�ʱ������).
** �䡡��  : psem          �ź������
**           rel_timeout   ��Գ�ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API  
int  sem_reltimedwait_np (sem_t  *psem, const struct timespec *rel_timeout)
{
    ULONG               ulTimeout;
    ULONG               ulError;
    __PX_SEM           *pxsem;
    
    __sem_init_invisible(psem);
    
    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((rel_timeout == LW_NULL)    || 
        (rel_timeout->tv_nsec <  0) ||
        (rel_timeout->tv_nsec >= __TIMEVAL_NSEC_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    /*
     *  ע��: �� rel_timeout ����ulong tick��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.
     */
    ulTimeout = __timespecToTick(rel_timeout);                          /*  �任Ϊ tick                 */
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    ulError = API_SemaphoreCPend(pxsem->PSEM_ulSemaphore, ulTimeout);   /*  �ȴ��ź���                  */
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = ETIMEDOUT;
        return  (PX_ERROR);
        
    } else if (ulError == EINTR) {
        return  (PX_ERROR);
    
    } else if (ulError) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** ��������: sem_post
** ��������: �ͷ�һ�� posix �ź���.
** �䡡��  : psem          �ź������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_post (sem_t  *psem)
{
    __PX_SEM    *pxsem;
    
    __sem_init_invisible(psem);

    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;

    if (API_SemaphoreCPost(pxsem->PSEM_ulSemaphore)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_getvalue
** ��������: ���һ�� posix �ź�����ǰ����ֵ.
** �䡡��  : psem          �ź������
**           pivalue       ��ǰ����ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_getvalue (sem_t  *psem, int  *pivalue)
{
    ULONG       ulValue;
    __PX_SEM   *pxsem;
    
    __sem_init_invisible(psem);
    
    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (pivalue == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    if (API_SemaphoreCStatus(pxsem->PSEM_ulSemaphore, &ulValue, LW_NULL, LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    *pivalue = (int)ulValue;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_flush
** ��������: �ͷŵ����������ڴ��ź����ϵ��߳�.
** �䡡��  : psem          �ź������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  sem_flush (sem_t  *psem)
{
    __PX_SEM   *pxsem;
    
    __sem_init_invisible(psem);
    
    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    if (API_SemaphoreCFlush(pxsem->PSEM_ulSemaphore, LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_getinfo
** ��������: ����ź�����Ϣ.
** �䡡��  : psem          �ź������
**           info          �ź�����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_getinfo (sem_t  *psem, sem_info_t  *info)
{
    __PX_SEM   *pxsem;
    
    __sem_init_invisible(psem);
    
    if ((psem == LW_NULL) || 
        (psem->SEM_pvPxSem == LW_NULL) ||
        (info == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    if (API_SemaphoreCStatus(pxsem->PSEM_ulSemaphore, 
                             &info->SEMINFO_ulCounter,
                             &info->SEMINFO_ulOption,
                             &info->SEMINFO_ulBlockNum)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sem_show
** ��������: ��ʾ�ź�����Ϣ.
** �䡡��  : psem          �ź������
**           level         ��ʾ�ȼ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sem_show (sem_t  *psem, int  level)
{
    __PX_SEM   *pxsem;
    
    (VOID)level;
    
    __sem_init_invisible(psem);
    
    if ((psem == LW_NULL) || (psem->SEM_pvPxSem == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxsem = (__PX_SEM *)psem->SEM_pvPxSem;
    
    API_SemaphoreShow(pxsem->PSEM_ulSemaphore);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
