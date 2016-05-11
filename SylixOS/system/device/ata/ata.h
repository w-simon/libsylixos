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
** ��   ��   ��: ata.h
**
** ��   ��   ��: Hou.XiaoLong (��С��)
**
** �ļ���������: 2010 �� 02 �� 23 ��
**
** ��        ��: ATA �豸����ͷ�ļ�.
*********************************************************************************************************/

#ifndef __ATA_H
#define __ATA_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_ATA_EN > 0)
/*********************************************************************************************************
  ����������װ�ص�����ʱ������
*********************************************************************************************************/
#define ATA_CALLBACK_WRITE_DATA             1                           /*  ��װд����ʱ�Ļص�          */
#define ATA_CALLBACK_CHECK_DEV              2                           /*  ��װ����豸�Ƿ����ʱ�Ļص�*/
/*********************************************************************************************************
  �û����ñ�־:����ģʽ, ���νṹ
*********************************************************************************************************/
#define ATA_PIO_DEF_0                       0x00                        /*  PIOĬ��ģʽ                 */
#define ATA_PIO_DEF_1                       0x01                        /*  PIOĬ��ģʽ, û��IORDY      */
#define ATA_PIO_0                           0x08                        /*  PIO mode 0                  */
#define ATA_PIO_1                           0x09                        /*  PIO mode 1                  */
#define ATA_PIO_2                           0x0A                        /*  PIO mode 2                  */
#define ATA_PIO_3                           0x0B                        /*  PIO mode 3                  */
#define ATA_PIO_4                           0x0C                        /*  PIO mode 4                  */
#define ATA_PIO_AUTO                        0x000D                      /*  �Զ�������PIOģʽ         */
#define ATA_DMA_0                           0x0002                      /*  DMA mode 0,��δ֧��         */
#define ATA_DMA_1                           0x0003                      /*  DMA mode 1,��δ֧��         */
#define ATA_DMA_2                           0x0004                      /*  DMA mode 2,��δ֧��         */
#define ATA_DMA_AUTO                        0x0005                      /*  �Զ�������DMAģʽ         */

#define ATA_BITS_8                          0x0040                      /*  RW bits size,8  bits        */
#define ATA_BITS_16                         0x0080                      /*  RW bits size,16 bits        */
#define ATA_BITS_32                         0x00C0                      /*  RW bits size,32 bits�ݲ�֧��*/

#define ATA_PIO_SINGLE                      0x0010                      /*  RW PIO��������д            */
#define ATA_PIO_MULTI                       0x0020                      /*  RW PIO����������д          */

#define ATA_DMA_SINGLE                      0x0400                      /*  RW DMA single word,�ݲ�֧�� */
#define ATA_DMA_MULTI                       0x0800                      /*  RW DMA multi word,�ݲ�֧��  */
#define ATA_DMA_ULTRA                       0x1000                      /*  RW DMA ultra word,�ݲ�֧��  */

#define ATA_GEO_FORCE                       0x0100                      /*  set geometry in the table   */
#define ATA_GEO_PHYSICAL                    0x0200                      /*  set physical geometry       */
#define ATA_GEO_CURRENT                     0x0300                      /*  set current geometry        */
/*********************************************************************************************************
  ATA ���������ṹ
*********************************************************************************************************/
#ifdef  __cplusplus
typedef INT     (*ATA_CALLBACK)(...);
#else
typedef INT     (*ATA_CALLBACK)();
#endif

typedef struct __ata_drv_funcs              ATA_DRV_FUNCS;

typedef struct __ata_chan {
    ATA_DRV_FUNCS    *pDrvFuncs;
} ATA_CHAN;                                                             /*  ATA �����ṹ��              */

struct __ata_drv_funcs {
    INT               (*ioctl)
                      (
                      ATA_CHAN  *patachan,
                      INT        iCmd,
                      PVOID      pvArg
                      );

    INT               (*ioOutByte)
                      (
                      ATA_CHAN  *patachan,
                      ULONG      ulIoAddr,
                      UINT       uiData
                      );

    INT               (*ioInByte)
                      (
                      ATA_CHAN  *patachan,
                      ULONG      ulIoAddr
                      );

    INT               (*ioOutWordString)
                      (
                      ATA_CHAN  *patachan,
                      ULONG      ulIoAddr,
                      INT16     *psBuff,
                      INT        iWord
                      );

    INT               (*ioInWordString)
                      (
                      ATA_CHAN  *patachan,
                      ULONG      ulIoAddr,
                      INT16     *psBuff,
                      INT        iWord
                      );

    INT               (*sysReset)
                      (
                      ATA_CHAN  *patachan,
                      INT        iDrive
                      );

    INT               (*callbackInstall)
                      (
                      ATA_CHAN     *patachan,
                      INT           iCallbackType,
                      ATA_CALLBACK  callback,
                      PVOID         pvCallbackArg
                      );
};
/*********************************************************************************************************
  ATA �������Ĵ�����ַ��ʾ
*********************************************************************************************************/
typedef struct ata_reg {
    addr_t      ATAREG_ulData;                                          /*  (RW) data register (16 bits)*/
    addr_t      ATAREG_ulError;                                         /*  (R)  error register         */
    addr_t      ATAREG_ulFeature;                                       /*  (W) feature or write-precomp*/
    addr_t      ATAREG_ulSeccnt;                                        /*  (RW) sector count           */
    addr_t      ATAREG_ulSector;                                        /*  (RW) first sector number    */
    addr_t      ATAREG_ulCylLo;                                         /*  (RW) cylinder low byte      */
    addr_t      ATAREG_ulCylHi;                                         /*  (RW) cylinder high byte     */
    addr_t      ATAREG_ulSdh;                                           /*  (RW) sector size/drive/head */
    addr_t      ATAREG_ulCommand;                                       /*  (W)  command register       */
    addr_t      ATAREG_ulStatus;                                        /*  (R)  immediate status       */
    addr_t      ATAREG_ulAStatus;                                       /*  (R)  alternate status       */
    addr_t      ATAREG_ulDControl;                                      /*  (W)  disk controller control*/
} ATA_REG;
/*********************************************************************************************************
  ATA �豸ͨ������
*********************************************************************************************************/
typedef struct atachan_paparm {
    INT         ATACP_iCtrlNum;                                         /*  ��������                    */
    INT         ATACP_iDrives;                                          /*  �豸����                    */
    INT         ATACP_iBytesPerSector;                                  /*  ÿ�����ֽ���                */
    INT         ATACP_iConfigType;                                      /*  ���ñ�־                    */
    BOOL        ATACP_bIntEnable;                                       /*  ϵͳ�ж�ʹ�ܱ�־            */
    BOOL        ATACP_bPreadBeSwap;                                     /*  ��� CPU Pread �Ƿ���Ҫ��ת */
    ULONG       ATACP_ulSyncSemTimeout;                                 /*  ͬ���ȴ���ʱʱ��(ϵͳʱ��)  */
    ATA_REG     ATACP_atareg;                                           /*  ATA�����ļ��Ĵ���           */
} ATA_CHAN_PARAM;
typedef ATA_CHAN_PARAM *PATA_CHAN_PARAM;
/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT            API_AtaDrv(ATA_CHAN  *patachan, ATA_CHAN_PARAM *patacp);
LW_API PLW_BLK_DEV    API_AtaDevCreate(INT iCtrl,
                                       INT iDrive,
                                       INT iNBlocks,
                                       INT iBlkOffset);
LW_API INT            API_AtaDevDelete(PLW_BLK_DEV pblkdev);

#define ataDrv        API_AtaDrv
#define ataDevCreate  API_AtaDevCreate
#define ataDevDelete  API_AtaDevDelete

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
#endif                                                                  /*  __ATA_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
