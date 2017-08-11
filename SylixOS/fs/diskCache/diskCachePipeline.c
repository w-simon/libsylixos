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
** ��   ��   ��: diskCachePipeline.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 07 �� 25 ��
**
** ��        ��: ���̸��ٻ��岢��д����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "diskCacheLib.h"
#include "diskCache.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
/*********************************************************************************************************
  ���̲���
*********************************************************************************************************/
#define __PHYDISK_READ          pblkd->BLKD_pfuncBlkRd
#define __PHYDISK_WRITE         pblkd->BLKD_pfuncBlkWrt
#define __PHYDISK_STATUS        pblkd->BLKD_pfuncBlkStatusChk
#define __PHYDISK_IOCTL         pblkd->BLKD_pfuncBlkIoctl
/*********************************************************************************************************
** ��������: __diskCacheWpThread
** ��������: д�����߳�
** �䡡��  : pvArg                 �̲߳���
** �䡡��  : LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �������߳��˳�֮ǰ��Ҫ��ǰ���� detach �����ļ�ϵͳж������, ��Ϊж����������� t_except ����
             �������߳�ɾ���������ܵò���ִ��.
*********************************************************************************************************/
static PVOID  __diskCacheWpThread (PVOID  pvArg)
{
    REGISTER INT                 i;
             PLW_DISKCACHE_CB    pdiskc = (PLW_DISKCACHE_CB)pvArg;
             PLW_DISKCACHE_WP    pwp    = &pdiskc->DISKC_wpWrite;
    REGISTER PLW_BLK_DEV         pblkd  = pdiskc->DISKC_pblkdDisk;
             LW_DISKCACHE_WPMSG  diskcwpm;
    
    for (;;) {
        API_MsgQueueReceive(pwp->DISKCWP_hMsgQueue, &diskcwpm,
                            sizeof(LW_DISKCACHE_WPMSG), LW_NULL,
                            LW_OPTION_WAIT_INFINITE);
        
        if (pwp->DISKCWP_bExit) {
            break;                                                      /*  �߳��˳�                    */
        }
        
        if (pwp->DISKCWP_bParallel == LW_FALSE) {
            API_SemaphoreMPend(pwp->DISKCWP_hDev, 
                               LW_OPTION_WAIT_INFINITE);                /*  �������                    */
        }
        
        i = 0;
        
        if (__PHYDISK_WRITE(pblkd, diskcwpm.DISKCWPM_pvBuffer,
                            diskcwpm.DISKCWPM_ulStartSector,
                            diskcwpm.DISKCWPM_ulNSector) < 0) {
            for (i = 0; i < pblkd->BLKD_iRetry; i++) {
                if (pblkd->BLKD_bRemovable) {
                    if (__PHYDISK_STATUS) {
                        if (__PHYDISK_STATUS(pblkd) != ERROR_NONE) {
                            continue;                                   /*  �豸״̬����, ���¼��      */
                        }
                    }
                }
                if (__PHYDISK_WRITE(pblkd, diskcwpm.DISKCWPM_pvBuffer,
                                    diskcwpm.DISKCWPM_ulStartSector,
                                    diskcwpm.DISKCWPM_ulNSector) >= 0) {
                    break;
                }
            }
        }
        
        if (i >= pblkd->BLKD_iRetry) {
            if (pblkd->BLKD_bRemovable) {
                __PHYDISK_IOCTL(pblkd, FIOCANCEL, 0);
            }
            _DebugFormat(__ERRORMESSAGE_LEVEL, "can not write block: blk %s sector %lu [%ld].\r\n",
                         pblkd->BLKD_pcName,
                         diskcwpm.DISKCWPM_ulStartSector, 
                         diskcwpm.DISKCWPM_ulNSector);
        }
                                                  
        if (pwp->DISKCWP_bParallel == LW_FALSE) {
            API_SemaphoreMPost(pwp->DISKCWP_hDev);                      /*  ����������                */
        }
        
        API_PartitionPut(pwp->DISKCWP_hPart, diskcwpm.DISKCWPM_pvBuffer);
        API_SemaphoreCPost(pwp->DISKCWP_hCounter);
        API_SemaphoreBPost(pwp->DISKCWP_hSync);
    }
    
    API_ThreadDetach(API_ThreadIdSelf());                               /*  ��� Join �߳�              */
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __diskCacheWpTCreate
** ��������: ����д�����߳���
** �䡡��  : pdiskc                ���̻�����ƿ�
**           pwp                   ����д���߿��ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __diskCacheWpTCreate (PLW_DISKCACHE_CB  pdiskc, PLW_DISKCACHE_WP  pwp)
{
    INT                     i, j;
    LW_CLASS_THREADATTR     threadattr;
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_THREAD_DISKCACHE_STK_SIZE,
                        LW_PRIO_T_SERVICE,
                        LW_CFG_DISKCACHE_OPTION | LW_OPTION_OBJECT_GLOBAL,
                        pdiskc);
    
    for (i = 0; i < pwp->DISKCWP_iPipeline; i++) {
        pwp->DISKCWP_hWThread[i] = API_ThreadInit("t_dcwpipe", 
                                                  __diskCacheWpThread,
                                                  &threadattr,
                                                  LW_NULL);
        if (pwp->DISKCWP_hWThread[i] == LW_OBJECT_HANDLE_INVALID) {
            break;
        }
    }
    
    if (i < pwp->DISKCWP_iPipeline) {
        for (j = 0; j < i; j++) {
            API_ThreadDelete(&pwp->DISKCWP_hWThread[j], LW_NULL);
        }
        return  (PX_ERROR);
    
    } else {
        for (i = 0; i < pwp->DISKCWP_iPipeline; i++) {
            API_ThreadStart(pwp->DISKCWP_hWThread[i]);
        }
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __diskCacheWpTDelete
** ��������: ɾ��д�����߳���
** �䡡��  : pwp                   ����д���߿��ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __diskCacheWpTDelete (PLW_DISKCACHE_WP  pwp)
{
    INT   i;

    for (i = 0; i < pwp->DISKCWP_iPipeline; i++) {
        API_ThreadJoin(pwp->DISKCWP_hWThread[i], LW_NULL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskCacheWpMemCreate
** ��������: ��������
** �䡡��  : pwp                   ����д���߿��ƿ�
**           bCacheCoherence       CACHE һ��������
**           iPipeline             д�����߳���
**           iMsgCount             ��Ϣ���г���
**           iMaxRBurstSector      ��⧷�����
**           iMaxWBurstSector      д⧷�����
**           ulBytesPerSector      ÿ������С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __diskCacheWpMemCreate (PLW_DISKCACHE_WP  pwp,
                                    BOOL              bCacheCoherence,
                                    INT               iPipeline, 
                                    INT               iMsgCount, 
                                    INT               iMaxRBurstSector,
                                    INT               iMaxWBurstSector,
                                    ULONG             ulBytesPerSector)
{
    INT     iMaxBurst;
    PVOID   pvRBurstBuffer;
    PVOID   pvWBurstBuffer;
    
    pwp->DISKCWP_bCacheCoherence = bCacheCoherence;
    
    if (iPipeline == 0) {                                               /*  ����Ҫ����֧��              */
        iMaxBurst = __MAX(iMaxRBurstSector, iMaxWBurstSector);
        
#if LW_CFG_CACHE_EN > 0
        if (bCacheCoherence) {
            pvRBurstBuffer = API_CacheDmaMalloc((size_t)(iMaxBurst * ulBytesPerSector));
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            pvRBurstBuffer = __SHEAP_ALLOC((size_t)(iMaxBurst * ulBytesPerSector));
        }
        if (pvRBurstBuffer == LW_NULL) {
            return  (PX_ERROR);
        }
    
        pvWBurstBuffer = pvRBurstBuffer;
        
        pwp->DISKCWP_pvRBurstBuffer = pvRBurstBuffer;
        pwp->DISKCWP_pvWBurstBuffer = pvRBurstBuffer;
    
    } else {                                                            /*  ��Ҫ����д֧��              */
#if LW_CFG_CACHE_EN > 0
        if (bCacheCoherence) {
            pvRBurstBuffer = API_CacheDmaMalloc((size_t)(iMaxRBurstSector * ulBytesPerSector));
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            pvRBurstBuffer = __SHEAP_ALLOC((size_t)(iMaxRBurstSector * ulBytesPerSector));
        }
        if (pvRBurstBuffer == LW_NULL) {
            return  (PX_ERROR);
        }
    
#if LW_CFG_CACHE_EN > 0
        if (bCacheCoherence) {
            pvWBurstBuffer = API_CacheDmaMalloc((size_t)(iMaxWBurstSector * ulBytesPerSector * iMsgCount));
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            pvWBurstBuffer = __SHEAP_ALLOC((size_t)(iMaxWBurstSector * ulBytesPerSector * iMsgCount));
        }
        if (pvWBurstBuffer == LW_NULL) {
#if LW_CFG_CACHE_EN > 0
            if (bCacheCoherence) {
                API_CacheDmaFree(pvRBurstBuffer);
            } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            {
                __SHEAP_FREE(pvRBurstBuffer);
            }
            return  (PX_ERROR);
        }
        
        pwp->DISKCWP_pvRBurstBuffer = pvRBurstBuffer;
        pwp->DISKCWP_pvWBurstBuffer = pvWBurstBuffer;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskCacheWpMemDelete
** ��������: ɾ������
** �䡡��  : pwp                   ����д���߿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __diskCacheWpMemDelete (PLW_DISKCACHE_WP  pwp)
{
    if (pwp->DISKCWP_iPipeline > 0) {                                   /*  ʹ�ù����߳�                */
#if LW_CFG_CACHE_EN > 0
        if (pwp->DISKCWP_bCacheCoherence) {
            API_CacheDmaFree(pwp->DISKCWP_pvWBurstBuffer);
        } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        {
            __SHEAP_FREE(pwp->DISKCWP_pvWBurstBuffer);
        }
    }
    
#if LW_CFG_CACHE_EN > 0
    if (pwp->DISKCWP_bCacheCoherence) {
        API_CacheDmaFree(pwp->DISKCWP_pvRBurstBuffer);
    } else 
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    {
        __SHEAP_FREE(pwp->DISKCWP_pvRBurstBuffer);
    }
    
    pwp->DISKCWP_pvRBurstBuffer = LW_NULL;
    pwp->DISKCWP_pvWBurstBuffer = LW_NULL;
}
/*********************************************************************************************************
** ��������: __diskCacheWpCreate
** ��������: ����д����
** �䡡��  : pdiskc                ���̻�����ƿ�
**           pwp                   ����д���߿��ƿ�
**           bCacheCoherence       CACHE һ��������
**           bParallel             ������д֧��
**           iPipeline             д�����߳���
**           iMsgCount             ��������Ϣ����
**           iMaxRBurstSector      ��⧷�����
**           iMaxWBurstSector      д⧷�����
**           ulBytesPerSector      ÿ������С
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheWpCreate (PLW_DISKCACHE_CB  pdiskc,
                          PLW_DISKCACHE_WP  pwp, 
                          BOOL              bCacheCoherence,
                          BOOL              bParallel,
                          INT               iPipeline, 
                          INT               iMsgCount,
                          INT               iMaxRBurstSector,
                          INT               iMaxWBurstSector,
                          ULONG             ulBytesPerSector)
{
    INT     iErrLevel = 0;

    pwp->DISKCWP_bExit     = LW_FALSE;
    pwp->DISKCWP_bParallel = bParallel;
    pwp->DISKCWP_iPipeline = iPipeline;
    pwp->DISKCWP_iMsgCount = iMsgCount;
    
    if (iPipeline == 0) {
        return  (__diskCacheWpMemCreate(pwp, bCacheCoherence, iPipeline, iMsgCount,
                                        iMaxRBurstSector, iMaxWBurstSector, ulBytesPerSector));
    
    } else {                                                            /*  ��Ҫ����д֧��              */
        if (__diskCacheWpMemCreate(pwp, bCacheCoherence, iPipeline, iMsgCount,
                                   iMaxRBurstSector, iMaxWBurstSector, ulBytesPerSector)) {
            return  (PX_ERROR);
        }

        pwp->DISKCWP_hMsgQueue = API_MsgQueueCreate("dc_msgwp", (ULONG)iMsgCount,
                                                    sizeof(LW_DISKCACHE_WPMSG), 
                                                    LW_OPTION_OBJECT_GLOBAL | 
                                                    LW_OPTION_WAIT_PRIORITY,
                                                    LW_NULL);
        if (pwp->DISKCWP_hMsgQueue == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 1;
            goto    __error_handle;
        }
        
        pwp->DISKCWP_hCounter = API_SemaphoreCCreate("dc_wpcnt", (ULONG)iMsgCount, 
                                                     (ULONG)iMsgCount, 
                                                     LW_OPTION_OBJECT_GLOBAL | 
                                                     LW_OPTION_WAIT_PRIORITY,
                                                     LW_NULL);
        if (pwp->DISKCWP_hCounter == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 2;
            goto    __error_handle;
        }
        
        pwp->DISKCWP_hPart = API_PartitionCreate("dc_wppart", pwp->DISKCWP_pvWBurstBuffer,
                                                 (ULONG)iMsgCount, 
                                                 (size_t)(iMaxWBurstSector * ulBytesPerSector),
                                                 LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pwp->DISKCWP_hPart == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 3;
            goto    __error_handle;
        }
        
        pwp->DISKCWP_hSync = API_SemaphoreBCreate("dc_sync", LW_FALSE, 
                                                  LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
        if (pwp->DISKCWP_hSync == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 4;
            goto    __error_handle;
        }
        
        if (bParallel == LW_FALSE) {
            pwp->DISKCWP_hDev = API_SemaphoreMCreate("dc_devlock", 
                                                     LW_PRIO_DEF_CEILING, 
                                                     LW_OPTION_INHERIT_PRIORITY | 
                                                     LW_OPTION_DELETE_SAFE |
                                                     LW_OPTION_OBJECT_GLOBAL, LW_NULL);
            if (pwp->DISKCWP_hDev == LW_OBJECT_HANDLE_INVALID) {
                iErrLevel = 5;
                goto    __error_handle;
            }
        }
        
        if (__diskCacheWpTCreate(pdiskc, pwp)) {
            iErrLevel = 6;
            goto    __error_handle;
        }
    }
    
    return  (ERROR_NONE);
    
__error_handle:
    if (iErrLevel > 5) {
        if (bParallel == LW_FALSE) {
            API_SemaphoreMDelete(&pwp->DISKCWP_hDev);
        }
    }
    if (iErrLevel > 4) {
        API_SemaphoreBDelete(&pwp->DISKCWP_hSync);
    }
    if (iErrLevel > 3) {
        API_PartitionDelete(&pwp->DISKCWP_hPart);
    }
    if (iErrLevel > 2) {
        API_SemaphoreCDelete(&pwp->DISKCWP_hCounter);
    }
    if (iErrLevel > 1) {
        API_MsgQueueDelete(&pwp->DISKCWP_hMsgQueue);
    }
    if (iErrLevel > 0) {
        __diskCacheWpMemDelete(pwp);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __diskCacheWpDelete
** ��������: ɾ��д����
** �䡡��  : pwp                   ����д���߿��ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheWpDelete (PLW_DISKCACHE_WP  pwp)
{
    pwp->DISKCWP_bExit = LW_TRUE;

    if (pwp->DISKCWP_iPipeline == 0) {
        __diskCacheWpMemDelete(pwp);
        return  (ERROR_NONE);
    }
    
    API_MsgQueueDelete(&pwp->DISKCWP_hMsgQueue);
    __diskCacheWpTDelete(pwp);
    
    API_PartitionDelete(&pwp->DISKCWP_hPart);
    API_SemaphoreCDelete(&pwp->DISKCWP_hCounter);
    API_SemaphoreBDelete(&pwp->DISKCWP_hSync);
    __diskCacheWpMemDelete(pwp);
    
    if (pwp->DISKCWP_bParallel == LW_FALSE) {
        API_SemaphoreMDelete(&pwp->DISKCWP_hDev);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskCacheWpGetBuffer
** ��������: ���һ��⧷���������
** �䡡��  : pwp                   ����д���߿��ƿ�
**           bRead                 �Ƿ�Ϊ��, ����Ϊд
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  __diskCacheWpGetBuffer (PLW_DISKCACHE_WP  pwp, BOOL bRead)
{
    PVOID  pvRet;

    if (bRead) {
        return  (pwp->DISKCWP_pvRBurstBuffer);
    }

    if (pwp->DISKCWP_iPipeline == 0) {
        return  (pwp->DISKCWP_pvWBurstBuffer);
    }
    
    if (API_SemaphoreCPend(pwp->DISKCWP_hCounter, LW_OPTION_WAIT_INFINITE)) {
        _BugHandle(LW_TRUE, LW_TRUE, "diskcache pipeline error!\r\n");
    }
    
    pvRet = API_PartitionGet(pwp->DISKCWP_hPart);
    _BugHandle((pvRet == LW_NULL), LW_TRUE, "diskcache pipeline error!\r\n");
    
    return  (pvRet);
}
/*********************************************************************************************************
** ��������: __diskCacheWpGetBuffer
** ��������: ����һ��δʹ�õ�⧷���������
** �䡡��  : pwp                   ����д���߿��ƿ�
**           pvBuffer              ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __diskCacheWpPutBuffer (PLW_DISKCACHE_WP  pwp, PVOID  pvBuffer)
{
    if ((pvBuffer == pwp->DISKCWP_pvRBurstBuffer) ||
        (pwp->DISKCWP_iPipeline == 0)) {
        return;
    }
    
    API_PartitionPut(pwp->DISKCWP_hPart, pvBuffer);
    API_SemaphoreCPost(pwp->DISKCWP_hCounter);
}
/*********************************************************************************************************
** ��������: __diskCacheWpRead
** ��������: ������
** �䡡��  : pdiskc                ���̻�����ƿ�
**           pblkd                 ���������豸
**           pvBuffer              ������
**           ulStartSector         ��ʼ����
**           ulNSector             ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheWpRead (PLW_DISKCACHE_CB  pdiskc,
                        PLW_BLK_DEV       pblkd,
                        PVOID             pvBuffer,
                        ULONG             ulStartSector,
                        ULONG             ulNSector)
{
             INT                iRet;
    REGISTER PLW_DISKCACHE_WP   pwp = &pdiskc->DISKC_wpWrite;
    
    if (pwp->DISKCWP_iPipeline && !pwp->DISKCWP_bParallel) {
        API_SemaphoreMPend(pwp->DISKCWP_hDev, LW_OPTION_WAIT_INFINITE);
    }
    
    iRet = __PHYDISK_READ(pblkd, pvBuffer, ulStartSector, ulNSector);
                                                    
    if (pwp->DISKCWP_iPipeline && !pwp->DISKCWP_bParallel) {
        API_SemaphoreMPost(pwp->DISKCWP_hDev);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __diskCacheWpWrite
** ��������: �ύһ��д����
** �䡡��  : pdiskc                ���̻�����ƿ�
**           pblkdDisk             ���������豸
**           pvBuffer              ������
**           ulStartSector         ��ʼ����
**           ulNSector             ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheWpWrite (PLW_DISKCACHE_CB  pdiskc,
                         PLW_BLK_DEV       pblkdDisk,
                         PVOID             pvBuffer,
                         ULONG             ulStartSector,
                         ULONG             ulNSector)
{
    LW_DISKCACHE_WPMSG  diskcwpm;
    PLW_DISKCACHE_WP    pwp = &pdiskc->DISKC_wpWrite;

    if (pwp->DISKCWP_iPipeline == 0) {
        return  (pdiskc->DISKC_pblkdDisk->BLKD_pfuncBlkWrt(pblkdDisk, 
                                                           pvBuffer,
                                                           ulStartSector,
                                                           ulNSector));
    }
    
    diskcwpm.DISKCWPM_ulStartSector = ulStartSector;
    diskcwpm.DISKCWPM_ulNSector     = ulNSector;
    diskcwpm.DISKCWPM_pvBuffer      = pvBuffer;
    
    API_MsgQueueSend2(pwp->DISKCWP_hMsgQueue, &diskcwpm, 
                      sizeof(LW_DISKCACHE_WPMSG), LW_OPTION_WAIT_INFINITE);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskCacheWpSync
** ��������: ���һ��⧷���������
** �䡡��  : pwp                   ����д���߿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __diskCacheWpSync (PLW_DISKCACHE_WP  pwp)
{
    ULONG      ulError;
    ULONG      ulCounter;

    if (pwp->DISKCWP_iPipeline == 0) {
        return;
    }
    
    do {
        if (API_PartitionStatus(pwp->DISKCWP_hPart, LW_NULL, 
                                &ulCounter, LW_NULL)) {
            break;
        }
        if (ulCounter >= pwp->DISKCWP_iMsgCount) {
            break;
        }
        ulError = API_SemaphoreBPend(pwp->DISKCWP_hSync, LW_OPTION_WAIT_INFINITE);
    } while (ulError == ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
