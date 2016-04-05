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
** ��   ��   ��: diskCacheLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 11 �� 26 ��
**
** ��        ��: ���̸��ٻ���������ڲ�������.

** BUG
2008.12.16  __diskCacheFlushInvalidate2() ����ָ���Ĵ���Ϊ��д������������.
2009.03.16  �����д��·��֧��.
2009.03.18  FIOCANCEL �� CACHE ֹͣ, û�л�д������ȫ������. �ڴ�ص���ʼ״̬.
2009.07.22  ���Ӷ� FIOSYNC ��֧��.
2009.11.03  FIOCANCEL ����Ӧ�ô�����������, ʹ�������������.
            FIODISKCHANGE �� FIOCANCEL ������ͬ.
2015.02.05  ���� __diskCacheNodeReadData() �������жϴ���.
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
  �ڲ������
*********************************************************************************************************/
#define __LW_DISKCACHE_DISK_READ(pdiskcDiskCache)               \
        pdiskcDiskCache->DISKC_pblkdDisk->BLKD_pfuncBlkRd
#define __LW_DISKCACHE_DISK_WRITE(pdiskcDiskCache)              \
        pdiskcDiskCache->DISKC_pblkdDisk->BLKD_pfuncBlkWrt
#define __LW_DISKCACHE_DISK_IOCTL(pdiskcDiskCache)              \
        pdiskcDiskCache->DISKC_pblkdDisk->BLKD_pfuncBlkIoctl
#define __LW_DISKCACHE_DISK_RESET(pdiskcDiskCache)              \
        pdiskcDiskCache->DISKC_pblkdDisk->BLKD_pfuncBlkReset
#define __LW_DISKCACHE_DISK_STATUS(pdiskcDiskCache)             \
        pdiskcDiskCache->DISKC_pblkdDisk->BLKD_pfuncBlkStatusChk
/*********************************************************************************************************
** ��������: __diskCacheMemReset
** ��������: ���³�ʼ������ CACHE �������ڴ������
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __diskCacheMemReset (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    REGISTER INT                    i;
    REGISTER PLW_DISKCACHE_NODE     pdiskn = (PLW_DISKCACHE_NODE)pdiskcDiskCache->DISKC_pcCacheNodeMem;
    REGISTER PCHAR                  pcData = (PCHAR)pdiskcDiskCache->DISKC_pcCacheMem;
    
    pdiskcDiskCache->DISKC_ulDirtyCounter = 0;                          /*  û����Ҫ��д������          */
    
    pdiskcDiskCache->DISKC_pringLruHeader = LW_NULL;                    /*  ��ʼ�� LRU ��               */
    lib_bzero(pdiskcDiskCache->DISKC_pplineHash,
              (sizeof(PVOID) * pdiskcDiskCache->DISKC_iHashSize));      /*  ��ʼ�� HASH ��              */
    
    for (i = 0; i < pdiskcDiskCache->DISKC_ulNCacheNode; i++) {
        pdiskn->DISKN_ulSectorNo = (ULONG)PX_ERROR;
        pdiskn->DISKN_iStatus    = 0;
        pdiskn->DISKN_pcData     = pcData;
        
        _List_Ring_Add_Last(&pdiskn->DISKN_ringLru, 
                            &pdiskcDiskCache->DISKC_pringLruHeader);    /*  ���� LRU ��                 */
        _LIST_LINE_INIT_IN_CODE(pdiskn->DISKN_lineHash);                /*  ��ʼ�� HASH ��              */
    
        pdiskn++;
        pcData += pdiskcDiskCache->DISKC_ulBytesPerSector;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskCacheMemInit
** ��������: ��ʼ������ CACHE �������ڴ������
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           pvDiskCacheMem     ������
**           ulBytesPerSector   ������С
**           ulNSector          ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheMemInit (PLW_DISKCACHE_CB   pdiskcDiskCache,
                         VOID              *pvDiskCacheMem,
                         ULONG              ulBytesPerSector,
                         ULONG              ulNSector)
{
    REGISTER PCHAR                  pcData = (PCHAR)pvDiskCacheMem;

    pdiskcDiskCache->DISKC_ulNCacheNode = ulNSector;
    pdiskcDiskCache->DISKC_pcCacheMem   = (caddr_t)pcData;              /*  CACHE ������                */
    
    pdiskcDiskCache->DISKC_pcCacheNodeMem = (caddr_t)__SHEAP_ALLOC(sizeof(LW_DISKCACHE_NODE) * 
                                                                   (size_t)ulNSector);
    if (pdiskcDiskCache->DISKC_pcCacheNodeMem == LW_NULL) {
        return  (PX_ERROR);
    }
    
    return  (__diskCacheMemReset(pdiskcDiskCache));
}
/*********************************************************************************************************
** ��������: __diskCacheHashAdd
** ��������: ��ָ�� CACHE �ڵ���뵽��ص� HASH ����
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           pdiskn             ��Ҫ����� CACHE �ڵ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __diskCacheHashAdd (PLW_DISKCACHE_CB   pdiskcDiskCache, PLW_DISKCACHE_NODE  pdiskn)
{
    REGISTER PLW_LIST_LINE      *pplineHashEntry;
    
    pplineHashEntry = &pdiskcDiskCache->DISKC_pplineHash[
                       pdiskn->DISKN_ulSectorNo % 
                       pdiskcDiskCache->DISKC_iHashSize];               /*  ��� HASH �����            */
                     
    _List_Line_Add_Ahead(&pdiskn->DISKN_lineHash, pplineHashEntry);     /*  ���� HASH ��                */
}
/*********************************************************************************************************
** ��������: __diskCacheHashRemove
** ��������: ��ָ�� CACHE �ڵ���뵽��ص� HASH ����
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           pdiskn             ��Ҫ����� CACHE �ڵ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __diskCacheHashRemove (PLW_DISKCACHE_CB   pdiskcDiskCache, PLW_DISKCACHE_NODE  pdiskn)
{
    REGISTER PLW_LIST_LINE      *pplineHashEntry;
    
    pplineHashEntry = &pdiskcDiskCache->DISKC_pplineHash[
                       pdiskn->DISKN_ulSectorNo % 
                       pdiskcDiskCache->DISKC_iHashSize];               /*  ��� HASH �����            */
                       
    _List_Line_Del(&pdiskn->DISKN_lineHash, pplineHashEntry);
    
    pdiskn->DISKN_ulSectorNo = (ULONG)PX_ERROR;
    pdiskn->DISKN_iStatus    = 0;                                       /*  ����λ����Чλ              */
}
/*********************************************************************************************************
** ��������: __diskCacheHashFind
** ��������: �� HASH ����Ѱ��ָ�����Ե�����
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           ulSectorNo         �����ı��
** �䡡��  : Ѱ�ҵ��Ľڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_DISKCACHE_NODE  __diskCacheHashFind (PLW_DISKCACHE_CB   pdiskcDiskCache, ULONG  ulSectorNo)
{
    REGISTER PLW_LIST_LINE          plineHash;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    
    plineHash = pdiskcDiskCache->DISKC_pplineHash[
                ulSectorNo % 
                pdiskcDiskCache->DISKC_iHashSize];                      /*  ��� HASH �����            */
                       
    for (; plineHash != LW_NULL; plineHash = _list_line_get_next(plineHash)) {
        pdiskn = _LIST_ENTRY(plineHash, LW_DISKCACHE_NODE, DISKN_lineHash);
        if (pdiskn->DISKN_ulSectorNo == ulSectorNo) {
            return  (pdiskn);                                           /*  Ѱ�ҵ�                      */
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __diskCacheLruFind
** ��������: �� LRU ����Ѱ��ָ�����Ե�����
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           ulSectorNo         �����ı��
** �䡡��  : Ѱ�ҵ��Ľڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_DISKCACHE_NODE  __diskCacheLruFind (PLW_DISKCACHE_CB   pdiskcDiskCache, ULONG  ulSectorNo)
{
             PLW_LIST_RING          pringLruHeader = pdiskcDiskCache->DISKC_pringLruHeader;
    REGISTER PLW_LIST_RING          pringTemp;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    
    if (_LIST_RING_IS_EMPTY(pringLruHeader)) {
        return  (LW_NULL);
    }
    
    pdiskn = (PLW_DISKCACHE_NODE)pringLruHeader;
    if (pdiskn->DISKN_ulSectorNo == ulSectorNo) {
        return  (pdiskn);                                               /*  Ѱ�ҵ�                      */
    }
    
    for (pringTemp  = _list_ring_get_next(pringLruHeader);
         pringTemp != pringLruHeader;
         pringTemp  = _list_ring_get_next(pringTemp)) {                 /*  ���� LRU ��                 */
         
        pdiskn = (PLW_DISKCACHE_NODE)pringTemp;
        if (pdiskn->DISKN_ulSectorNo == ulSectorNo) {
            return  (pdiskn);                                           /*  Ѱ�ҵ�                      */
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __diskCacheSortListAdd
** ��������: ��ָ�� CACHE �ڵ���뵽һ�����������������
** �䡡��  : pringListHeader    �����ͷ
**           pdiskn             ��Ҫ����� CACHE �ڵ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __diskCacheSortListAdd (PLW_LIST_RING  *ppringListHeader, PLW_DISKCACHE_NODE  pdiskn)
{
    REGISTER PLW_LIST_RING          pringTemp;
             PLW_LIST_RING          pringDummyHeader;
    REGISTER PLW_DISKCACHE_NODE     pdisknTemp;

    if (_LIST_RING_IS_EMPTY(*ppringListHeader)) {
        _List_Ring_Add_Ahead(&pdiskn->DISKN_ringLru, 
                             ppringListHeader);
        return;
    }
    
    pdisknTemp = (PLW_DISKCACHE_NODE)(*ppringListHeader);               /*  LRU ��Ϊ CACHE �ڵ��׸�Ԫ�� */
    if (pdiskn->DISKN_ulSectorNo < pdisknTemp->DISKN_ulSectorNo) {
        _List_Ring_Add_Ahead(&pdiskn->DISKN_ringLru, 
                             ppringListHeader);                         /*  ֱ�Ӳ����ͷ                */
        return;
    }
    
    for (pringTemp  = _list_ring_get_next(*ppringListHeader);
         pringTemp != *ppringListHeader;
         pringTemp  = _list_ring_get_next(pringTemp)) {                 /*  �ӵڶ����ڵ����Բ���        */
        
        pdisknTemp = (PLW_DISKCACHE_NODE)pringTemp;
        if (pdiskn->DISKN_ulSectorNo < pdisknTemp->DISKN_ulSectorNo) {
            pringDummyHeader = pringTemp;
            _List_Ring_Add_Last(&pdiskn->DISKN_ringLru,
                                &pringDummyHeader);                     /*  ���뵽���ʵ�λ��            */
            return;
        }
    }
    
    _List_Ring_Add_Last(&pdiskn->DISKN_ringLru,
                        ppringListHeader);                              /*  ���뵽�������              */
}
/*********************************************************************************************************
** ��������: __diskCacheFlushList
** ��������: ��ָ�������ڵ� CACHE �ڵ�����ȫ����д����
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           pringFlushHeader   ����ͷ
**           bMakeInvalidate    �Ƿ��ڻ�д����ؽڵ���Ϊ��Ч״̬
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __diskCacheFlushList (PLW_DISKCACHE_CB   pdiskcDiskCache, 
                                  PLW_LIST_RING      pringFlushHeader,
                                  BOOL               bMakeInvalidate)
{
    REGISTER INT                    i;

             PLW_DISKCACHE_NODE     pdiskn;
    REGISTER PLW_DISKCACHE_NODE     pdisknContinue;
             PLW_LIST_RING          pringTemp;
             
    REGISTER INT                    iBurstCount;
             PCHAR                  pcBurstBuffer;
             INT                    iRetVal;
             BOOL                   bHasError = LW_FALSE;
    
    for (pringTemp  = pringFlushHeader;
         pringTemp != LW_NULL;
         pringTemp  = pringFlushHeader) {                               /*  ֱ������Ϊ��Ϊֹ            */
        
        pdiskn = (PLW_DISKCACHE_NODE)pringTemp;                         /*  DISKN_ringLru �ǵ�һ��Ԫ��  */
        pdisknContinue = pdiskn;
        
        for (iBurstCount = 1; 
             iBurstCount < pdiskcDiskCache->DISKC_iMaxWBurstSector;
             iBurstCount++) {
            
            pdisknContinue = 
                (PLW_DISKCACHE_NODE)_list_ring_get_next(&pdisknContinue->DISKN_ringLru);
            if (pdisknContinue->DISKN_ulSectorNo != 
                (pdiskn->DISKN_ulSectorNo + iBurstCount)) {             /*  �ж��Ƿ�Ϊ��������          */
                break;
            }
        }
        
        if (iBurstCount == 1) {                                         /*  ���ܽ���⧷�����            */
            lib_memcpy(pdiskcDiskCache->DISKC_pcBurstBuffer,
                       pdiskn->DISKN_pcData,
                       (size_t)pdiskcDiskCache->DISKC_ulBytesPerSector);
        
        } else {                                                        /*  ����ʹ��⧷�д��            */
            pcBurstBuffer  = (PCHAR)pdiskcDiskCache->DISKC_pcBurstBuffer;
            pdisknContinue = pdiskn;
            
            for (i = 0; i < iBurstCount; i++) {                         /*  װ��⧷���������            */
                lib_memcpy(pcBurstBuffer, 
                       pdisknContinue->DISKN_pcData, 
                       (size_t)pdiskcDiskCache->DISKC_ulBytesPerSector);/*  ��������                    */
                           
                pdisknContinue = 
                    (PLW_DISKCACHE_NODE)_list_ring_get_next(&pdisknContinue->DISKN_ringLru);
                pcBurstBuffer += pdiskcDiskCache->DISKC_ulBytesPerSector;
            }
        }
        
        iRetVal = __LW_DISKCACHE_DISK_WRITE(pdiskcDiskCache)(
                                pdiskcDiskCache->DISKC_pblkdDisk, 
                                pdiskcDiskCache->DISKC_pcBurstBuffer,
                                pdiskn->DISKN_ulSectorNo,
                                iBurstCount);                           /*  ⧷�д������                */
        if (iRetVal < 0) {
            bHasError       = LW_TRUE;
            bMakeInvalidate = LW_TRUE;                                  /*  ��дʧ��, ��Ҫ��Ч�豸      */
        }
        
        for (i = 0; i < iBurstCount; i++) {                             /*  ��ʼ������Щ��д��������¼  */
            __LW_DISKCACHE_CLR_DIRTY(pdiskn);
            
            if (bMakeInvalidate) {
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);         /*  ����Ч�� HASH ����ɾ��      */
            }
            
            pdisknContinue = 
                (PLW_DISKCACHE_NODE)_list_ring_get_next(&pdiskn->DISKN_ringLru);
                
            _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                           &pringFlushHeader);                          /*  ����ʱ��д������ɾ��        */
            _List_Ring_Add_Last(&pdiskn->DISKN_ringLru, 
                           &pdiskcDiskCache->DISKC_pringLruHeader);     /*  ���뵽���� CACHE LRU ����   */
                                 
            pdiskn = pdisknContinue;                                    /*  ��һ�������ڵ�              */
        }
        
        pdiskcDiskCache->DISKC_ulDirtyCounter -= iBurstCount;           /*  ���¼�����Ҫ��д��������    */
    }
    
    if (bHasError) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __diskCacheFlushInvalidate
** ��������: ��ָ������ CACHE ��ָ��������Χ��д���Ұ�Ѱ������Ϊ��Ч.
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           ulStartSector      ��ʼ����
**           ulEndSector        ��������
**           bMakeFlush         �Ƿ���� FLUSH ����
**           bMakeInvalidate    �Ƿ���� INVALIDATE ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __diskCacheFlushInvalidate (PLW_DISKCACHE_CB   pdiskcDiskCache,
                                        ULONG              ulStartSector,
                                        ULONG              ulEndSector,
                                        BOOL               bMakeFlush,
                                        BOOL               bMakeInvalidate)
{
             INT                    i;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    REGISTER PLW_DISKCACHE_NODE     pdisknPrev;
             PLW_LIST_RING          pringFlushHeader = LW_NULL;
                                                                        /*  ��������û��ʹ�õĿ�ʼ    */
    pdiskn = (PLW_DISKCACHE_NODE)_list_ring_get_prev(pdiskcDiskCache->DISKC_pringLruHeader);
    
    for (i = 0; i < pdiskcDiskCache->DISKC_ulNCacheNode; i++) {
    
        pdisknPrev = (PLW_DISKCACHE_NODE)_list_ring_get_prev(&pdiskn->DISKN_ringLru);
        
        if ((pdiskn->DISKN_ulSectorNo > ulEndSector) ||
            (pdiskn->DISKN_ulSectorNo < ulStartSector)) {               /*  �����趨�ķ�Χ��            */
            goto    __check_again;
        }
        
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn)) {                          /*  �������Ҫ��д              */
            if (bMakeFlush) {
                _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                               &pdiskcDiskCache->DISKC_pringLruHeader);
                __diskCacheSortListAdd(&pringFlushHeader, pdiskn);      /*  ���뵽����ȴ���д����      */
            }
            goto    __check_again;
        }
        
        if (__LW_DISKCACHE_IS_VALID(pdiskn)) {                          /*  ����Ƿ���Ч                */
            if (bMakeInvalidate) {
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);         /*  ����Ч�� HASH ����ɾ��      */
            }
        }
        
__check_again:
        pdiskn = pdisknPrev;                                            /*  ���� LRU ��                 */
    }
    
    if (!_LIST_RING_IS_EMPTY(pringFlushHeader)) {                       /*  �Ƿ���Ҫ��д                */
        return  (__diskCacheFlushList(pdiskcDiskCache, pringFlushHeader, bMakeInvalidate));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __diskCacheFlushInvalidate2
** ��������: ��ָ������ CACHE ��ָ��������Χ��д���Ұ�Ѱ������Ϊ��Ч. (����2)
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           iLruSectorNum      ���δʹ�õ���������
**           bMakeFlush         �Ƿ���� FLUSH ����
**           bMakeInvalidate    �Ƿ���� INVALIDATE ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __diskCacheFlushInvalidate2 (PLW_DISKCACHE_CB   pdiskcDiskCache,
                                         INT                iLruSectorNum,
                                         BOOL               bMakeFlush,
                                         BOOL               bMakeInvalidate)
{
             INT                    i = 0;
             
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    REGISTER PLW_DISKCACHE_NODE     pdisknPrev;
             PLW_LIST_RING          pringFlushHeader = LW_NULL;
             
    if (iLruSectorNum < 1) {
        return  (ERROR_NONE);                                           /*  ����Ҫ����                  */
    }
                                                                        /*  ��������û��ʹ�õĿ�ʼ    */
    pdiskn = (PLW_DISKCACHE_NODE)_list_ring_get_prev(pdiskcDiskCache->DISKC_pringLruHeader);
    
    for (i = 0; 
         (i < pdiskcDiskCache->DISKC_ulNCacheNode) && (iLruSectorNum > 0);
         i++) {
    
        pdisknPrev = (PLW_DISKCACHE_NODE)_list_ring_get_prev(&pdiskn->DISKN_ringLru);
        
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn)) {                          /*  �������Ҫ��д              */
            if (bMakeFlush) {
                _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                               &pdiskcDiskCache->DISKC_pringLruHeader);
                __diskCacheSortListAdd(&pringFlushHeader, pdiskn);      /*  ���뵽����ȴ���д����      */
            }
            iLruSectorNum--;
            goto    __check_again;
        }
        
        if (__LW_DISKCACHE_IS_VALID(pdiskn)) {                          /*  ����Ƿ���Ч                */
            if (bMakeInvalidate) {
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);         /*  ����Ч�� HASH ����ɾ��      */
                iLruSectorNum--;
            }
        }
    
__check_again:
        pdiskn = pdisknPrev;                                            /*  ���� LRU ��                 */
    }
    
    if (!_LIST_RING_IS_EMPTY(pringFlushHeader)) {                       /*  �Ƿ���Ҫ��д                */
        return  (__diskCacheFlushList(pdiskcDiskCache, pringFlushHeader, bMakeInvalidate));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __diskCacheNodeFind
** ��������: Ѱ��һ��ָ���� CACHE ��
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           ulSectorNo         ������
** �䡡��  : Ѱ�ҵ��Ľڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_DISKCACHE_NODE  __diskCacheNodeFind (PLW_DISKCACHE_CB   pdiskcDiskCache, ULONG  ulSectorNo)
{
    REGISTER PLW_DISKCACHE_NODE         pdiskn;
    
    pdiskn = __diskCacheHashFind(pdiskcDiskCache, ulSectorNo);          /*  ���ȴ���Ч�� HASH ���в���  */
    if (pdiskn) {
        return  (pdiskn);                                               /*  HASH ������                 */
    }
    
    pdiskn = __diskCacheLruFind(pdiskcDiskCache, ulSectorNo);           /*  ��ʼ�� LRU ��������         */
    
    return  (pdiskn);
}
/*********************************************************************************************************
** ��������: __diskCacheNodeAlloc
** ��������: ����һ�� CACHE ��
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           ulSectorNo         ������
**           iExpectation       ����Ԥ���ٵĶ������������ (δ��, ʹ�� DISKC_iMaxBurstSector)
** �䡡��  : �������Ľڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_DISKCACHE_NODE  __diskCacheNodeAlloc (PLW_DISKCACHE_CB   pdiskcDiskCache, 
                                                 ULONG              ulSectorNo,
                                                 INT                iExpectation)
{
    REGISTER PLW_LIST_RING          pringTemp;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
    
__check_again:                                                          /*  ��������û��ʹ�õĿ�ʼ    */
    for (pringTemp  = _list_ring_get_prev(pdiskcDiskCache->DISKC_pringLruHeader);
         pringTemp != pdiskcDiskCache->DISKC_pringLruHeader;
         pringTemp  = _list_ring_get_prev(pringTemp)) {
        
        pdiskn = (PLW_DISKCACHE_NODE)pringTemp;
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn) == 0) {                     /*  ����Ҫ��д�����ݿ�          */
            break;
        }
    }
    
    if (pringTemp == pdiskcDiskCache->DISKC_pringLruHeader) {           /*  û�к��ʵĿ��ƿ�            */
        pdiskn = (PLW_DISKCACHE_NODE)pringTemp;
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn)) {                          /*  ��ͷҲ����ʹ��              */
            REGISTER INT   iWriteNum = pdiskcDiskCache->DISKC_iMaxWBurstSector;
            __diskCacheFlushInvalidate2(pdiskcDiskCache, 
                                        iWriteNum,
                                        LW_TRUE, LW_FALSE);             /*  ��дһЩ����������          */
            goto    __check_again;
        }
    }

    if (__LW_DISKCACHE_IS_VALID(pdiskn)) {                              /*  �Ƿ�Ϊ��Ч����              */
        __diskCacheHashRemove(pdiskcDiskCache, pdiskn);                 /*  ����Ч����ɾ��              */
    }

    pdiskn->DISKN_ulSectorNo = ulSectorNo;
    __LW_DISKCACHE_SET_VALID(pdiskn);
    
    __diskCacheHashAdd(pdiskcDiskCache, pdiskn);
    
    return  (pdiskn);
}
/*********************************************************************************************************
** ��������: __diskCacheNodeReadData
** ��������: �Ӵ����ж�ȡ������д�� CACHE ����.
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           pdiskn             CACHE ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __diskCacheNodeReadData (PLW_DISKCACHE_CB  pdiskcDiskCache, PLW_DISKCACHE_NODE  pdiskn)
{
    REGISTER INT        i;
    REGISTER INT        iNSector;
             INT        iRetVal;
             ULONG      ulStartSector = pdiskn->DISKN_ulSectorNo;
             PCHAR      pcData;
             
    iNSector = (INT)__MIN((ULONG)pdiskcDiskCache->DISKC_iMaxRBurstSector, 
                     (ULONG)((pdiskcDiskCache->DISKC_ulNCacheNode - 
                      pdiskcDiskCache->DISKC_ulDirtyCounter)));         /*  ��ö������ĸ���            */
                      
    iNSector = (INT)__MIN((ULONG)iNSector,                              /*  �����޷������Ƚ�            */
                     (ULONG)(pdiskcDiskCache->DISKC_ulEndStector - 
                      pdiskn->DISKN_ulSectorNo));                       /*  ���ܳ�Խ���һ������        */
                      
    if (iNSector <= 0) {
        return  (PX_ERROR);
    }

    iRetVal = __LW_DISKCACHE_DISK_READ(pdiskcDiskCache)(
                                       pdiskcDiskCache->DISKC_pblkdDisk, 
                                       pdiskcDiskCache->DISKC_pcBurstBuffer,
                                       pdiskn->DISKN_ulSectorNo,
                                       iNSector);                       /*  ������ȡ�������������      */
    if (iRetVal < 0) {
        return  (iRetVal);
    }
    
    i      = 0;
    pcData = pdiskcDiskCache->DISKC_pcBurstBuffer;
    do {
        lib_memcpy(pdiskn->DISKN_pcData, pcData, 
                   (UINT)pdiskcDiskCache->DISKC_ulBytesPerSector);      /*  ��������                    */
        
        _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                       &pdiskcDiskCache->DISKC_pringLruHeader);         /*  ����ȷ�� LRU ��λ��         */
        _List_Ring_Add_Ahead(&pdiskn->DISKN_ringLru, 
                       &pdiskcDiskCache->DISKC_pringLruHeader);
    
        i++;
        pcData += pdiskcDiskCache->DISKC_ulBytesPerSector;
        
        if (i < iNSector) {                                             /*  ����Ҫ��������              */
            pdiskn = __diskCacheNodeFind(pdiskcDiskCache, (ulStartSector + i));
            if (pdiskn) {                                               /*  ��һ������������Ч          */
                break;
            }
            pdiskn = __diskCacheNodeAlloc(pdiskcDiskCache, 
                                          (ulStartSector + i), 
                                          (iNSector - i));              /*  ���¿���һ���յĽڵ�        */
            if (pdiskn == LW_NULL) {
                break;
            }
        }
    } while (i < iNSector);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __diskCacheNodeGet
** ��������: ��ȡһ������ָ������� CACHE �ڵ�
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           ulSectorNo         ������
**           iExpectation       ����Ԥ���ٵĶ������������
**           bIsRead            �Ƿ�Ϊ������
** �䡡��  : �ڵ㴦�����Ľڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_DISKCACHE_NODE  __diskCacheNodeGet (PLW_DISKCACHE_CB   pdiskcDiskCache, 
                                               ULONG              ulSectorNo,
                                               INT                iExpectation,
                                               BOOL               bIsRead)
{
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
             BOOL                   bIsNewNode = LW_FALSE;
             INT                    iError;
    
    pdiskn = __diskCacheNodeFind(pdiskcDiskCache, ulSectorNo);
    if (pdiskn) {                                                       /*  ֱ������                    */
        goto    __data_op;
    }
    
    pdiskn = __diskCacheNodeAlloc(pdiskcDiskCache, ulSectorNo, iExpectation);
    if (pdiskn == LW_NULL) {                                            /*  �����½ڵ�ʧ��              */
        return  (LW_NULL);
    }
    bIsNewNode = LW_TRUE;
    
__data_op:
    if (bIsRead) {                                                      /*  ��ȡ����                    */
        if (bIsNewNode) {                                               /*  ��Ҫ�Ӵ��̶�ȡ����          */
            iError = __diskCacheNodeReadData(pdiskcDiskCache, pdiskn);
            if (iError < 0) {                                           /*  ��ȡ����                    */
                __diskCacheHashRemove(pdiskcDiskCache, pdiskn);
                return  (LW_NULL);
            }
        }
    
    } else {                                                            /*  д����                      */
        if (__LW_DISKCACHE_IS_DIRTY(pdiskn) == 0) {
            __LW_DISKCACHE_SET_DIRTY(pdiskn);                           /*  ������λ��־                */
            pdiskcDiskCache->DISKC_ulDirtyCounter++;
        }
    }
    
    pdiskcDiskCache->DISKC_disknLuck = pdiskn;                          /*  �������˽ڵ�                */
    
    _List_Ring_Del(&pdiskn->DISKN_ringLru, 
                   &pdiskcDiskCache->DISKC_pringLruHeader);             /*  ����ȷ�� LRU ��λ��         */
    _List_Ring_Add_Ahead(&pdiskn->DISKN_ringLru, 
                   &pdiskcDiskCache->DISKC_pringLruHeader);
                   
    return  (pdiskn);
}
/*********************************************************************************************************
** ��������: __diskCacheRead
** ��������: ͨ������ CACHE ��ȡ���̵�ָ������
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           pvBuffer           ������
**           ulStartSector      ��ʼ����
**           ulSectorCount      ������������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheRead (PLW_DISKCACHE_CB   pdiskcDiskCache,
                      VOID              *pvBuffer, 
                      ULONG              ulStartSector, 
                      ULONG              ulSectorCount)
{
             INT                    i;
             INT                    iError = ERROR_NONE;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
             PCHAR                  pcData = (PCHAR)pvBuffer;

    __LW_DISKCACHE_LOCK(pdiskcDiskCache);                               /*  �������                    */
    if (pdiskcDiskCache->DISKC_iCacheOpt & LW_DISKCACHE_OPT_ENABLE) {
        for (i = 0; i < ulSectorCount; i++) {
            pdiskn = __diskCacheNodeGet(pdiskcDiskCache, 
                                        (ulStartSector + i), 
                                        (INT)(ulSectorCount - i),
                                        LW_TRUE);
            if (pdiskn == LW_NULL) {
                iError =  PX_ERROR;
                break;
            }
            
            lib_memcpy(pcData, pdiskn->DISKN_pcData,
                       (UINT)pdiskcDiskCache->DISKC_ulBytesPerSector);  /*  ��������                    */
                       
            pcData += pdiskcDiskCache->DISKC_ulBytesPerSector;
        }
    } else {
        iError = __LW_DISKCACHE_DISK_READ(pdiskcDiskCache)(
                                          pdiskcDiskCache->DISKC_pblkdDisk, 
                                          pvBuffer,
                                          ulStartSector,
                                          ulSectorCount);               /*  ������ȡ�������������      */
    }
    __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                             /*  ����                        */

    return  (iError);
}
/*********************************************************************************************************
** ��������: __diskCacheWrite
** ��������: ͨ������ CACHE д����̵�ָ������
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
**           pvBuffer           ������
**           ulStartSector      ��ʼ����
**           ulSectorCount      ������������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheWrite (PLW_DISKCACHE_CB   pdiskcDiskCache,
                       VOID              *pvBuffer, 
                       ULONG              ulStartSector, 
                       ULONG              ulSectorCount)
{
             INT                    i;
             INT                    iError = ERROR_NONE;
    REGISTER PLW_DISKCACHE_NODE     pdiskn;
             PCHAR                  pcData = (PCHAR)pvBuffer;
             
    __LW_DISKCACHE_LOCK(pdiskcDiskCache);                               /*  �������                    */
    if (pdiskcDiskCache->DISKC_iCacheOpt & LW_DISKCACHE_OPT_ENABLE) {
        for (i = 0; i < ulSectorCount; i++) {
            pdiskn = __diskCacheNodeGet(pdiskcDiskCache, 
                                        (ulStartSector + i), 
                                        (INT)(ulSectorCount - i),
                                        LW_FALSE);
                                        
            if (pdiskn == LW_NULL) {
                iError =  PX_ERROR;
                break;
            }
            
            if (__LW_DISKCACHE_IS_DIRTY(pdiskn) == 0) {
                __LW_DISKCACHE_SET_DIRTY(pdiskn);                       /*  ������λ��־                */
                pdiskcDiskCache->DISKC_ulDirtyCounter++;
            }
            
            lib_memcpy(pdiskn->DISKN_pcData, pcData, 
                       (UINT)pdiskcDiskCache->DISKC_ulBytesPerSector);  /*  д������                    */
                       
            pcData += pdiskcDiskCache->DISKC_ulBytesPerSector;
        }
    } else {
        iError = __LW_DISKCACHE_DISK_WRITE(pdiskcDiskCache)(
                                           pdiskcDiskCache->DISKC_pblkdDisk, 
                                           pvBuffer,
                                           ulStartSector,
                                           ulSectorCount);              /*  д��������                */
    }
    __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                             /*  ����                        */

    return  (iError);
}
/*********************************************************************************************************
** ��������: __diskCacheIoctl
** ��������: ͨ������ CACHE ����һ������
** �䡡��  : pdiskcDiskCache   ���� CACHE ���ƿ�
**           iCmd              ��������
**           lArg              ���Ʋ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheIoctl (PLW_DISKCACHE_CB   pdiskcDiskCache, INT  iCmd, LONG  lArg)
{
    REGISTER INT            iError = PX_ERROR;
             PLW_BLK_RANGE  pblkrange;

    __LW_DISKCACHE_LOCK(pdiskcDiskCache);                               /*  �������                    */

    switch (iCmd) {
    
    case LW_BLKD_DISKCACHE_GET_OPT:
        *(INT *)lArg = pdiskcDiskCache->DISKC_iCacheOpt;
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  ����                        */
        return  (ERROR_NONE);
        
    case LW_BLKD_DISKCACHE_SET_OPT:
        if ((pdiskcDiskCache->DISKC_iCacheOpt & LW_DISKCACHE_OPT_ENABLE) &&
            (!((INT)lArg & LW_DISKCACHE_OPT_ENABLE))) {                 /*  ��Ҫ��д����                */
            __diskCacheFlushInvalidate(pdiskcDiskCache,
                                       0, (ULONG)PX_ERROR,
                                       LW_TRUE, LW_TRUE);               /*  ȫ����д��ʹ��Ч            */
        }
        pdiskcDiskCache->DISKC_iCacheOpt = (INT)lArg;
        __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                         /*  ����                        */
        return  (ERROR_NONE);
        
    case LW_BLKD_DISKCACHE_INVALID:                                     /*  ȫ��������                  */
        iError = ERROR_NONE;
        __diskCacheFlushInvalidate(pdiskcDiskCache,
                                   0, (ULONG)PX_ERROR,
                                   LW_TRUE, LW_TRUE);
        break;
        
    case LW_BLKD_DISKCACHE_RAMFLUSH:                                    /*  �����д                    */
        iError = ERROR_NONE;
        __diskCacheFlushInvalidate2(pdiskcDiskCache, 
                                    (INT)lArg,
                                    LW_TRUE, LW_FALSE);
        break;
        
    case FIOSYNC:                                                       /*  ͬ������                    */
    case FIODATASYNC:
    case FIOFLUSH:                                                      /*  ȫ����д                    */
        iError = ERROR_NONE;
        __diskCacheFlushInvalidate(pdiskcDiskCache,
                                   0, (ULONG)PX_ERROR,
                                   LW_TRUE, LW_FALSE);
        break;
    
    case FIOTRIM:
    case FIOSYNCMETA:                                                   /*  TRIM, SYNCMETA ��Ҫ��д���� */
        iError    = ERROR_NONE;
        pblkrange = (PLW_BLK_RANGE)lArg;
        __diskCacheFlushInvalidate(pdiskcDiskCache,
                                   pblkrange->BLKR_ulStartSector, 
                                   pblkrange->BLKR_ulEndSector,
                                   LW_TRUE, LW_FALSE);                  /*  ��дָ����Χ������          */
        break;
        
    case FIODISKCHANGE:                                                 /*  ���̷����ı�                */
        pdiskcDiskCache->DISKC_blkdCache.BLKD_bDiskChange = LW_TRUE;
    case FIOCANCEL:                                                     /*  ֹͣ CACHE, ��λ�ڴ�,����д */
    case FIOUNMOUNT:                                                    /*  ж�ؾ�                      */
        iError = ERROR_NONE;
        __diskCacheMemReset(pdiskcDiskCache);                           /*  ���³�ʼ�� CACHE �ڴ�       */
        break;
    }
    
    __LW_DISKCACHE_UNLOCK(pdiskcDiskCache);                             /*  ����                        */
    
    if (iError == ERROR_NONE) {
        __LW_DISKCACHE_DISK_IOCTL(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk, iCmd, lArg);
    
    } else {
        iError = __LW_DISKCACHE_DISK_IOCTL(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk, iCmd, lArg);
    }
    
    if (iCmd == FIODISKINIT) {                                          /*  ��Ҫȷ�����̵����һ������  */
        ULONG           ulNDiskSector = (ULONG)PX_ERROR;
        PLW_BLK_DEV     pblkdDisk     = pdiskcDiskCache->DISKC_pblkdDisk;
        
        if (pblkdDisk->BLKD_ulNSector) {
            ulNDiskSector = pblkdDisk->BLKD_ulNSector;
        
        } else {
            pblkdDisk->BLKD_pfuncBlkIoctl(pblkdDisk, 
                                          LW_BLKD_GET_SECNUM, 
                                          (LONG)&ulNDiskSector);
        }
        pdiskcDiskCache->DISKC_ulEndStector = ulNDiskSector - 1;        /*  ������һ�������ı��      */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __diskCacheReset
** ��������: ͨ������ CACHE ��λһ������(��ʼ��)
** �䡡��  : pdiskcDiskCache   ���� CACHE ���ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheReset (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    __diskCacheIoctl(pdiskcDiskCache, FIOSYNC, 0);                      /*  CACHE ��д����              */

    return  (__LW_DISKCACHE_DISK_RESET(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk));
}
/*********************************************************************************************************
** ��������: __diskCacheStatusChk
** ��������: ͨ������ CACHE ���һ������
** �䡡��  : pdiskcDiskCache   ���� CACHE ���ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __diskCacheStatusChk (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    return  (__LW_DISKCACHE_DISK_STATUS(pdiskcDiskCache)(pdiskcDiskCache->DISKC_pblkdDisk));
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
