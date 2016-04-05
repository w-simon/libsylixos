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
** ��   ��   ��: diskCache.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 11 �� 26 ��
**
** ��        ��: ���̸��ٻ��������.
*********************************************************************************************************/

#ifndef __DISKCACHE_H
#define __DISKCACHE_H

/*********************************************************************************************************
  ע��:
  
  ���ʹ�� CACHE, �뱣֤�������豸��һһ��Ӧ, (����߼�����������һ�� CACHE)
  
  DISK CACHE ��ʹ��:

  ʹ�� DISK CACHE �����Ŀ�ľ���������ٿ��豸 IO �ķ���Ч��, ����ϵͳ�ڴ���Ϊ���ݵĻ���, 
  ʹ�� DISK CACHE �������������ڴ��������뻺�����ݵĲ�ͬ����, ͬ�����ݿ��Ե��� ioctl(..., FIOFLUSH, ...)
  ʵ��.
  
  pblkDev = xxxBlkDevCreate(...);
  diskCacheCreate(pblkDev, ..., &pCacheBlk);
  ...(������ڶ���̷���, ���: diskPartition.h)
  fatFsDevCreate(pVolName, pCacheBlk);
  
  ...�����豸
  
  umount(pVolName);
  ...(������ڶ���̷���, ���: diskPartition.h)
  diskCacheDelete(pCacheBlk);
  xxxBlkDevDelete(pblkDev);
  
  �Ƽ�ʹ�� oem ���̲�����.
*********************************************************************************************************/

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)

/*********************************************************************************************************
  ioctl ��������
*********************************************************************************************************/

#define LW_BLKD_DISKCACHE_GET_OPT       LW_OSIOR('b', 150, INT)         /*  ��ȡ CACHE ѡ��             */
#define LW_BLKD_DISKCACHE_SET_OPT       LW_OSIOD('b', 151, INT)         /*  ���� CACHE ѡ��             */
#define LW_BLKD_DISKCACHE_INVALID       LW_OSIO( 'b', 152)              /*  ʹ CACHE ��д��ȫ��������   */
#define LW_BLKD_DISKCACHE_RAMFLUSH      LW_OSIOD('b', 153, ULONG)       /*  �����дһЩ������          */

/*********************************************************************************************************
  ����������
  
  Ӳ������ DMA ������������· DISK CACHE, ��Ϊ DISK CACHE ��� CPU CACHE ӵ�����Ӳ����������.
*********************************************************************************************************/

#define LW_DISKCACHE_OPT_DISABLE        0x0                             /*  ���� CACHE ��·             */
#define LW_DISKCACHE_OPT_ENABLE         0x1                             /*  ���� CACHE ��Ч             */

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API ULONG  API_DiskCacheCreate(PLW_BLK_DEV   pblkdDisk, 
                                  PVOID         pvDiskCacheMem, 
                                  size_t        stMemSize, 
                                  INT           iMaxBurstSector,
                                  PLW_BLK_DEV  *ppblkDiskCache);
                                  
LW_API ULONG  API_DiskCacheCreateEx(PLW_BLK_DEV   pblkdDisk, 
                                    PVOID         pvDiskCacheMem, 
                                    size_t        stMemSize, 
                                    INT           iMaxRBurstSector,
                                    INT           iMaxWBurstSector,
                                    PLW_BLK_DEV  *ppblkDiskCache);
                                  
LW_API INT    API_DiskCacheDelete(PLW_BLK_DEV   pblkdDiskCache);

LW_API INT    API_DiskCacheSync(PLW_BLK_DEV   pblkdDiskCache);

#define diskCacheCreate     API_DiskCacheCreate
#define diskCacheCreateEx   API_DiskCacheCreateEx
#define diskCacheDelete     API_DiskCacheDelete
#define diskCacheSync       API_DiskCacheSync

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
#endif                                                                  /*  __DISKCACHE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
