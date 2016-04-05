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
** ��   ��   ��: diskCacheThread.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 23 ��
**
** ��        ��: ���̸��ٻ��������������д����.
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
  ��������
*********************************************************************************************************/
#define __LW_DISKCACHE_BG_SECONDS       2                               /*  ��д����                    */
#define __LW_DISKCACHE_BG_MINSECTOR     64                              /*  ÿ�����ٻ�д��������        */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
LW_OBJECT_HANDLE        _G_ulDiskCacheListLock  = 0ul;                  /*  ������                      */
PLW_LIST_LINE           _G_plineDiskCacheHeader = LW_NULL;              /*  ����ͷ                      */
static PLW_LIST_LINE    _G_plineDiskCacheOp     = LW_NULL;              /*  ��������ڵ�                */
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define __LW_DISKCACHE_LIST_LOCK()      \
        API_SemaphoreMPend(_G_ulDiskCacheListLock, LW_OPTION_WAIT_INFINITE)
#define __LW_DISKCACHE_LIST_UNLOCK()    \
        API_SemaphoreMPost(_G_ulDiskCacheListLock)
/*********************************************************************************************************
** ��������: __diskCacheThread
** ��������: ���̸��ٻ��������������д����
** �䡡��  : pvArg             ��������
** �䡡��  : NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  __diskCacheThread (PVOID  pvArg)
{
    PLW_DISKCACHE_CB   pdiskcDiskCache;
    ULONG              ulNSector;
    
    (VOID)pvArg;
    
    for (;;) {
        API_TimeSSleep(__LW_DISKCACHE_BG_SECONDS);                      /*  ������ʱ                    */
        
        __LW_DISKCACHE_LIST_LOCK();
        _G_plineDiskCacheOp = _G_plineDiskCacheHeader;
        while (_G_plineDiskCacheOp) {
            pdiskcDiskCache = _LIST_ENTRY(_G_plineDiskCacheOp, 
                                          LW_DISKCACHE_CB, 
                                          DISKC_lineManage);
            _G_plineDiskCacheOp = _list_line_get_next(_G_plineDiskCacheOp);
            
            __LW_DISKCACHE_LIST_UNLOCK();                               /*  �ͷ��������ʹ��Ȩ          */
            
            ulNSector = __MAX(__LW_DISKCACHE_BG_MINSECTOR, 
                              (pdiskcDiskCache->DISKC_ulDirtyCounter / 2));
                                                                        /*  �����д���̵�������        */
            pdiskcDiskCache->DISKC_blkdCache.BLKD_pfuncBlkIoctl(pdiskcDiskCache, 
                LW_BLKD_DISKCACHE_RAMFLUSH, (LONG)ulNSector);           /*  ��д����                    */
            
            __LW_DISKCACHE_LIST_LOCK();                                 /*  ��ȡ�������ʹ��Ȩ          */
        }
        __LW_DISKCACHE_LIST_UNLOCK();
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __diskCacheListAdd
** ��������: �� DISK CACHE ������д��
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __diskCacheListAdd (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    __LW_DISKCACHE_LIST_LOCK();
    _List_Line_Add_Ahead(&pdiskcDiskCache->DISKC_lineManage,
                         &_G_plineDiskCacheHeader);
    __LW_DISKCACHE_LIST_UNLOCK();
}
/*********************************************************************************************************
** ��������: __diskCacheListDel
** ��������: �ӻ�д���н� DISK CACHE �Ƴ�
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __diskCacheListDel (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    __LW_DISKCACHE_LIST_LOCK();
    if (&pdiskcDiskCache->DISKC_lineManage == _G_plineDiskCacheOp) {
        _G_plineDiskCacheOp = _list_line_get_next(_G_plineDiskCacheOp);
    }
    _List_Line_Del(&pdiskcDiskCache->DISKC_lineManage,
                   &_G_plineDiskCacheHeader);
    __LW_DISKCACHE_LIST_UNLOCK();
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
