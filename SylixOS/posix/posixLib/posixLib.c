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
** ��   ��   ��: posixLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: posix �ڲ���.

** BUG:
2011.02.26  ����� aio �ĳ�ʼ��.
2011.03.11  ����� syslog �ĳ�ʼ��.
2013.05.02  _posixCtxDelete() ʹ�� syshook ����, ����ʹ�� cleanup.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/posixLib.h"                                        /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
VOID  __procFsPosixInfoInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#if LW_CFG_SIGNAL_EN > 0
VOID  _posixAioInit(VOID);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */

VOID  _posixPSemInit(VOID);
VOID  _posixPMutexInit(VOID);
VOID  _posixPRWLockInit(VOID);
VOID  _posixPCondInit(VOID);
VOID  _posixSyslogInit(VOID);
/*********************************************************************************************************
  posix lock
*********************************************************************************************************/
LW_OBJECT_HANDLE        _G_ulPosixLock = 0;
/*********************************************************************************************************
** ��������: _posixCtxDelete
** ��������: ɾ�� posix �߳�������
** �䡡��  : ulId          �߳� id
**           pvRetVal      ����ֵ
**           ptcb          �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _posixCtxDelete (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcb)
{
    if (ptcb->TCB_pvPosixContext) {
        __SHEAP_FREE(ptcb->TCB_pvPosixContext);
        ptcb->TCB_pvPosixContext = LW_NULL;                             /*  һ����������!               */
                                                                        /*  �����߳�����������ɴ���    */
    }
}
/*********************************************************************************************************
** ��������: _posixCtxCreate
** ��������: ���� posix �߳�������
** �䡡��  : ptcb          �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _posixCtxCreate (PLW_CLASS_TCB   ptcb)
{
    API_ThreadSafe();
    ptcb->TCB_pvPosixContext =  (PVOID)__SHEAP_ALLOC(sizeof(__PX_CONTEXT));
    if (ptcb->TCB_pvPosixContext == LW_NULL) {
        API_ThreadUnsafe();                                             /*  ȱ���ڴ�                    */
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    API_ThreadUnsafe();
    
    lib_bzero(ptcb->TCB_pvPosixContext, sizeof(__PX_CONTEXT));          /*  ��������                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _posixCtxGet
** ��������: ��ȡ posix �߳������� (���򴴽�)
** �䡡��  : ptcb          �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
__PX_CONTEXT  *_posixCtxGet (PLW_CLASS_TCB   ptcb)
{
    if (ptcb == LW_NULL) {
        errno = EINVAL;
        return  (LW_NULL);
    }

    if (ptcb->TCB_pvPosixContext == LW_NULL) {
        _posixCtxCreate(ptcb);                                          /*  ����                        */
    }
    
    return  ((__PX_CONTEXT *)ptcb->TCB_pvPosixContext);
}
/*********************************************************************************************************
** ��������: _posixCtxTryGet
** ��������: ��ȡ posix �߳�������
** �䡡��  : ptcb          �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
__PX_CONTEXT  *_posixCtxTryGet (PLW_CLASS_TCB   ptcb)
{
    if (ptcb == LW_NULL) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    return  ((__PX_CONTEXT *)ptcb->TCB_pvPosixContext);
}
/*********************************************************************************************************
** ��������: API_PosixInit
** ��������: ��ʼ�� posix ϵͳ (���ϵͳ֧�� proc �ļ�ϵͳ, �������� proc �ļ�ϵͳ��װ֮��!).
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_PosixInit (VOID)
{
#if LW_CFG_SHELL_EN > 0
    VOID  __tshellPosixInit(VOID);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

    static BOOL         bIsInit = LW_FALSE;

    if (_G_ulPosixLock == 0) {
        _G_ulPosixLock =  API_SemaphoreMCreate("px_lock", LW_PRIO_DEF_CEILING, 
                                               LW_OPTION_INHERIT_PRIORITY | 
                                               LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
    
    if (bIsInit) {
        return;
    }
    
    API_SystemHookAdd(_posixCtxDelete, LW_OPTION_THREAD_DELETE_HOOK);   /*  posix �߳���������          */
    
#if LW_CFG_PROCFS_EN > 0
    __procFsPosixInfoInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#if LW_CFG_POSIX_AIO_EN > 0
    _posixAioInit();
#endif                                                                  /*  LW_CFG_POSIX_AIO_EN > 0     */

    _posixPSemInit();
    _posixPMutexInit();
    _posixPRWLockInit();
    _posixPCondInit();
    _posixSyslogInit();
    
#if LW_CFG_SHELL_EN > 0
    __tshellPosixInit();                                                /*  ��ʼ�� shell ����           */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    bIsInit = LW_TRUE;
}
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
