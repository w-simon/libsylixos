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
** ��   ��   ��: mqueue.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: posix ��Ϣ���й���. 

** BUG:
2010.01.13  ʹ�ü����ź������ƶ�дͬ��.
2010.01.14  �ж� O_EXCL ��Ҫ��� O_CREAT.
2010.10.06  ����� cancel type �Ĳ���, ���� POSIX ��׼.
            mq_open() ����������Ϣ���к�, ϵͳ��ӡ��������Ϣ.
2011.02.17  �޸�ע��.
2011.02.22  mq_notify() ���� _sigPendInit() ��ʼ��.
2011.02.23  ���� EINTR �ļ��.
2011.02.26  ʹ�� sigevent ��Ϊ�����źŲ���.
2012.12.07  �� mqueue ������Դ������.
2013.03.18  �Ľ� mqueue ��ԭ�Ӳ�����.
2013.04.01  ���봴�� mode �ı���, Ϊδ��Ȩ�޲����ṩ����.
2013.12.12  ʹ�� archFindLsb() ��ȷ����Ϣ���ȼ�.
2014.05.30  ʹ�� ROUND_UP �������.
2015.07.24  ���� mq_send ���ȼ�Ϊ 0 ʱ�Ĵ���.
2016.04.13  ���� GJB7714 ��� API ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../include/px_mqueue.h"
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  ע��: message queue ��ͬ���ź���, �ź��� __PX_NAME_NODE ��ֱ�ӱ�������ź������, ��Ϊ open ʱ�� flag ��
        �������ź�������û���κ�Ӱ��, (�û���������ź������ƿ�!)
        
        ���� message queue ������! open ʱ�� flag ��Ӱ��֮��Ĳ���, ����, �� message queue ��, ���û����
        mqt_t ����Ϣ���п��ƿ����, mqt_t ���������� flag Ҳ����, ��ͬ�ľ������ָ��ͬһ����Ϣ����, ��:
        ����ʱ�� flag ���Բ�ͬ! 
        
        ���ϵ���������� message queue �� semaphore ���ھ���Ϳ��ƿ�Ĺ��������в�ͬ.
*********************************************************************************************************/
/*********************************************************************************************************
  create option (������� LW_OPTION_OBJECT_GLOBAL ��Ϊ mqueue �Ļ���ͨ��ԭʼ��Դ���л���)
*********************************************************************************************************/
#if LW_CFG_POSIX_INTER_EN > 0
#define __PX_MQUEUE_OPTION          (LW_OPTION_DEFAULT | LW_OPTION_SIGNAL_INTER | LW_OPTION_OBJECT_GLOBAL)
#else
#define __PX_MQUEUE_OPTION          (LW_OPTION_DEFAULT | LW_OPTION_OBJECT_GLOBAL)
#endif                                                                  /*  LW_CFG_POSIX_INTER_EN > 0   */
/*********************************************************************************************************
  ���ȼ�ת��
*********************************************************************************************************/
#define __PX_MQ_PRIO_CONVERT(prio)  ((MQ_PRIO_MAX - 1) - (prio))
/*********************************************************************************************************
  һ����Ϣ�ڵ�
*********************************************************************************************************/

typedef struct {
    LW_LIST_RING        PMSGN_ringManage;                               /*  ��Ϣ���λ���                */
    size_t              PMSGN_stMsgLen;                                 /*  ��Ϣ����                    */
} __PX_MSG_NODE;

/*********************************************************************************************************
  ��Ϣ�����ڴ���ƿ�
*********************************************************************************************************/

typedef struct {
    caddr_t             PMSGM_pcMem;                                    /*  ��Ϣ�����ڴ�                */
    size_t              PMSGM_stBytesPerMsg;                            /*  ÿһ����Ϣ������ռ���ֽ���  */
    LW_LIST_RING_HEADER PMSGM_pringFreeList;                            /*  ������Ϣ�ڴ�                */
} __PX_MSG_MEM;

/*********************************************************************************************************
  ��Ϣ���� notify 
*********************************************************************************************************/

typedef struct {
    LW_OBJECT_HANDLE    PMSGNTF_ulThreadId;                             /*  �߳� id                     */
    struct sigevent     PMSGNTF_sigevent;                               /*  notify ����                 */
} __PX_MSG_NOTIFY;

/*********************************************************************************************************
  ��Ϣ���п��ƿ�
*********************************************************************************************************/

typedef struct {
    mq_attr_t           PMSG_mqattr;                                    /*  ��Ϣ��������                */
    mode_t              PMSG_mode;                                      /*  ���� mode                   */
    LW_OBJECT_HANDLE    PMSG_ulReadSync;                                /*  ��ͬ��                      */
    LW_OBJECT_HANDLE    PMSG_ulWriteSync;                               /*  дͬ��                      */
    LW_OBJECT_HANDLE    PMSG_ulMutex;                                   /*  ������                      */
    __PX_MSG_NOTIFY     PMSG_pmsgntf;                                   /*  notify �ź����             */
    __PX_MSG_MEM        PMSG_pmsgmem;                                   /*  ��Ϣ�����ڴ滺��            */
    uint32_t            PMSG_u32Map;                                    /*  λͼ�� (�����ȼ��㷨��ͬ)   */
    LW_LIST_RING_HEADER PMSG_pringMsg[MQ_PRIO_MAX];                     /*  ÿ�����ȼ�����Ϣ����        */
    __PX_NAME_NODE      PMSG_pxnode;                                    /*  ���ֽڵ�                    */
} __PX_MSG;

/*********************************************************************************************************
  mqueue file
*********************************************************************************************************/

typedef struct {
    __PX_MSG           *PMSGF_pmg;                                      /*  ��Ϣ����                    */
    int                 PMSGF_iFlag;                                    /*  mq_open() ʱ�� flag         */
    LW_RESOURCE_RAW     PMSGF_resraw;                                   /*  ��Դ����ڵ�                */
} __PX_MSG_FILE;

/*********************************************************************************************************
  ����
*********************************************************************************************************/
mq_attr_t  mq_attr_default = {O_RDWR, 128, 64, 0};

#define __PX_MQ_LOCK(pmq)           API_SemaphoreMPend(pmq->PMSG_ulMutex, LW_OPTION_WAIT_INFINITE)
#define __PX_MQ_UNLOCK(pmq)         API_SemaphoreMPost(pmq->PMSG_ulMutex)

/*********************************************************************************************************
  ��дͬ��
*********************************************************************************************************/

#define __PX_MQ_RWAIT(pmq, to)      API_SemaphoreBPend(pmq->PMSG_ulReadSync, to)
#define __PX_MQ_RPOST(pmq)          API_SemaphoreBPost(pmq->PMSG_ulReadSync)

#define __PX_MQ_WWAIT(pmq, to)      API_SemaphoreBPend(pmq->PMSG_ulWriteSync, to)
#define __PX_MQ_WPOST(pmq)          API_SemaphoreBPost(pmq->PMSG_ulWriteSync)

/*********************************************************************************************************
** ��������: __mqueueSignalNotify
** ��������: ��Ϣ���з���һ���ź�֪ͨ�ɶ�.
** �䡡��  : pmq           ��Ϣ���п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

static VOID  __mqueueSignalNotify (__PX_MSG  *pmq)
{
    if (pmq->PMSG_pmsgntf.PMSGNTF_ulThreadId == LW_OBJECT_HANDLE_INVALID) {
        return;                                                         /*  û���������                */
    }
    
    _doSigEvent(pmq->PMSG_pmsgntf.PMSGNTF_ulThreadId,
                &pmq->PMSG_pmsgntf.PMSGNTF_sigevent, SI_MESGQ);         /*  ���� sigevent               */
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
** ��������: __mqueueSeekPrio
** ��������: ȷ����Ϣ������������ȼ���Ϣ�����ȼ�.
** �䡡��  : pmq           ��Ϣ���п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static uint_t  __mqueueSeekPrio (__PX_MSG  *pmq)
{
    REGISTER uint32_t    u32Map = pmq->PMSG_u32Map;

    archAssert(u32Map, __func__, __FILE__, __LINE__);                   /*  ���� u32Map ���Բ�Ϊ 0      */

    return  ((uint_t)archFindLsb(u32Map) - 1);
}
/*********************************************************************************************************
** ��������: __mqueueSend
** ��������: ����һ����Ϣ.
** �䡡��  : pmq           ��Ϣ���п��ƿ�
**           msg           ��Ϣָ��
**           msgsize       ��Ϣ��С
**           uiRealPrio    ת��������ȼ�
**           ulTimeout     ��ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __mqueueSend (__PX_MSG  *pmq, const char  *msg, size_t  msglen, 
                          unsigned uiRealPrio, ULONG  ulTimeout)
{
    ULONG               ulError;
    __PX_MSG_NODE      *pmqn;
    PLW_LIST_RING       pringMsg;
    caddr_t             pcBuffer;                                       /*  ��Ϣ����                    */
    ULONG               ulOrgKernelTime;
    
    /*
     *  ��ȡ���ͻ��岢��������
     */
    do {
        __KERNEL_TIME_GET(ulOrgKernelTime, ULONG);                      /*  ��¼ϵͳʱ��                */
        
        ulError = __PX_MQ_WWAIT(pmq, ulTimeout);                        /*  �ȴ�дͬ��                  */
        if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
            errno = ETIMEDOUT;
            return  (PX_ERROR);
        
        } else if (ulError == EINTR) {
            errno = EINTR;
            return  (PX_ERROR);
        
        } else if (ulError) {
            return  (PX_ERROR);
        }
        
        if (__PX_MQ_LOCK(pmq)) {                                        /*  ������Ϣ����                */
            errno = ENOENT;
            return  (PX_ERROR);
        }
        
        if (pmq->PMSG_pmsgmem.PMSGM_pringFreeList) {                    /*  ���Է���                    */
            break;
        }
        __PX_MQ_UNLOCK(pmq);                                            /*  ������Ϣ����                */
    
        ulTimeout = _sigTimeoutRecalc(ulOrgKernelTime, ulTimeout);      /*  ���¼���ȴ�ʱ��            */
    } while (1);
    
    pringMsg = pmq->PMSG_pmsgmem.PMSGM_pringFreeList;
    pmqn     = (__PX_MSG_NODE *)pringMsg;
    pcBuffer = ((char *)pmqn + sizeof(__PX_MSG_NODE));                  /*  ��λ��Ϣ����                */
    
    _List_Ring_Del(&pmqn->PMSGN_ringManage,
                   &pmq->PMSG_pmsgmem.PMSGM_pringFreeList);             /*  �ӿ��ж�����ɾ��            */
                   
    lib_memcpy(pcBuffer, msg, msglen);                                  /*  ������Ϣ                    */
    pmqn->PMSGN_stMsgLen = msglen;                                      /*  ��¼��Ϣ����                */
    
    /*
     *  ������Ϣ��λͼ
     */
    _List_Ring_Add_Last(&pmqn->PMSGN_ringManage,
                        &pmq->PMSG_pringMsg[uiRealPrio]);               /*  ������Ϣɢ�б�              */
    pmq->PMSG_u32Map |= (1 << uiRealPrio);                              /*  λͼ����                  */
    
    /*
     *  ������Ϣ����״̬
     */
    pmq->PMSG_mqattr.mq_curmsgs++;                                      /*  �������Ϣ����++            */
    __PX_MQ_RPOST(pmq);                                                 /*  ��Ϣ���пɶ�                */
    
#if LW_CFG_SIGNAL_EN > 0
    if (pmq->PMSG_mqattr.mq_curmsgs == 1) {                             /*  �ź��첽֪ͨ�ɶ�            */
        __mqueueSignalNotify(pmq);
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    
    if (pmq->PMSG_pmsgmem.PMSGM_pringFreeList) {
        __PX_MQ_WPOST(pmq);                                             /*  ��д                        */
    }
    __PX_MQ_UNLOCK(pmq);                                                /*  ������Ϣ����                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __mqueueRecv
** ��������: ����һ����Ϣ.
** �䡡��  : pmq           ��Ϣ���п��ƿ�
**           msg           ��Ϣָ��
**           msglen        �����С
**           pmsgprio      ���յ���Ϣ���ȼ� (����)
**           ulTimeout     ��ʱʱ��
** �䡡��  : ������Ϣ�Ĵ�С
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __mqueueRecv (__PX_MSG  *pmq, char  *msg, size_t  msglen, 
                              unsigned *pmsgprio, ULONG  ulTimeout)
{
    __PX_MSG_NODE      *pmqn;
    PLW_LIST_RING       pringMsg;
    caddr_t             pcBuffer;                                       /*  ��Ϣ����                    */
    
    uint_t              uiRealPrio;
    ULONG               ulError;
    ULONG               ulOrgKernelTime;

    /*
     *  ��ʼ������Ϣ
     */
    do {
        __KERNEL_TIME_GET(ulOrgKernelTime, ULONG);                      /*  ��¼ϵͳʱ��                */
        
        ulError = __PX_MQ_RWAIT(pmq, ulTimeout);                        /*  �ȴ���ͬ��                  */
        if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
            errno = ETIMEDOUT;
            return  (PX_ERROR);
        
        } else if (ulError == EINTR) {
            errno = EINTR;
            return  (PX_ERROR);
        
        } else if (ulError) {
            return  (PX_ERROR);
        }
        
        if (__PX_MQ_LOCK(pmq)) {                                        /*  ������Ϣ����                */
            errno = ENOENT;
            return  (PX_ERROR);
        }
    
        if (pmq->PMSG_mqattr.mq_curmsgs) {                              /*  �����ݿɶ�                  */
            break;
        }
        __PX_MQ_UNLOCK(pmq);                                            /*  ������Ϣ����                */
    
        ulTimeout = _sigTimeoutRecalc(ulOrgKernelTime, ulTimeout);      /*  ���¼���ȴ�ʱ��            */
    } while (1);
    
    uiRealPrio = __mqueueSeekPrio(pmq);                                 /*  ȷ�����ȼ�                  */
    pringMsg   = pmq->PMSG_pringMsg[uiRealPrio];
    pmqn       = (__PX_MSG_NODE *)pringMsg;
    pcBuffer   = ((char *)pmqn + sizeof(__PX_MSG_NODE));                /*  ��λ��Ϣ����                */
    
    if (pmqn->PMSGN_stMsgLen > msglen) {                                /*  ��Ϣ̫���޷�����            */
        __PX_MQ_RPOST(pmq);
        errno = EMSGSIZE;
        __PX_MQ_UNLOCK(pmq);                                            /*  ������Ϣ����                */
        return  (PX_ERROR);
    }
    
    /*
     *  ������Ϣ��λͼ
     */
    _List_Ring_Del(&pmqn->PMSGN_ringManage,
                   &pmq->PMSG_pringMsg[uiRealPrio]);                    /*  ����Ϣɢ�б���ɾ��          */
    if (pmq->PMSG_pringMsg[uiRealPrio] == LW_NULL) {                    /*  �����ȼ��Ѿ�û����Ϣ        */
        pmq->PMSG_u32Map &= ~(1 << uiRealPrio);                         /*  λͼ����                  */
    }
    
    if (pmsgprio) {
        *pmsgprio = __PX_MQ_PRIO_CONVERT(uiRealPrio);                   /*  ��д��Ϣ���ȼ�              */
    }
    
    /*
     *  ������Ϣ
     */
    msglen = pmqn->PMSGN_stMsgLen;                                      /*  ��¼��Ϣ����                */
    lib_memcpy(msg, pcBuffer, msglen);                                  /*  ������Ϣ                    */
    
    _List_Ring_Add_Last(&pmqn->PMSGN_ringManage,
                        &pmq->PMSG_pmsgmem.PMSGM_pringFreeList);        /*  ���¼�����ж���            */
                        
    /*
     *  ������Ϣ����״̬
     */
    pmq->PMSG_mqattr.mq_curmsgs--;                                      /*  �������Ϣ����++            */
    __PX_MQ_WPOST(pmq);                                                 /*  ���пռ���Է�����Ϣ        */
    
    if (pmq->PMSG_mqattr.mq_curmsgs) {
        __PX_MQ_RPOST(pmq);                                             /*  �����Զ�ȡ��Ϣ              */
    }
    __PX_MQ_UNLOCK(pmq);                                                /*  ������Ϣ����                */
    
    return  ((ssize_t)msglen);
}
/*********************************************************************************************************
** ��������: __mqueueInitBuffer
** ��������: ��ʼ����Ϣ���е� buffer.
** �䡡��  : pmq           ��Ϣ���п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __mqueueInitBuffer (__PX_MSG  *pmq)
{
    ULONG                i;
    __PX_MSG_NODE       *pmsgnode = (__PX_MSG_NODE *)pmq->PMSG_pmsgmem.PMSGM_pcMem;
                                                                        /*  �����Ϣ�����׵�ַ          */
    
    for (i = 0; i < pmq->PMSG_mqattr.mq_maxmsg; i++) {                  /*  ��������Ϣ�ڵ�����������  */
        _List_Ring_Add_Last(&pmsgnode->PMSGN_ringManage,
                            &pmq->PMSG_pmsgmem.PMSGM_pringFreeList);
        pmsgnode = (__PX_MSG_NODE *)((char *)pmsgnode + pmq->PMSG_pmsgmem.PMSGM_stBytesPerMsg);
    }
}
/*********************************************************************************************************
** ��������: __mqueueCreate
** ��������: ����һ����Ϣ����.
** �䡡��  : name          ��Ϣ������
**           mode          ����mode
**           pmqattr       ��Ϣ��������
** �䡡��  : ��Ϣ���п��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static __PX_MSG  *__mqueueCreate (const char  *name, mode_t mode, struct mq_attr  *pmqattr)
{
    INT        iErrLevel = 0;
    size_t     stMemBufferSize;                                         /*  ��Ҫ�Ļ����С              */
    size_t     stLen = lib_strlen(name);
    __PX_MSG  *pmq;
    
    /*
     *  �������ƿ��ڴ�
     */
    pmq = (__PX_MSG *)__SHEAP_ALLOC(sizeof(__PX_MSG) + stLen + 1);
    if (pmq == LW_NULL) {
        errno = ENOMEM;
        return  (LW_NULL);
    }
    lib_bzero(pmq, sizeof(__PX_MSG));                                   /*  ��������ȫ������            */
    
    pmq->PMSG_pxnode.PXNODE_pcName = (char *)pmq + sizeof(__PX_MSG);    /*  ȷ�����ֻ����λ��          */
    pmq->PMSG_pxnode.PXNODE_pvData = (void *)pmq;                       /*  ��¼���ƿ��ַ              */
    pmq->PMSG_pxnode.PXNODE_iType  = __PX_NAMED_OBJECT_MQ;              /*  ��Ϣ����                    */
    lib_strcpy(pmq->PMSG_pxnode.PXNODE_pcName, name);                   /*  ������Ϣ������              */
    
    pmq->PMSG_mode = mode;                                              /*  ���� mode                   */
    
    /*
     *  ����ÿһ����Ϣռ�õ��ֽ���
     */
    stMemBufferSize = ((size_t)pmqattr->mq_msgsize 
                    + sizeof(__PX_MSG_NODE));                           /*  һ����Ϣ�ڵ�Ĵ�С          */
    stMemBufferSize = ROUND_UP(stMemBufferSize, sizeof(LW_STACK));      /*  �����С����                */
    
    pmq->PMSG_pmsgmem.PMSGM_stBytesPerMsg = stMemBufferSize;            /*  ÿһ����Ϣռ�õ��ֽ���      */
    
    /*
     *  ������Ϣ�����ڴ�
     */
    stMemBufferSize *= (size_t)pmqattr->mq_maxmsg;                      /*  ������Ϣ�ڵ��ܹ���Ҫ�Ĵ�С  */
    pmq->PMSG_pmsgmem.PMSGM_pcMem = (caddr_t)__SHEAP_ALLOC(stMemBufferSize);
    if (pmq->PMSG_pmsgmem.PMSGM_pcMem == LW_NULL) {
        iErrLevel = 1;
        errno     = ENOMEM;
        goto    __error_handle;
    }
    pmq->PMSG_mqattr = *pmqattr;                                        /*  �������Կ�                  */
    pmq->PMSG_mqattr.mq_curmsgs = 0;                                    /*  Ŀǰû���κ���Ч��Ϣ        */
    
    /*
     *  ��ʼ���ź���
     */
    pmq->PMSG_ulReadSync = API_SemaphoreBCreate("pxmq_rdsync", LW_FALSE, 
                                                __PX_MQUEUE_OPTION, 
                                                LW_NULL);               /*  ��ʼ��Ϊ���ɶ�              */
    if (pmq->PMSG_ulReadSync == LW_OBJECT_HANDLE_INVALID) {
        iErrLevel = 2;
        errno     = ENOSPC;
        goto    __error_handle;
    }
    
    pmq->PMSG_ulWriteSync = API_SemaphoreBCreate("pxmq_wrsync", LW_TRUE,
                                                 __PX_MQUEUE_OPTION, 
                                                 LW_NULL);              /*  ��ʼ��Ϊ��д                */
    if (pmq->PMSG_ulWriteSync == LW_OBJECT_HANDLE_INVALID) {
        iErrLevel = 3;
        errno     = ENOSPC;
        goto    __error_handle;
    }
    
    pmq->PMSG_ulMutex = API_SemaphoreMCreate("pxmq_mutex", LW_PRIO_DEF_CEILING, 
                                             LW_OPTION_INHERIT_PRIORITY | 
                                             LW_OPTION_DELETE_SAFE |
                                             LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pmq->PMSG_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        iErrLevel = 4;
        errno     = ENOSPC;
        goto    __error_handle;
    }
    
    /*
     *  �����е���Ϣ�ڵ�ȫ��������ж���
     */
    __mqueueInitBuffer(pmq);
    
    return  (pmq);
    
__error_handle:
    if (iErrLevel > 3) {
        API_SemaphoreBDelete(&pmq->PMSG_ulWriteSync);
    }
    if (iErrLevel > 2) {
        API_SemaphoreBDelete(&pmq->PMSG_ulReadSync);
    }
    if (iErrLevel > 1) {
        __SHEAP_FREE(pmq->PMSG_pmsgmem.PMSGM_pcMem);
    }
    if (iErrLevel > 0) {
        __SHEAP_FREE(pmq);
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __mqueueDelete
** ��������: ɾ��һ����Ϣ����.
** �䡡��  : pmq           ��Ϣ���п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __mqueueDelete (__PX_MSG  *pmq)
{
    API_SemaphoreMDelete(&pmq->PMSG_ulMutex);
    API_SemaphoreBDelete(&pmq->PMSG_ulWriteSync);
    API_SemaphoreBDelete(&pmq->PMSG_ulReadSync);
    __SHEAP_FREE(pmq->PMSG_pmsgmem.PMSGM_pcMem);
    __SHEAP_FREE(pmq);
}
/*********************************************************************************************************
** ��������: mq_open
** ��������: ��һ�������� posix ��Ϣ����.
** �䡡��  : name          �ź���������
**           flag          ��ѡ�� (O_NONBLOCK, O_CREAT, O_EXCL, O_RDONLY, O_WRONLY, O_RDWR...)
**           ...
** �䡡��  : ��Ϣ���о��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
mqd_t  mq_open (const char  *name, int  flag, ...)
{
    __PX_MSG           *pmq;
    __PX_NAME_NODE     *pxnode;
    __PX_MSG_FILE      *pmqfile;
    
    va_list             valist;
    size_t              stNameLen;

    if (name == LW_NULL) {
        errno = EINVAL;
        return  (MQ_FAILED);
    }
    
    stNameLen = lib_strnlen(name, (NAME_MAX + 1));
    if (stNameLen > NAME_MAX) {
        errno = ENAMETOOLONG;
        return  (MQ_FAILED);
    }
    
    __PX_LOCK();                                                        /*  ��ס posix                  */
    pxnode = __pxnameSeach(name, -1);
    if (pxnode) {                                                       /*  �ҵ�                        */
        if ((flag & O_EXCL) && (flag & O_CREAT)) {
            __PX_UNLOCK();                                              /*  ���� posix                  */
            errno = EEXIST;
            return  (MQ_FAILED);                                        /*  ����ͬ���½�                */
        
        } else {
            if (pxnode->PXNODE_iType != __PX_NAMED_OBJECT_MQ) {         /*  �������                    */
                __PX_UNLOCK();                                          /*  ���� posix                  */
                errno = EEXIST;
                return  (MQ_FAILED);
            }
            
            pmqfile = (__PX_MSG_FILE *)__SHEAP_ALLOC(sizeof(__PX_MSG_FILE));
            if (pmqfile == LW_NULL) {
                __PX_UNLOCK();                                          /*  ���� posix                  */
                errno = ENOMEM;
                return  (MQ_FAILED);
            }
            pmqfile->PMSGF_pmg   = (__PX_MSG *)pxnode->PXNODE_pvData;
            pmqfile->PMSGF_iFlag = flag;
            
            API_AtomicInc(&pxnode->PXNODE_atomic);
            __PX_UNLOCK();                                              /*  ���� posix                  */
            
            __resAddRawHook(&pmqfile->PMSGF_resraw, (VOIDFUNCPTR)mq_close, 
                            pmqfile, 0, 0, 0, 0, 0);                    /*  ������Դ������              */
                    
            return  ((mqd_t)pmqfile);                                   /*  ���ؾ����ַ                */
        }
    
    } else {
        
        if (flag & O_CREAT) {
            mode_t          mode;
            struct mq_attr *pmqattr;
            
            va_start(valist, flag);
            mode    = va_arg(valist, mode_t);
            pmqattr = va_arg(valist, struct mq_attr *);
            va_end(valist);
        
            if (pmqattr == LW_NULL) {
                pmqattr =  &mq_attr_default;                            /*  ʹ��Ĭ������                */
            } else {
                if ((pmqattr->mq_msgsize <= 0) ||
                    (pmqattr->mq_maxmsg  <= 0)) {
                    __PX_UNLOCK();                                      /*  ���� posix                  */
                    errno = EINVAL;
                    return  (MQ_FAILED);
                }
            }
            
            /*
             *  ��ʼ������Ϣ���м����.
             */
            pmqfile = (__PX_MSG_FILE *)__SHEAP_ALLOC(sizeof(__PX_MSG_FILE));
            if (pmqfile == LW_NULL) {
                __PX_UNLOCK();                                          /*  ���� posix                  */
                errno = ENOMEM;
                return  (MQ_FAILED);
            }
            
            pmq = __mqueueCreate(name, mode, pmqattr);                  /*  ������Ϣ����                */
            if (pmq == LW_NULL) {
                __SHEAP_FREE(pmqfile);
                __PX_UNLOCK();                                          /*  ���� posix                  */
                return  (MQ_FAILED);
            }
            __pxnameAdd(&pmq->PMSG_pxnode);                             /*  �������ֽڵ��              */
            
            pmqfile->PMSGF_pmg   = pmq;
            pmqfile->PMSGF_iFlag = flag;
            
            API_AtomicInc(&pmq->PMSG_pxnode.PXNODE_atomic);
            __PX_UNLOCK();                                              /*  ���� posix                  */
            
            __resAddRawHook(&pmqfile->PMSGF_resraw, (VOIDFUNCPTR)mq_close, 
                            pmqfile, 0, 0, 0, 0, 0);                    /*  ������Դ������              */
            
            _DebugFormat(__LOGMESSAGE_LEVEL, "posix msgqueue \"%s\" has been create.\r\n", name);
            
            return  ((mqd_t)pmqfile);                                   /*  ���ؾ����ַ                */
        
        } else {
            __PX_UNLOCK();                                              /*  ���� posix                  */
            errno = ENOENT;
            return  (MQ_FAILED);
        }
    }
}
/*********************************************************************************************************
** ��������: mq_close
** ��������: �ر�һ�� posix ��Ϣ����.
** �䡡��  : mqd       ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mq_close (mqd_t  mqd)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;

    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    __PX_LOCK();                                                        /*  ��ס posix                  */
    if (API_AtomicGet(&pmq->PMSG_pxnode.PXNODE_atomic) > 0) {
        API_AtomicDec(&pmq->PMSG_pxnode.PXNODE_atomic);                 /*  ��������ʹ�ü���            */
    }
    __PX_UNLOCK();                                                      /*  ���� posix                  */

    __resDelRawHook(&pmqfile->PMSGF_resraw);

    __SHEAP_FREE(pmqfile);                                              /*  �ͷž��                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mq_unlink
** ��������: ɾ��һ�� posix ��Ϣ����.
** �䡡��  : name      ��Ϣ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mq_unlink (const char  *name)
{
    __PX_MSG           *pmq;
    __PX_NAME_NODE     *pxnode;

    if (name == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    __PX_LOCK();                                                        /*  ��ס posix                  */
    pxnode = __pxnameSeach(name, -1);
    if (pxnode) {
        if (pxnode->PXNODE_iType != __PX_NAMED_OBJECT_MQ) {             /*  �������                    */
            __PX_UNLOCK();                                              /*  ���� posix                  */
            errno = ENOENT;
            return  (PX_ERROR);
        }
        if (API_AtomicGet(&pxnode->PXNODE_atomic) > 0) {
            __PX_UNLOCK();                                              /*  ���� posix                  */
            errno = EBUSY;
            return  (PX_ERROR);
        }
        pmq = (__PX_MSG *)pxnode->PXNODE_pvData;
        
        __pxnameDel(name);                                              /*  �����ֱ���ɾ��              */
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "posix msgqueue \"%s\" has been delete.\r\n", name);
        
        __mqueueDelete(pmq);                                            /*  ɾ����Ϣ���п��ƿ�          */
    
        __PX_UNLOCK();                                                  /*  ���� posix                  */
        return  (ERROR_NONE);
        
    } else {
        __PX_UNLOCK();                                                  /*  ���� posix                  */
        errno = ENOENT;
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: mq_getattr
** ��������: ��� posix ��Ϣ��������.
** �䡡��  : mqd       ���
**           pmqattr   ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mq_getattr (mqd_t  mqd, struct mq_attr *pmqattr)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    
    if (pmqattr == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    __PX_MQ_LOCK(pmq);                                                  /*  ������Ϣ����                */
    *pmqattr = pmq->PMSG_mqattr;
    __PX_MQ_UNLOCK(pmq);                                                /*  ������Ϣ����                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mq_setattr
** ��������: ���� posix ��Ϣ��������.
** �䡡��  : mqd           ���
**           pmqattrNew    ����
**           pmqattrOld    ���������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mq_setattr (mqd_t  mqd, const struct mq_attr *pmqattrNew, struct mq_attr *pmqattrOld)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    
    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    __PX_MQ_LOCK(pmq);                                                  /*  ������Ϣ����                */
    if (pmqattrOld) {
        *pmqattrOld = pmq->PMSG_mqattr;
    }
    if (pmqattrNew) {
        if (pmqattrNew->mq_flags & O_NONBLOCK) {
            pmq->PMSG_mqattr.mq_flags |= O_NONBLOCK;
        } else {
            pmq->PMSG_mqattr.mq_flags &= ~O_NONBLOCK;
        }
    }
    __PX_MQ_UNLOCK(pmq);                                                /*  ������Ϣ����                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mq_send
** ��������: ��һ����Ϣ���͵� posix ��Ϣ����.
** �䡡��  : mqd           ���
**           msg           ��Ϣָ��
**           msglen        ��Ϣ����
**           msgprio       ��Ϣ���ȼ�    (< MQ_PRIO_MAX)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mq_send (mqd_t  mqd, const char  *msg, size_t  msglen, unsigned msgprio)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    uint_t              uiRealPrio;
    
    ULONG               ulTimeout;
    INT                 iRet;
    
    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    if ((msg == LW_NULL) || (msglen == 0)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (msgprio >= MQ_PRIO_MAX) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    uiRealPrio = __PX_MQ_PRIO_CONVERT(msgprio);                         /*  ������ʵ���ȼ�              */
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    if (msglen > pmq->PMSG_mqattr.mq_msgsize) {                         /*  ��Ϣ̫��                    */
        errno = EMSGSIZE;
        return  (PX_ERROR);
    }
    if ((pmqfile->PMSGF_iFlag & O_ACCMODE) == O_RDONLY) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    if ((pmqfile->PMSGF_iFlag & O_NONBLOCK) ||
        (pmq->PMSG_mqattr.mq_flags & O_NONBLOCK)) {                     /*  ������                      */
        ulTimeout = LW_OPTION_NOT_WAIT;
        
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    iRet = __mqueueSend(pmq, msg, msglen, uiRealPrio, ulTimeout);       /*  ������Ϣ                    */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: mq_timedsend
** ��������: ��һ����Ϣ���͵� posix ��Ϣ����.
** �䡡��  : mqd           ���
**           msg           ��Ϣָ��
**           msglen        ��Ϣ����
**           msgprio       ��Ϣ���ȼ�    (< MQ_PRIO_MAX)
**           abs_timeout   ��ʱʱ�� (ע��: �����Ǿ���ʱ��, ��һ��ȷ������ʷʱ������: 2009.12.31 15:36:04)
                           ���߾���������Ǻ�ˬ, Ӧ���ټ�һ���������Եȴ����ʱ��.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mq_timedsend (mqd_t  mqd, const char  *msg, size_t  msglen, 
                   unsigned msgprio, const struct timespec *abs_timeout)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    uint_t              uiRealPrio;
    
    ULONG               ulTimeout;
    INT                 iRet;
    struct timespec     tvNow;
    struct timespec     tvWait = {0, 0};
    
    if ((abs_timeout == LW_NULL)    || 
        (abs_timeout->tv_nsec <  0) ||
        (abs_timeout->tv_nsec >= __TIMEVAL_NSEC_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    if ((msg == LW_NULL) || (msglen == 0)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (msgprio >= MQ_PRIO_MAX) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    uiRealPrio = __PX_MQ_PRIO_CONVERT(msgprio);                         /*  ������ʵ���ȼ�              */
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    if (msglen > pmq->PMSG_mqattr.mq_msgsize) {                         /*  ��Ϣ̫��                    */
        errno = EMSGSIZE;
        return  (PX_ERROR);
    }
    if ((pmqfile->PMSGF_iFlag & O_ACCMODE) == O_RDONLY) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    lib_clock_gettime(CLOCK_REALTIME, &tvNow);                          /*  ��õ�ǰϵͳʱ��            */
    if (__timespecLeftTime(&tvNow, abs_timeout)) {
        tvWait = *abs_timeout;
        __timespecSub(&tvWait, &tvNow);                                 /*  �����뵱ǰ�ȴ���ʱ����    */
    }
    /*
     *  ע��: �� tvWait ����ulong tick��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.
     */
    ulTimeout = __timespecToTick(&tvWait);                              /*  �任Ϊ tick                 */
    
    if ((pmqfile->PMSGF_iFlag & O_NONBLOCK) ||
        (pmq->PMSG_mqattr.mq_flags & O_NONBLOCK)) {                     /*  ������                      */
        ulTimeout = LW_OPTION_NOT_WAIT;
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    iRet = __mqueueSend(pmq, msg, msglen, uiRealPrio, ulTimeout);       /*  ������Ϣ                    */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: mq_reltimedsend_np
** ��������: ��һ����Ϣ���͵� posix ��Ϣ����.
** �䡡��  : mqd           ���
**           msg           ��Ϣָ��
**           msglen        ��Ϣ����
**           msgprio       ��Ϣ���ȼ�    (< MQ_PRIO_MAX)
**           rel_timeout   ��Գ�ʱʱ��.
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  mq_reltimedsend_np (mqd_t  mqd, const char  *msg, size_t  msglen, 
                         unsigned msgprio, const struct timespec *rel_timeout)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    uint_t              uiRealPrio;
    
    ULONG               ulTimeout;
    INT                 iRet;
    
    if ((rel_timeout == LW_NULL)    || 
        (rel_timeout->tv_nsec <  0) ||
        (rel_timeout->tv_nsec >= __TIMEVAL_NSEC_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    if ((msg == LW_NULL) || (msglen == 0)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (msgprio >= MQ_PRIO_MAX) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    uiRealPrio = __PX_MQ_PRIO_CONVERT(msgprio);                         /*  ������ʵ���ȼ�              */
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    if (msglen > pmq->PMSG_mqattr.mq_msgsize) {                         /*  ��Ϣ̫��                    */
        errno = EMSGSIZE;
        return  (PX_ERROR);
    }
    if ((pmqfile->PMSGF_iFlag & O_ACCMODE) == O_RDONLY) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    /*
     *  ע��: �� rel_timeout ����ulong tick��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.
     */
    ulTimeout = __timespecToTick(rel_timeout);                          /*  �任Ϊ tick                 */
    
    if ((pmqfile->PMSGF_iFlag & O_NONBLOCK) ||
        (pmq->PMSG_mqattr.mq_flags & O_NONBLOCK)) {                     /*  ������                      */
        ulTimeout = LW_OPTION_NOT_WAIT;
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    iRet = __mqueueSend(pmq, msg, msglen, uiRealPrio, ulTimeout);       /*  ������Ϣ                    */
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** ��������: mq_receive
** ��������: �� posix ��Ϣ���н���һ����Ϣ.
** �䡡��  : mqd           ���
**           msg           ��Ϣ������
**           msglen        ����������
**           pmsgprio      ������Ϣ�����ȼ� (����)
** �䡡��  : ������Ϣ�ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ssize_t  mq_receive (mqd_t  mqd, char  *msg, size_t  msglen, unsigned *pmsgprio)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    
    ULONG               ulTimeout;
    ssize_t             sstRet;
    
    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    if ((msg == LW_NULL) || (msglen == 0)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    if (msglen < pmq->PMSG_mqattr.mq_msgsize) {                         /*  ����̫С                    */
        errno = EMSGSIZE;
        return  (PX_ERROR);
    }
    if ((pmqfile->PMSGF_iFlag & O_ACCMODE) & O_WRONLY) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    if ((pmqfile->PMSGF_iFlag & O_NONBLOCK) ||
        (pmq->PMSG_mqattr.mq_flags & O_NONBLOCK)) {                     /*  ������                      */
        ulTimeout = LW_OPTION_NOT_WAIT;
        
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    sstRet = __mqueueRecv(pmq, msg, msglen, pmsgprio, ulTimeout);       /*  ������Ϣ                    */
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: mq_timedreceive
** ��������: �� posix ��Ϣ���н���һ����Ϣ.
** �䡡��  : mqd           ���
**           msg           ��Ϣ������
**           msglen        ����������
**           pmsgprio      ������Ϣ�����ȼ� (����)
**           abs_timeout   ��ʱʱ��
** �䡡��  : ������Ϣ�ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ssize_t  mq_timedreceive (mqd_t  mqd, char  *msg, size_t  msglen, 
                          unsigned *pmsgprio, const struct timespec *abs_timeout)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    
    ULONG               ulTimeout;
    ssize_t             sstRet;
    struct timespec     tvNow;
    struct timespec     tvWait = {0, 0};
    
    if ((abs_timeout == LW_NULL)    || 
        (abs_timeout->tv_nsec <  0) ||
        (abs_timeout->tv_nsec >= __TIMEVAL_NSEC_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    if ((msg == LW_NULL) || (msglen == 0)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    if (msglen < pmq->PMSG_mqattr.mq_msgsize) {                         /*  ����̫С                    */
        errno = EMSGSIZE;
        return  (PX_ERROR);
    }
    if ((pmqfile->PMSGF_iFlag & O_ACCMODE) & O_WRONLY) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    lib_clock_gettime(CLOCK_REALTIME, &tvNow);                          /*  ��õ�ǰϵͳʱ��            */
    if (__timespecLeftTime(&tvNow, abs_timeout)) {
        tvWait = *abs_timeout;
        __timespecSub(&tvWait, &tvNow);                                 /*  �����뵱ǰ�ȴ���ʱ����    */
    }
    /*
     *  ע��: �� tvWait ����ulong tick��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.
     */
    ulTimeout = __timespecToTick(&tvWait);                              /*  �任Ϊ tick                 */
    
    if ((pmqfile->PMSGF_iFlag & O_NONBLOCK) ||
        (pmq->PMSG_mqattr.mq_flags & O_NONBLOCK)) {                     /*  ������                      */
        ulTimeout = LW_OPTION_NOT_WAIT;
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    sstRet = __mqueueRecv(pmq, msg, msglen, pmsgprio, ulTimeout);       /*  ������Ϣ                    */
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: mq_reltimedreceive_np
** ��������: �� posix ��Ϣ���н���һ����Ϣ.
** �䡡��  : mqd           ���
**           msg           ��Ϣ������
**           msglen        ����������
**           pmsgprio      ������Ϣ�����ȼ� (����)
**           rel_timeout   ��Գ�ʱʱ��
** �䡡��  : ������Ϣ�ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
ssize_t  mq_reltimedreceive_np (mqd_t  mqd, char  *msg, size_t  msglen, 
                                unsigned *pmsgprio, const struct timespec *rel_timeout)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    
    ULONG               ulTimeout;
    ssize_t             sstRet;
    
    if ((rel_timeout == LW_NULL)    ||
        (rel_timeout->tv_nsec <  0) ||
        (rel_timeout->tv_nsec >= __TIMEVAL_NSEC_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    if ((msg == LW_NULL) || (msglen == 0)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    if (msglen < pmq->PMSG_mqattr.mq_msgsize) {                         /*  ����̫С                    */
        errno = EMSGSIZE;
        return  (PX_ERROR);
    }
    if ((pmqfile->PMSGF_iFlag & O_ACCMODE) & O_WRONLY) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    /*
     *  ע��: �� rel_timeout ����ulong tick��Χʱ, ����Ȼ�������, ���³�ʱʱ�䲻׼ȷ.
     */
    ulTimeout = __timespecToTick(rel_timeout);                          /*  �任Ϊ tick                 */
    
    if ((pmqfile->PMSGF_iFlag & O_NONBLOCK) ||
        (pmq->PMSG_mqattr.mq_flags & O_NONBLOCK)) {                     /*  ������                      */
        ulTimeout = LW_OPTION_NOT_WAIT;
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    sstRet = __mqueueRecv(pmq, msg, msglen, pmsgprio, ulTimeout);       /*  ������Ϣ                    */
    
    return  (sstRet);
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** ��������: mq_notify
** ��������: ����Ϣ���пɶ�ʱ, ע��һ��֪ͨ�ź�.
** �䡡��  : mqd           ���
**           pnotify       �ź��¼�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mq_notify (mqd_t  mqd, const struct sigevent  *pnotify)
{
#if LW_CFG_SIGNAL_EN > 0
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;

    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    if (pnotify) {                                                      /*  ��װ                        */
        if (!__issig(pnotify->sigev_signo)) {                           /*  �ź�ֵ����                  */
            errno = EINVAL;
            return  (PX_ERROR);
        }
        __PX_MQ_LOCK(pmq);                                              /*  ������Ϣ����                */
        pmq->PMSG_pmsgntf.PMSGNTF_sigevent   = *pnotify;
        pmq->PMSG_pmsgntf.PMSGNTF_ulThreadId = API_ThreadIdSelf();
        __PX_MQ_UNLOCK(pmq);                                            /*  ������Ϣ����                */
        
    } else {                                                            /*  ж��                        */
        __PX_MQ_LOCK(pmq);                                              /*  ������Ϣ����                */
        pmq->PMSG_pmsgntf.PMSGNTF_ulThreadId = LW_OBJECT_HANDLE_INVALID;
        __PX_MQ_UNLOCK(pmq);                                            /*  ������Ϣ����                */
    }

    return  (ERROR_NONE);
    
#else
    errno = ENOSYS;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
}
/*********************************************************************************************************
** ��������: mq_getinfo
** ��������: �����Ϣ������Ϣ
** �䡡��  : mqd           ���
**           info          ��д����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  mq_getinfo (mqd_t  mqd, mq_info_t  *info)
{
    __PX_MSG           *pmq;
    __PX_MSG_FILE      *pmqfile;
    
    if (info == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if ((mqd == MQ_FAILED) || (mqd == 0)) {
        errno = EBADF;
        return  (PX_ERROR);
    }
    
    pmqfile = (__PX_MSG_FILE *)mqd;
    pmq = pmqfile->PMSGF_pmg;
    
    __PX_MQ_LOCK(pmq);                                                  /*  ������Ϣ����                */
    info->attr    = pmq->PMSG_mqattr;
    info->mode    = pmq->PMSG_mode;
    info->priomap = pmq->PMSG_u32Map;
    __PX_MQ_UNLOCK(pmq);                                                /*  ������Ϣ����                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mq_show
** ��������: ��ʾ��Ϣ������Ϣ
** �䡡��  : mqd           ���
**           level         ��ʾ��Ϣ�ȼ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mq_show (mqd_t  mqd, int  level)
{
    mq_info_t  info;
    
    if (mq_getinfo(mqd, &info)) {
        fprintf(stderr, "can not get message queue information.\n");
        return  (PX_ERROR);
    }
    
    printf("message queue show >>\n\n");
    printf("mq flags   %lx\n", info.attr.mq_flags);
    printf("mq maxmsg  %lu\n", info.attr.mq_maxmsg);
    printf("mq msgsize %lu\n", info.attr.mq_msgsize);
    printf("mq curmsg  %lu\n", info.attr.mq_curmsgs);
    printf("mq mode    0%o\n", info.mode);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
