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
** �ļ���������: 2008 �� 10 �� 10 ��
**
** ��        ��: ���̸��ٻ�������. 
*********************************************************************************************************/

#ifndef __DISKCACHELIB_H
#define __DISKCACHELIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
extern LW_OBJECT_HANDLE     _G_ulDiskCacheListLock;                     /*  ������                      */
extern PLW_LIST_LINE        _G_plineDiskCacheHeader;                    /*  ����ͷ                      */
/*********************************************************************************************************
  DISK CACHE NODE LOCK
*********************************************************************************************************/
#define __LW_DISKCACHE_LOCK(pdiskc) do {                                                    \
            API_SemaphoreMPend(pdiskc->DISKC_hDiskCacheLock, LW_OPTION_WAIT_INFINITE);      \
        } while (0)
#define __LW_DISKCACHE_UNLOCK(pdiskc)   do {                                                \
            API_SemaphoreMPost(pdiskc->DISKC_hDiskCacheLock);                               \
        } while (0)
/*********************************************************************************************************
  DISK CACHE NODE OP
*********************************************************************************************************/
#define __LW_DISKCACHE_NODE_READ                0                       /*  �ڵ��                      */
#define __LW_DISKCACHE_NODE_WRITE               1                       /*  �ڵ�д                      */
/*********************************************************************************************************
  DISK CACHE NODE MACRO
*********************************************************************************************************/
#define __LW_DISKCACHE_IS_VALID(pdiskn)         ((pdiskn)->DISKN_iStatus & 0x01)
#define __LW_DISKCACHE_IS_DIRTY(pdiskn)         ((pdiskn)->DISKN_iStatus & 0x02)

#define __LW_DISKCACHE_SET_VALID(pdiskn)        ((pdiskn)->DISKN_iStatus |= 0x01)
#define __LW_DISKCACHE_SET_DIRTY(pdiskn)        ((pdiskn)->DISKN_iStatus |= 0x02)

#define __LW_DISKCACHE_CLR_VALID(pdiskn)        ((pdiskn)->DISKN_iStatus &= ~0x01)
#define __LW_DISKCACHE_CLR_DIRTY(pdiskn)        ((pdiskn)->DISKN_iStatus &= ~0x02)
/*********************************************************************************************************
  DISK CACHE NODE
*********************************************************************************************************/
typedef struct {
    LW_LIST_RING            DISKN_ringLru;                              /*  LRU ��ڵ�                  */
    LW_LIST_LINE            DISKN_lineHash;                             /*  HASH ��ڵ�                 */
    
    ULONG                   DISKN_ulSectorNo;                           /*  �����������                */
    INT                     DISKN_iStatus;                              /*  �ڵ�״̬                    */
    caddr_t                 DISKN_pcData;                               /*  �������ݻ���                */
} LW_DISKCACHE_NODE;
typedef LW_DISKCACHE_NODE  *PLW_DISKCACHE_NODE;
/*********************************************************************************************************
  DISK CACHE NODE
*********************************************************************************************************/
typedef struct {
    LW_BLK_DEV              DISKC_blkdCache;                            /*  DISK CACHE �� BLK IO ���ƿ� */
    PLW_BLK_DEV             DISKC_pblkdDisk;                            /*  ������ BLK IO ���ƿ��ַ    */
    LW_LIST_LINE            DISKC_lineManage;                           /*  �����̹߳�������            */
    
    LW_OBJECT_HANDLE        DISKC_hDiskCacheLock;                       /*  DISK CACHE ������           */
    INT                     DISKC_iCacheOpt;                            /*  CACHE ����ѡ��              */
    
    ULONG                   DISKC_ulEndStector;                         /*  ���һ�������ı��          */
    ULONG                   DISKC_ulDirtyCounter;                       /*  ��Ҫ��д����������          */
    ULONG                   DISKC_ulBytesPerSector;                     /*  ÿһ�����ֽ�����            */
    
    INT                     DISKC_iMaxRBurstSector;                     /*  ���⧷���д��������        */
    INT                     DISKC_iMaxWBurstSector;
    caddr_t                 DISKC_pcBurstBuffer;                        /*  ⧷���д������              */
    
    PLW_LIST_RING           DISKC_pringLruHeader;                       /*  LRU ��ͷ                    */
    PLW_LIST_LINE          *DISKC_pplineHash;                           /*  HASH ���                   */
    INT                     DISKC_iHashSize;                            /*  HASH ���С                 */
    
    ULONG                   DISKC_ulNCacheNode;                         /*  CACHE ����Ľڵ���          */
    caddr_t                 DISKC_pcCacheNodeMem;                       /*  CACHE �ڵ������׵�ַ        */
    caddr_t                 DISKC_pcCacheMem;                           /*  CACHE ������                */
    PLW_DISKCACHE_NODE      DISKC_disknLuck;                            /*  ���������ڵ�                */
} LW_DISKCACHE_CB;
typedef LW_DISKCACHE_CB    *PLW_DISKCACHE_CB;
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
PVOID   __diskCacheThread(PVOID  pvArg);
VOID    __diskCacheListAdd(PLW_DISKCACHE_CB   pdiskcDiskCache);
VOID    __diskCacheListDel(PLW_DISKCACHE_CB   pdiskcDiskCache);
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_DISKCACHE_EN > 0     */
#endif                                                                  /*  __DISKCACHELIB_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
