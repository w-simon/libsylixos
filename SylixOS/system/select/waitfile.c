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
** ��   ��   ��: waitfile.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ���� API. �ṩ�Ե����ļ��Ŀ��ٲ���.

** BUG
2007.11.20  �� pselctx->SELCTX_bPendedOnSelect = LW_TRUE; ��ǰ, ��ʹ FIOSELECT ���е���;�̱߳�ɾ��ʱ
            ����ʹ delete hook ��������, �������� WAKE NODE ɾ��.
2007.11.20  ����� context �� Orig??? �ļ����Ĳ���, �Ա�֤ delete hook ���Է�������.
2007.12.11  ��������ʱ,����ֵӦ��Ϊ -1.
2007.12.22  ����ע��, �޸� _DebugHandle() ������ַ���.
2008.03.01  wait???() ��������ļ����������ж���©��.
2009.07.17  ��������ļ��Ų���Ӧ����ǰ, ��֤ delete hook �ܹ�������ɾ�������Ľڵ�.
2011.03.11  ȷ����ʱ���Է��ض�Ӧ�� errno ���.
2011.08.15  �ش����: ������ posix ����ĺ����Ժ�����ʽ(�Ǻ�)����.
2017.08.18  �ȴ�ʱ����Ӿ�ȷ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
/*********************************************************************************************************
  select ��ϵͳ�ڲ���������
*********************************************************************************************************/
VOID  __selFdsetInit(INT               iWidthInBytes,
                     fd_set           *pfdsetRead,
                     fd_set           *pfdsetWrite,
                     fd_set           *pfdsetExcept,
                     LW_SEL_CONTEXT   *pselctx);
/*********************************************************************************************************
** ��������: waitread
** ��������: select() ����,�ȴ������ļ��ɶ�.
** �䡡��  : iFd               �ļ�������
**           ptmvalTO          �ȴ���ʱʱ��, LW_NULL ��ʾ��Զ�ȴ�.
** �䡡��  : �����ȴ������ļ���, Ϊ 1 ��ʾ�ļ����Զ�, Ϊ 0 ��ʾ��ʱ ,���󷵻� PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       ��������֧��
**           errno == ERROR_IO_SELECT_CONTEXT                   �̲߳����� context
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 �ȴ���ʱ
**           errno == ERROR_KERNEL_IN_ISR                       ���ж��е���
**           errno == ERROR_IOS_INVALID_FILE_DESCRIPTOR         �ļ���������Ч.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     waitread (INT  iFd, struct timeval   *ptmvalTO)
{
    REGISTER INT                 iIsOk  = ERROR_NONE;                   /*  ��ʼ��Ϊû�д���            */
    REGISTER INT                 iWidth = iFd + 1;                      /*  iFd + 1                     */
    REGISTER INT                 iWidthInBytes;                         /*  ��Ҫ����λ��ռ�����ֽ�    */
    REGISTER ULONG               ulWaitTime;                            /*  �ȴ�ʱ��                    */
    REGISTER LW_SEL_CONTEXT     *pselctx;
             PLW_CLASS_TCB       ptcbCur;
             struct timespec     tvTO;

             fd_set              fdsetRead;
             LW_SEL_WAKEUPNODE   selwunNode;                            /*  ���ɵ� NODE ģ��            */
             ULONG               ulError;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    if (iFd > (FD_SETSIZE - 1) || iFd < 0) {                            /*  �ļ��Ŵ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate..\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ���������Ч              */
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pselctx = ptcbCur->TCB_pselctxContext;
    
    if (!pselctx) {                                                     /*  û�� select context         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no select context.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_CONTEXT);
        return  (PX_ERROR);
    }

    iWidthInBytes = __HOWMANY(iWidth, NFDBITS) * sizeof(fd_mask);       /*  ��Ҫ����λ��ռ�����ֽ�    */
    
    FD_ZERO(&fdsetRead);                                                /*  ����ļ���                  */
    FD_SET(iFd, &fdsetRead);                                            /*  ָ���ļ���λ                */
    
    __selFdsetInit(iWidthInBytes, &fdsetRead,                           /*  ���� OrigRead �ļ�λ        */
                   LW_NULL, LW_NULL, pselctx);                          /*  __selTaskDeleteHook ʹ��    */
                   
    FD_CLR(iFd, &fdsetRead);                                            /*  ����ļ���                  */

    if (!ptmvalTO) {                                                    /*  ����ȴ�ʱ��                */
        ulWaitTime = LW_OPTION_WAIT_INFINITE;                           /*  ���޵ȴ�                    */

    } else {
        LW_TIMEVAL_TO_TIMESPEC(ptmvalTO, &tvTO);
        ulWaitTime = LW_TS_TIMEOUT_TICK(LW_TRUE, &tvTO);
    }
    
    pselctx->SELCTX_pfdsetReadFds   = &fdsetRead;
    pselctx->SELCTX_pfdsetWriteFds  = LW_NULL;                          /*  �����û�������ַ            */
    pselctx->SELCTX_pfdsetExceptFds = LW_NULL;
    
    API_SemaphoreBClear(pselctx->SELCTX_hSembWakeup);                   /*  ����ź���                  */
    
    selwunNode.SELWUN_hThreadId  = API_ThreadIdSelf();
    selwunNode.SELWUN_seltypType = SELREAD;
    selwunNode.SELWUN_iFd        = iFd;
    
    pselctx->SELCTX_iWidth = iWidth;                                    /*  ��¼����ļ���              */
    pselctx->SELCTX_bPendedOnSelect = LW_TRUE;                          /*  ��Ҫ delete hook ��� NODE  */
    
    iIsOk = ioctl(iFd, FIOSELECT, (LONG)&selwunNode);                   /*  FIOSELECT                   */
    if (iIsOk != ERROR_NONE) {
        ULONG   ulError = API_GetLastError();
        iIsOk  = ioctl(iFd, FIOUNSELECT, (LONG)&selwunNode);            /*  FIOUNSELECT                 */
        if (ulError == ERROR_IO_UNKNOWN_REQUEST) {
            _ErrorHandle(ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER);          /*  ��������֧��              */
        }
        pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                     /*  �����������                */
        return  (PX_ERROR);                                             /*  ����                        */
    }

    ulError = API_SemaphoreBPend(pselctx->SELCTX_hSembWakeup,
                                 ulWaitTime);                           /*  ��ʼ�ȴ�                    */
    
    iIsOk = ioctl(iFd, FIOUNSELECT, (LONG)&selwunNode);                 /*  FIOUNSELECT                 */
    
    pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                         /*  �����������                */
    
    if (iIsOk != ERROR_NONE) {
        return  (PX_ERROR);                                             /*  ���ִ���                    */
    } else {
        _ErrorHandle(ulError);
    }
    
    if (FD_ISSET(iFd, &fdsetRead)) {                                    /*  ����ļ��Ƿ�ɶ�            */
        return  (1);
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** ��������: waitwrite
** ��������: select() ����,�ȴ������ļ���д.
** �䡡��  : iFd               �ļ�������
**           ptmvalTO          �ȴ���ʱʱ��, LW_NULL ��ʾ��Զ�ȴ�.
** �䡡��  : �����ȴ������ļ���, Ϊ 1 ��ʾ�ļ����Զ�, Ϊ 0 ��ʾ��ʱ ,���󷵻� PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       ��������֧��
**           errno == ERROR_IO_SELECT_CONTEXT                   �̲߳����� context
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 �ȴ���ʱ
**           errno == ERROR_KERNEL_IN_ISR                       ���ж��е���
**           errno == ERROR_IOS_INVALID_FILE_DESCRIPTOR         �ļ���������Ч.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     waitwrite (INT  iFd, struct timeval   *ptmvalTO)
{
    REGISTER INT                 iIsOk  = ERROR_NONE;                   /*  ��ʼ��Ϊû�д���            */
    REGISTER INT                 iWidth = iFd + 1;                      /*  iFd + 1                     */
    REGISTER INT                 iWidthInBytes;                         /*  ��Ҫ����λ��ռ�����ֽ�    */
    REGISTER ULONG               ulWaitTime;                            /*  �ȴ�ʱ��                    */
    REGISTER LW_SEL_CONTEXT     *pselctx;
             PLW_CLASS_TCB       ptcbCur;
             struct timespec     tvTO;
    
             fd_set              fdsetWrite;
             LW_SEL_WAKEUPNODE   selwunNode;                            /*  ���ɵ� NODE ģ��            */
             ULONG               ulError;
             
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    if (iFd > (FD_SETSIZE - 1) || iFd < 0) {                            /*  �ļ��Ŵ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate..\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ���������Ч              */
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pselctx = ptcbCur->TCB_pselctxContext;
    
    if (!pselctx) {                                                     /*  û�� select context         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no select context.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_CONTEXT);
        return  (PX_ERROR);
    }

    iWidthInBytes = __HOWMANY(iWidth, NFDBITS) * sizeof(fd_mask);       /*  ��Ҫ����λ��ռ�����ֽ�    */
    
    FD_ZERO(&fdsetWrite);                                               /*  ����ļ���                  */
    FD_SET(iFd, &fdsetWrite);                                           /*  ָ���ļ���λ                */
    
    __selFdsetInit(iWidthInBytes, LW_NULL,                              /*  ���� OrigRead �ļ�λ        */
                   &fdsetWrite, LW_NULL, pselctx);                      /*  __selTaskDeleteHook ʹ��    */
    
    FD_CLR(iFd, &fdsetWrite);                                           /*  ����ļ���                  */
    
    if (!ptmvalTO) {                                                    /*  ����ȴ�ʱ��                */
        ulWaitTime = LW_OPTION_WAIT_INFINITE;                           /*  ���޵ȴ�                    */

    } else {
        LW_TIMEVAL_TO_TIMESPEC(ptmvalTO, &tvTO);
        ulWaitTime = LW_TS_TIMEOUT_TICK(LW_TRUE, &tvTO);
    }
    
    pselctx->SELCTX_pfdsetReadFds   = LW_NULL;
    pselctx->SELCTX_pfdsetWriteFds  = &fdsetWrite;                      /*  �����û�������ַ            */
    pselctx->SELCTX_pfdsetExceptFds = LW_NULL;
    
    API_SemaphoreBClear(pselctx->SELCTX_hSembWakeup);                   /*  ����ź���                  */
    
    selwunNode.SELWUN_hThreadId  = API_ThreadIdSelf();
    selwunNode.SELWUN_seltypType = SELWRITE;
    selwunNode.SELWUN_iFd        = iFd;
    
    pselctx->SELCTX_iWidth = iWidth;                                    /*  ��¼����ļ���              */
    pselctx->SELCTX_bPendedOnSelect = LW_TRUE;                          /*  ��Ҫ delete hook ��� NODE  */
    
    iIsOk = ioctl(iFd, FIOSELECT, (LONG)&selwunNode);                   /*  FIOSELECT                   */
    if (iIsOk != ERROR_NONE) {
        ULONG   ulError = API_GetLastError();
        iIsOk  = ioctl(iFd, FIOUNSELECT, (LONG)&selwunNode);            /*  FIOUNSELECT                 */
        if (ulError == ERROR_IO_UNKNOWN_REQUEST) {
            _ErrorHandle(ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER);          /*  ��������֧��              */
        }
        pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                     /*  �����������                */
        return  (PX_ERROR);                                             /*  ����                        */
    }
    
    ulError = API_SemaphoreBPend(pselctx->SELCTX_hSembWakeup,
                                 ulWaitTime);                           /*  ��ʼ�ȴ�                    */
    
    iIsOk = ioctl(iFd, FIOUNSELECT, (LONG)&selwunNode);                 /*  FIOUNSELECT                 */
    
    pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                         /*  �����������                */
    
    if (iIsOk != ERROR_NONE) {
        return  (PX_ERROR);                                             /*  ���ִ���                    */
    } else {
        _ErrorHandle(ulError);
    }
    
    if (FD_ISSET(iFd, &fdsetWrite)) {                                   /*  ����ļ��Ƿ�ɶ�            */
        return  (1);
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** ��������: waitexcept
** ��������: select() ����,�ȴ������ļ��쳣.
** �䡡��  : iFd               �ļ�������
**           ptmvalTO          �ȴ���ʱʱ��, LW_NULL ��ʾ��Զ�ȴ�.
** �䡡��  : �����ȴ������ļ���, Ϊ 1 ��ʾ�ļ����Զ�, Ϊ 0 ��ʾ��ʱ ,���󷵻� PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       ��������֧��
**           errno == ERROR_IO_SELECT_CONTEXT                   �̲߳����� context
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 �ȴ���ʱ
**           errno == ERROR_KERNEL_IN_ISR                       ���ж��е���
**           errno == ERROR_IOS_INVALID_FILE_DESCRIPTOR         �ļ���������Ч.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     waitexcept (INT  iFd, struct timeval   *ptmvalTO)
{
    REGISTER INT                 iIsOk  = ERROR_NONE;                   /*  ��ʼ��Ϊû�д���            */
    REGISTER INT                 iWidth = iFd + 1;                      /*  iFd + 1                     */
    REGISTER INT                 iWidthInBytes;                         /*  ��Ҫ����λ��ռ�����ֽ�    */
    REGISTER ULONG               ulWaitTime;                            /*  �ȴ�ʱ��                    */
    REGISTER LW_SEL_CONTEXT     *pselctx;
             PLW_CLASS_TCB       ptcbCur;
             struct timespec     tvTO;
    
             fd_set              fdsetExcept;
             LW_SEL_WAKEUPNODE   selwunNode;                            /*  ���ɵ� NODE ģ��            */
             ULONG               ulError;
             
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    if (iFd > (FD_SETSIZE - 1) || iFd < 0) {                            /*  �ļ��Ŵ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate..\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ���������Ч              */
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pselctx = ptcbCur->TCB_pselctxContext;
    
    if (!pselctx) {                                                     /*  û�� select context         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no select context.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_CONTEXT);
        return  (PX_ERROR);
    }

    iWidthInBytes = __HOWMANY(iWidth, NFDBITS) * sizeof(fd_mask);       /*  ��Ҫ����λ��ռ�����ֽ�    */
    
    FD_ZERO(&fdsetExcept);                                              /*  ����ļ���                  */
    FD_SET(iFd, &fdsetExcept);                                          /*  ָ���ļ���λ                */
    
    __selFdsetInit(iWidthInBytes, LW_NULL,                              /*  ���� OrigRead �ļ�λ        */
                   LW_NULL, &fdsetExcept, pselctx);                     /*  __selTaskDeleteHook ʹ��    */
    
    FD_CLR(iFd, &fdsetExcept);                                          /*  ����ļ���                  */
    
    if (!ptmvalTO) {                                                    /*  ����ȴ�ʱ��                */
        ulWaitTime = LW_OPTION_WAIT_INFINITE;                           /*  ���޵ȴ�                    */

    } else {
        LW_TIMEVAL_TO_TIMESPEC(ptmvalTO, &tvTO);
        ulWaitTime = LW_TS_TIMEOUT_TICK(LW_TRUE, &tvTO);
    }
    
    pselctx->SELCTX_pfdsetReadFds   = LW_NULL;
    pselctx->SELCTX_pfdsetWriteFds  = LW_NULL;                          /*  �����û�������ַ            */
    pselctx->SELCTX_pfdsetExceptFds = &fdsetExcept;
    
    API_SemaphoreBClear(pselctx->SELCTX_hSembWakeup);                   /*  ����ź���                  */
    
    selwunNode.SELWUN_hThreadId  = API_ThreadIdSelf();
    selwunNode.SELWUN_seltypType = SELEXCEPT;
    selwunNode.SELWUN_iFd        = iFd;
    
    pselctx->SELCTX_iWidth = iWidth;                                    /*  ��¼����ļ���              */
    pselctx->SELCTX_bPendedOnSelect = LW_TRUE;                          /*  ��Ҫ delete hook ��� NODE  */
    
    iIsOk = ioctl(iFd, FIOSELECT, (LONG)&selwunNode);                   /*  FIOSELECT                   */
    if (iIsOk != ERROR_NONE) {
        ULONG   ulError = API_GetLastError();
        iIsOk  = ioctl(iFd, FIOUNSELECT, (LONG)&selwunNode);            /*  FIOUNSELECT                 */
        if (ulError == ERROR_IO_UNKNOWN_REQUEST) {
            _ErrorHandle(ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER);          /*  ��������֧��              */
        }
        pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                     /*  �����������                */
        return  (PX_ERROR);                                             /*  ����                        */
    }
    
    ulError = API_SemaphoreBPend(pselctx->SELCTX_hSembWakeup,
                                 ulWaitTime);                           /*  ��ʼ�ȴ�                    */
    
    iIsOk = ioctl(iFd, FIOUNSELECT, (LONG)&selwunNode);                 /*  FIOUNSELECT                 */
    
    pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                         /*  �����������                */
    
    if (iIsOk != ERROR_NONE) {
        return  (PX_ERROR);                                             /*  ���ִ���                    */
    } else {
        _ErrorHandle(ulError);
    }
    
    if (FD_ISSET(iFd, &fdsetExcept)) {                                  /*  ����ļ��Ƿ�ɶ�            */
        return  (1);
    } else {
        return  (0);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
