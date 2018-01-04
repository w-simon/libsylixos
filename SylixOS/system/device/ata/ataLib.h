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
** ��   ��   ��: ataLib.h
**
** ��   ��   ��: Hou.XiaoLong (��С��)
**
** �ļ���������: 2010 �� 02 �� 01 ��
**
** ��        ��: ATA �豸�����ͷ�ļ�.

** BUG:
2010.05.13  �޸�__ATA_TIMEOUT���ֵ.
*********************************************************************************************************/

#ifndef __ATALIB_H
#define __ATALIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_ATA_EN > 0)

#include "ata.h"
/*********************************************************************************************************
  ATA DEBUG
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG
#define ATA_DEBUG_MSG(msg)                  printk msg
#else
#define ATA_DEBUG_MSG(msg)
#endif                                                                  /*  __SYLIXOS_DEBUG             */

#define __ATA_DELAYMS(ms)                                   \
        do {                                                \
            ULONG   ulTimeout = LW_MSECOND_TO_TICK_1(ms);   \
            API_TimeSleep(ulTimeout);                       \
        } while (0)
/*********************************************************************************************************
  �����ļ��Ĵ���
*********************************************************************************************************/
#define __ATA_DATA(patactrl)        (patactrl)->ATACTRL_ataReg.ATAREG_ulData
#define __ATA_ERROR(patactrl)       (patactrl)->ATACTRL_ataReg.ATAREG_ulError
#define __ATA_FEATURE(patactrl)     (patactrl)->ATACTRL_ataReg.ATAREG_ulFeature
#define __ATA_SECCNT(patactrl)      (patactrl)->ATACTRL_ataReg.ATAREG_ulSeccnt
#define __ATA_SECTOR(patactrl)      (patactrl)->ATACTRL_ataReg.ATAREG_ulSector
#define __ATA_CYLLO(patactrl)       (patactrl)->ATACTRL_ataReg.ATAREG_ulCylLo
#define __ATA_CYLHI(patactrl)       (patactrl)->ATACTRL_ataReg.ATAREG_ulCylHi
#define __ATA_SDH(patactrl)         (patactrl)->ATACTRL_ataReg.ATAREG_ulSdh
#define __ATA_COMMAND(patactrl)     (patactrl)->ATACTRL_ataReg.ATAREG_ulCommand
#define __ATA_STATUS(patactrl)      (patactrl)->ATACTRL_ataReg.ATAREG_ulStatus
#define __ATA_ASTATUS(patactrl)     (patactrl)->ATACTRL_ataReg.ATAREG_ulAStatus
#define __ATA_DCONTROL(patactrl)    (patactrl)->ATACTRL_ataReg.ATAREG_ulDControl
/*********************************************************************************************************
  ������������
*********************************************************************************************************/
#define __ATA_RETRY_TIMES                   3                           /*  �豸���Դ���                */
#define __ATA_MAX_DRIVES                    2                           /*  �����豸��,���ʹ��豸       */
#define __ATA_MAX_RW_SECTORS                256                         /*  ������ȡ�����������        */

#define __ATA_TIMEOUT_LOOP                  0x5FFFFFE                   /*  ��ʱ��Ӧ�豸                */
#define __ATA_TIMEOUT_SEC                   3                           /*  ���ʱʱ��                */

#define __ATA_IDE_LOCAL                     0                           /*  ctrl type: LOCAL(IDE)       */
#define __ATA_PCMCIA                        1                           /*  ctrl type: PCMCIA           */

#define __ATA_SEM_TIMEOUT_DEF               30000

#define __ATA_MAX_28LBA                     0x0fffffff
#define __ATA_MAX_48LBA                     (0xffffffff | ((UINT64)0xffff << 32))
/*********************************************************************************************************
  ����Ϊ���������岿�ݣ��û�����Ķ�
*********************************************************************************************************/
/*********************************************************************************************************
  �豸����
*********************************************************************************************************/
#define __ATA_TYPE_NONE                     0x00                        /*  �豸������                  */
#define __ATA_TYPE_ATA                      0x01                        /*  ATA�豸                     */
#define __ATA_TYPE_ATAPI                    0x02                        /*  ATAPI�豸                   */
#define __ATA_TYPE_INIT                     255                         /*  �豸����ȷ��                */
/*********************************************************************************************************
  �豸״̬
*********************************************************************************************************/
#define __ATA_DEV_OK                        0                           /*  �豸OK                      */
#define __ATA_DEV_NONE                      1                           /*  �豸�����ڻ��豸û����Ӧ    */
#define __ATA_DEV_DIAG_F                    2                           /*  �豸���ʧ��                */
#define __ATA_DEV_PREAD_F                   3                           /*  ��ȡ�豸����ʧ��            */
#define __ATA_DEV_MED_CH                    4                           /*  ý���Ѿ��ı�                */
#define __ATA_DEV_NO_BLKDEV                 5                           /*  û�п��õĿ��豸            */
#define __ATA_DEV_INIT                      255                         /*  δ��ʼ���豸                */
/*********************************************************************************************************
  ATA ָ�����(����)
*********************************************************************************************************/
#define __ATA_CMD_RECALIB                   0x10                        /*  У׼                        */
#define __ATA_CMD_READ                      0x20                        /*  ����������                  */
#define __ATA_CMD_WRITE                     0x30                        /*  д��������                  */
#define __ATA_CMD_FORMAT                    0x50                        /*  ��ʽ�켣                    */
#define __ATA_CMD_SEEK                      0x70                        /*  ̽��                        */
#define __ATA_CMD_DIAGNOSE                  0x90                        /*  ִ�п��������              */
#define __ATA_CMD_INITP                     0x91                        /*  ��ʼ����������              */
#define __ATA_CMD_READ_MULTI                0xC4                        /*  ���ض�                      */
#define __ATA_CMD_WRITE_MULTI               0xC5                        /*  ����д                      */
#define __ATA_CMD_SET_MULTI                 0xC6                        /*  ���ö���ģʽ                */
#define __ATA_CMD_STANDBY_IMMEDIATE         0xE0                        /*  ��������                    */
#define __ATA_CMD_IDLE_IMMEDIATE            0xE1                        /*  ��������                    */
#define __ATA_CMD_READP                     0xEC                        /*  ȷ��                        */
#define __ATA_CMD_SET_FEATURE               0xEF                        /*  ��������                    */

#define __ATA_CMD_READ_DMA                  0xC8                        /*  with retries                */
#define __ATA_CMD_READ_DMA_NO               0xC9                        /*  without retries             */
#define __ATA_CMD_WRITE_DMA                 0xCA                        /*  with retries                */
#define __ATA_CMD_WRITE_DMA_NO              0xCB                        /*  without retries             */
/*********************************************************************************************************
  ATAPI ָ�����
*********************************************************************************************************/
#define __ATA_PI_CMD_SRST                   0x08                        /*  ATAPI�����λ               */
#define __ATA_PI_CMD_PKTCMD                 0xA0                        /*  ATAPI Pakcet Command        */
#define __ATA_PI_CMD_IDENTD                 0xA1                        /*  ATAPIȷ���豸               */
#define __ATA_PI_CMD_SERVICE                0xA2                        /*  Service                     */
/*********************************************************************************************************
  CDROM ָ�����
*********************************************************************************************************/
#define __CDROM_CMD_TEST_UNIT_READY         0x00                        /*  CDROM Test Unit Ready       */
#define __CDROM_CMD_REQUEST_SENSE           0x03                        /*  CDROM Request Sense         */
#define __CDROM_CMD_INQUIRY                 0x12                        /*  CDROM Inquiry               */
#define __CDROM_CMD_READ_CDROM_CAP          0x25                        /*  CDROM Read CD-ROM Capacity  */
#define __CDROM_CMD_READ_12                 0xA8                        /*  CDROM Read (12)             */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define __ATA_SUB_ENABLE_8BIT               0x01                        /*  ʹ��8λ���ݴ���             */
#define __ATA_SUB_ENABLE_WCACHE             0x02                        /*  ʹ��д����                  */
#define __ATA_SUB_SET_RWMODE                0x03                        /*  ����PIOģʽ                 */
#define __ATA_SUB_ENABLE_APM                0x05                        /*  ʹ�ܸ߼���Դ����            */
#define __ATA_SUB_DISABLE_RETRY             0x33                        /*  ��ֹ����                    */
#define __ATA_SUB_SET_LENGTH                0x44                        /*  ���������ֽڳ���            */
#define __ATA_SUB_SET_CACHE                 0x54                        /*  ����CACHE��                 */
#define __ATA_SUB_DISABLE_LOOK              0x55                        /*  ��ֹǰ��������              */
#define __ATA_SUB_DISABLE_REVE              0x66                        /*  ��ֹ�ϵ縴λ                */
#define __ATA_SUB_DISABLE_ECC               0x77                        /*  ��ֹECC                     */
#define __ATA_SUB_DISABLE_8BIT              0x81                        /*  ʹ��16λ���ݴ���            */
#define __ATA_SUB_DISABLE_WCACHE            0x82                        /*  ��ֹд����                  */
#define __ATA_SUB_DISABLE_APM               0x85                        /*  ��ֹ�߼���Դ����            */
#define __ATA_SUB_ENABLE_ECC                0x88                        /*  ʹ��ECC                     */
#define __ATA_SUB_ENABLE_RETRY              0x99                        /*  ʹ������                    */
#define __ATA_SUB_ENABLE_LOOK               0xAA                        /*  ʹ��Ԥ������                */
#define __ATA_SUB_SET_PREFETCH              0xAB                        /*  ����Ԥȡ������              */
#define __ATA_SUB_SET_4BYTES                0xBB                        /*  4�ֽڲ����ĳ���             */
#define __ATA_SUB_ENABLE_REVE               0xCC                        /*  ʹ���ϵ縴λ                */
/*********************************************************************************************************
  ״̬�Ĵ������λ
*********************************************************************************************************/
#define __ATA_STAT_ERR                      0x01                        /*  �豸�����˴���              */
#define __ATA_STAT_INDEX                    0x02
#define __ATA_STAT_ECCCOR                   0x04
#define __ATA_STAT_DRQ                      0x08                        /*  �����ݴ�������              */
#define __ATA_STAT_SEEKCMPLT                0x10                        /*  �������                    */
#define __ATA_STAT_WRTFLT                   0x20                        /*  д����                      */
#define __ATA_STAT_READY                    0x40                        /*  �豸׼����                  */
#define __ATA_STAT_BUSY                     0x80                        /*  �豸æ                      */
#define __ATA_STAT_IDLE                     0x88                        /*  �豸����                    */
/*********************************************************************************************************
  �豸��ͷ�Ĵ���
*********************************************************************************************************/
#define __ATA_DevReg_DEV0                   0x00                        /*  �豸Ϊ����                  */
#define __ATA_DevReg_DEV1                   0x10                        /*  �豸Ϊ����                  */

#define __ATA_SDH_CHS                       0xa0                        /*  chs, 512�ֽ�ÿ����          */
#define __ATA_SDH_LBA	                    0xe0                        /*  lba, 512�ֽ�ÿ����          */

#define __ATA_DRIVE_BIT                     4                           /*  �����豸λ                  */
/*********************************************************************************************************
  ���ƼĴ���
*********************************************************************************************************/
#define __ATA_CTL_RST                       0x04                        /*  �����λ��Ч                */
#define __ATA_CTL_IEN                       0x00                        /*  ʹ���ж�                    */
#define __ATA_CTL_IDS                       0x02                        /*  �����ж�                    */
/*********************************************************************************************************
  ���������16λ�����������,��8λ��ATA����Ĵ���һ��,��8λ�Զ���
*********************************************************************************************************/
#define __ATA_EER_REG_AMNF                  0x01                        /*  һ�����                    */
#define __ATA_EER_REG_ABRT                  0x04                        /*  ָ����Ч����                */
#define __ATA_EER_REG_IDNF                  0x10                        /*  Ѱ������ַ����              */
#define __ATA_EER_REG_UNC                   0x40                        /*  �����˲��ɾ����Ĵ���        */
#define __ATA_EER_REG_BBK                   0x80                        /*  ���ִ����                  */
/*********************************************************************************************************
  �Զ���
*********************************************************************************************************/
#define __ATA_MASTER_DEV                    0                           /*  ���豸                      */
#define __ATA_DEVICE_DEV                    1                           /*  ���豸                      */
#define __ATA_WHOLE_BLOCK                   0                           /*  ʹ�������豸�Ĵ洢�ռ�      */
#define __ATA_BLOCK_OFFSET                  0                           /*  �豸��ƫ����                */
/*********************************************************************************************************
  ATA����ģʽ
*********************************************************************************************************/
#define __ATA_PIO_DEF_W                     0x00                        /*  PIO default trans. mode     */
#define __ATA_PIO_DEF_WO                    0x01                        /*  PIO default trans. mode,
                                                                            no IORDY                    */
#define __ATA_PIO_W_0                       0x08                        /*  PIO flow trans. mode 0      */
#define __ATA_PIO_W_1                       0x09                        /*  PIO flow trans. mode 1      */
#define __ATA_PIO_W_2                       0x0A                        /*  PIO flow trans. mode 2      */
#define __ATA_PIO_W_3                       0x0B                        /*  PIO flow trans. mode 3      */
#define __ATA_PIO_W_4                       0x0C                        /*  PIO flow trans. mode 4      */

#define __ATA_SET_PIO_MODE_0                0x0
#define __ATA_SET_PIO_MODE_1                0x1
#define __ATA_SET_PIO_MODE_2                0x2
#define __ATA_SET_PIO_MODE_3                0x3
#define __ATA_SET_PIO_MODE_4                0x4

#define __ATA_DMA_SINGLE_0                  0x10                        /*  singleword DMA mode 0       */
#define __ATA_DMA_SINGLE_1                  0x11                        /*  singleword DMA mode 1       */
#define __ATA_DMA_SINGLE_2                  0x12                        /*  singleword DMA mode 2       */
#define __ATA_DMA_MULTI_0                   0x20                        /*  multiword DMA mode 0        */
#define __ATA_DMA_MULTI_1                   0x21                        /*  multiword DMA mode 1        */
#define __ATA_DMA_MULTI_2                   0x22                        /*  multiword DMA mode 2        */
#define __ATA_DMA_ULTRA_0                   0x40                        /*  ultraword DMA mode 0        */
#define __ATA_DMA_ULTRA_1                   0x41                        /*  ultraword DMA mode 1        */
#define __ATA_DMA_ULTRA_2                   0x42                        /*  ultraword DMA mode 2        */

#define __ATA_SET_SDMA_MODE_0               0x0
#define __ATA_SET_SDMA_MODE_1               0x1
#define __ATA_SET_SDMA_MODE_2               0x2

#define __ATA_SET_MDMA_MODE_0               0x0
#define __ATA_SET_MDMA_MODE_1               0x1
#define __ATA_SET_MDMA_MODE_2               0x2

#define __ATA_SET_UDMA_MODE_0               0x0
#define __ATA_SET_UDMA_MODE_1               0x1
#define __ATA_SET_UDMA_MODE_2               0x2
#define __ATA_SET_UDMA_MODE_3               0x3
#define __ATA_SET_UDMA_MODE_4               0x4
#define __ATA_SET_UDMA_MODE_5               0x5
/*********************************************************************************************************
  ����λ
*********************************************************************************************************/
#define __ATA_INTER_DMA_MASK                0x8000
#define __ATA_CMD_QUE_MASK                  0x4000
#define __ATA_OVERLAP_MASK                  0x2000
#define __ATA_IORDY_MASK                    0x0800
#define __ATA_IOLBA_MASK                    0x0200
#define __ATA_DMA_CAP_MASK                  0x0100

#define __ATA_BIT_MASK0                     0x0001
#define __ATA_BIT_MASK1                     0x0002
#define __ATA_BIT_MASK2                     0x0004
#define __ATA_BIT_MASK3                     0x0008
#define __ATA_BIT_MASK4                     0x0010
#define __ATA_BIT_MASK5                     0x0020
#define __ATA_BIT_MASK6                     0x0040
#define __ATA_BIT_MASK7                     0x0080
#define __ATA_BIT_MASK8                     0x0100
#define __ATA_BIT_MASK9                     0x0200
#define __ATA_BIT_MASK10                    0x0400
#define __ATA_BIT_MASK11                    0x0800
#define __ATA_BIT_MASK12                    0x1000
#define __ATA_BIT_MASK13                    0x2000
#define __ATA_BIT_MASK14                    0x4000
#define __ATA_BIT_MASK15                    0x8000

#define __ATA_MULTISEC_MASK                 0x00FF
#define __ATA_LBA_HEAD_MASK                 0x0f000000
#define __ATA_LBA_CYL_MASK                  0x00ffff00
#define __ATA_LBA_SECTOR_MASK               0x000000ff
/*********************************************************************************************************
  ���ñ�־:����ģʽ, ���νṹ
*********************************************************************************************************/
#define __ATA_MODE_MASK                     0x000F                      /*  transfer mode mask          */
#define __ATA_BITS_MASK                     0x00C0                      /*  RW bits size mask           */
#define __ATA_PIO_MASK                      0x0030                      /*  RW PIO mask                 */
#define __ATA_DMA_MASK                      0x1C00                      /*  RW DMA mask                 */
#define __ATA_GEO_MASK                      0x0300                      /*  geometry mask               */

#define __ATA_PIO_MASK_012                  0x03                        /*  PIO Mode 0,1,2 masks        */
#define __ATA_PIO_MASK_34                   0x02                        /*  PIO Mode 3,4 masks          */

#define __ATA_WORD54_58_VALID               0x01
#define __ATA_WORD64_70_VALID               0x02
#define __ATA_WORD88_VALID                  0x04

#define __ATA_PIO                           0
#define __ATA_MDMA                          1
#define __ATA_UDMA                          2

#define __ATA_SIGNATURE       			    0x01010000
#define __ATAPI_SIGNATURE     			    0x010114EB
/*********************************************************************************************************
  �豸ʶ����Ϣ(ȫ���豸��Ϣ)
*********************************************************************************************************/
typedef struct __ata_param {
    INT16  ATAPAR_sConfig;                                              /*  general configuration       */
    INT16  ATAPAR_sCylinders;                                           /*  number of cylinders         */
    INT16  ATAPAR_sRemovcyl;                                            /*  number of removable cylinders*/
    INT16  ATAPAR_sHeads;                                               /*  number of heads             */
    INT16  ATAPAR_sBytesTrack;                                          /*  number of unformatted
                                                                            bytes/track                 */
    INT16  ATAPAR_sBytesSec;                                            /*  number of unformatted
                                                                            bytes/sector                */
    INT16  ATAPAR_sSectors;                                             /*  number of sectors/track     */
    INT16  ATAPAR_sBytesGap;                                            /*  minimum bytes in intersector
                                                                            gap                         */
    INT16  ATAPAR_sBytesSync;                                           /*  minimum bytes in sync field */
    INT16  ATAPAR_sVendstat;                                            /*  number of words of vendor
                                                                            status                      */
    INT8   ATAPAR_cSerial[20];                                          /*  controller serial number    */
    INT16  ATAPAR_sType;                                                /*  controller type             */
    INT16  ATAPAR_sSize;                                                /*  sector buffer size, in
                                                                            sectors                     */
    INT16  ATAPAR_sBytesEcc;                                            /*  ecc bytes appended          */
    INT8   ATAPAR_cRev[8];                                              /*  firmware revision           */
    INT8   ATAPAR_cModel[40];                                           /*  model name                  */
    INT16  ATAPAR_sMultiSecs;                                           /*  RW multiple support.
                                                                            bits 7-0 ia max secs        */
    INT16  ATAPAR_sReserved48;                                          /*  reserved                    */
    INT16  ATAPAR_sCapabilities;                                        /*  capabilities                */
    INT16  ATAPAR_sReserved50;                                          /*  reserved                    */
    INT16  ATAPAR_sPioMode;                                             /*  PIO data transfer cycle
                                                                            timing mode                 */
    INT16  ATAPAR_sDmaMode;                                             /*  single word DMA data transfer
                                                                            cycle timing                */
    INT16  ATAPAR_sValid;                                               /*  53    field validity
                                                                            R-15-3 reserved;
                                                                            F-2= word 88 validity;
                                                                            F-1= word 64-70 validity;
                                                                            V-0= word 54-58 validity    */
    INT16  ATAPAR_sCurrentCylinders;                                    /*  number of current logical
                                                                            cylinders                   */
    INT16  ATAPAR_sCurrentHeads;                                        /*  number of current logical
                                                                            heads                       */
    INT16  ATAPAR_sCurrentSectors;                                      /*  number of current logical
                                                                            sectors / track             */
    INT16  ATAPAR_sCapacity0;                                           /*  current capacity in sectors */
    INT16  ATAPAR_sCapacity1;                                           /*  current capacity in sectors */
    INT16  ATAPAR_sMultiSet;                                            /*  multiple sector setting     */
    UINT16 ATAPAR_usSectors0;                                           /*  total number of user
                                                                            addressable sectors         */
    UINT16 ATAPAR_usSectors1;                                           /*  total number of user
                                                                            addressable sectors         */
    INT16  ATAPAR_sSingleDma;                                           /*  [62]single word DMA transfer*/
    INT16  ATAPAR_sMultiDma;                                            /*  [63]multi word DMA transfer */
    INT16  ATAPAR_sAdvancedPio;                                         /*  [64]flow control PIO transfer
                                                                            modes supported             */
    INT16  ATAPAR_sCycletimeDma;                                        /*  minimum multiword DMA
                                                                            transfer cycle time         */
    INT16  ATAPAR_sCycletimeMulti;                                      /*  recommended multiword DMA
                                                                            cycle time                  */
    INT16  ATAPAR_sCycletimePioNoIordy;                                 /*  min PIO transfer cycle time
                                                                            wo flow ctl                 */
    INT16  ATAPAR_sCycletimePioIordy;                                   /*  min PIO transfer cycle time
                                                                            w IORDY                     */
    INT16  ATAPAR_sReserved69;                                          /*  reserved                    */
    INT16  ATAPAR_sReserved70;                                          /*  reserved                    */
    /*
     *  ATAPI
     */
    INT16  ATAPAR_sPktCmdRelTime;                                       /*  [71]Typical Time for Release
                                                                            after Packet                */
    INT16  ATAPAR_sServCmdRelTime;                                      /*  [72]Typical Time for Release
                                                                            after SERVICE               */
    INT16  ATAPAR_sReserved73[2];                                       /*  reserved                    */
    INT16  ATAPAR_sQueuedepth;                                          /*  [75]4-0 Maximum queue depth1*/
    INT16  ATAPAR_sReserved76[4];                                       /*  76-79 reserved              */
    INT16  ATAPAR_sMajorversNum;                                        /*  Major version number
                                                                            0000h or FFFFh = device does
                                                                            not report version F 15 Reserved
                                                                            F 14 Reserved for ATA/ATAPI-14
                                                                            F 13 Reserved for ATA/ATAPI-13
                                                                            F 12 Reserved for ATA/ATAPI-12
                                                                            F 11 Reserved for ATA/ATAPI-11
                                                                            F 10 Reserved for ATA/ATAPI-10
                                                                            F 9 Reserved for ATA/ATAPI-9
                                                                            F 8 Reserved for ATA/ATAPI-8
                                                                            F 7 Reserved for ATA/ATAPI-7
                                                                            F 6 1 = supports ATA/ATAPI-6
                                                                            F 5 1 = supports ATA/ATAPI-5
                                                                            F 4 1 = supports ATA/ATAPI-4
                                                                            F 3 1 = supports ATA-3
                                                                            X 2 Obsolete
                                                                            X 1 Obsolete
                                                                            F 0 Reserved                */
    INT16  ATAPAR_sMinorVersNum;                                        /*  Minor version number
                                                                            0000h or FFFFh = device does
                                                                            not report version
                                                                            0001h-FFFEh = see 3.16.41   */
    INT16  ATAPAR_sCommandSetSup1;                                      /*  [82] Command set supported  */
    INT16  ATAPAR_sCommandSetSup2;
    INT16  ATAPAR_sCommandSetExt;                                       /*  Command set/feature supported
                                                                            extension                   */
    INT16  ATAPAR_sCommandSetEnable1;                                   /*  Command set/feature enabled.*/
    INT16  ATAPAR_sCommandSetEnable2;
    INT16  ATAPAR_sCommandSetDefault;                                   /*  Command set/feature default */
    INT16  ATAPAR_sUltraDmaMode;                                        /*  [88] F 15-14 Reserved
                                                                            V 13 1 = Ultra DMA mode 5 is selected
                                                                            0 = Ultra DMA mode 5 is not selected
                                                                            V 12 1 = Ultra DMA mode 4 is selected
                                                                            0 = Ultra DMA mode 4 is not selected
                                                                            V 11 1 = Ultra DMA mode 3 is selected
                                                                            0 = Ultra DMA mode 3 is not selected
                                                                            V 10 1 = Ultra DMA mode 2 is selected
                                                                            0 = Ultra DMA mode 2 is not selected
                                                                            V 9 1 = Ultra DMA mode 1 is selected
                                                                            0 = Ultra DMA mode 1 is not selected
                                                                            V 8 1 = Ultra DMA mode 0 is selected
                                                                            0 = Ultra DMA mode 0 is not selected
                                                                            F 7-6 Reserved
                                                                            F 5 1 = Ultra DMA mode 5 and below are supported
                                                                            F 4 1 = Ultra DMA mode 4 and below are supported
                                                                            F 3 1 = Ultra DMA mode 3 and below are supported
                                                                            F 2 1 = Ultra DMA mode 2 and below are supported
                                                                            F 1 1 = Ultra DMA mode 1 and below are supported
                                                                            F 0 1 = Ultra DMA mode 0 is supported*/
    INT16  ATAPAR_sSecurityEraseTime;                                   /*  Time Required for security
                                                                            erase unit completion(F)    */
    INT16  ATAPAR_sEnSecurityEraseTime;                                 /*  Time Required for enhansed
                                                                            security erase completion(F)*/
    INT16  ATAPAR_sAdvPowerManVal;                                      /*  Current adv. power mgmnt
                                                                            value (V)                   */
    INT16  ATAPAR_sMasterPassRev;                                       /*  Master password Revision
                                                                            code (V)                    */
    INT16  ATAPAR_sHardResetResult;                                     /*  Hardware Reset result       */
    INT16  ATAPAR_sAcousticMgmt;                                        /*  acoustic management         */
    INT16  ATAPAR_sReserved95[5];                                       /*  reserved                    */
    UINT16 ATAPAR_sMaxLBA[4];                                           /*  MAX LBA                     */
    INT16  ATAPAR_sReserved104[23];                                     /*  reserved                    */
    INT16  ATAPAR_sRemoveNotification;                                  /*  Removable Media Status Notification
                                                                            feature set supported       */
    INT16  ATAPAR_sSecurityStatus;                                      /*  Security status             */
    INT16  ATAPAR_sVendor[31];                                          /*  Vendor Specifc              */
    INT16  ATAPAR_sCfaPowerMode;                                        /*  CFA power mode 1            */
    INT16  ATAPAR_sReserved161[15];                                     /*  reserved                    */
    INT16  ATAPAR_sMediaSerialNum[60];                                  /*  current media serial number */
    INT16  ATAPAR_sReserved205[49];                                     /*  reserved                    */
    INT16  ATAPAR_sIntegrityWord;                                       /*  Integrity word              */
} __ATA_PARAM;
/*********************************************************************************************************
  ATA�豸��Ϣ
*********************************************************************************************************/
typedef struct __ata_dev {
    BLK_DEV     ATAD_blkdBlkDev;                                        /*  must be here                */
    INT         ATAD_iCtrl;                                             /*  ctrl no.  0 - 1             */
    INT         ATAD_iDrive;                                            /*  drive no. 0 - 1             */
    INT         ATAD_iBlkOffset;                                        /*  sector offset               */
    INT         ATAD_iNBlocks;                                          /*  number of sectors           */
    INT         ATAD_iTransCount;                                       /*  Number of transfer cycles   */
    INT         ATAD_iErrNum;                                           /*  Error description message
                                                                            number                      */
    INT         ATAD_iDirection;                                        /*  Transfer direction          */
    INT8       *ATAD_pcBuf;                                             /*  Current position in an user
                                                                            buffer                      */
    INT8       *ATAD_pcBufEnd;                                          /*  End of user buffer          */

    /*
     *  ATAPI Registers contents
     */
    UINT8       ATAD_ucIntReason;                                       /*  Interrupt Reason Register   */
    UINT8       ATAD_ucStatus;                                          /*  Status Register             */
    UINT16      ATAD_usTransSize;                                       /*  Byte Count Register         */
} __ATA_DEV;
typedef __ATA_DEV     *__PATA_DEV;
/*********************************************************************************************************
  ATA�豸������Ϣ
*********************************************************************************************************/
typedef struct __ata_type {
    INT         ATATYPE_iCylinders;                                     /*  number of cylinders         */
    INT         ATATYPE_iHeads;                                         /*  number of heads             */
    INT         ATATYPE_iSectors;                                       /*  number of sectors per track */
    INT         ATATYPE_iBytes;                                         /*  number of bytes per sector  */
    INT         ATATYPE_iPrecomp;                                       /*  precompensation cylinder    */
} __ATA_TYPE;
typedef __ATA_TYPE    *__PATA_TYPE;
/*********************************************************************************************************
  ATA�豸��������Ϣ
*********************************************************************************************************/
typedef struct __ata_drive {
    __ATA_PARAM   ATADRIVE_ataParam;                                    /*  �豸����                    */
    __ATA_TYPE   *ATADRIVE_patatypeDriverInfo;                          /*  �豸������Ϣ                */
    INT16         ATADRIVE_sOkMulti;                                    /*  �Ƿ�֧�ֶ��ض�д            */
    INT16         ATADRIVE_sOkIordy;                                    /*  IORY�Ƿ�ʹ��                */
    INT16         ATADRIVE_sOkDma;                                      /*  �Ƿ�ʹ��DMA                 */
    INT16         ATADRIVE_sOkLba;                                      /*  �Ƿ�ʹ��LBA                 */
    INT16         ATADRIVE_sMultiSecs;                                  /*  ���ض�д��֧�ֵ����������  */
    INT16         ATADRIVE_sPioMode;                                    /*  ��ߵ�PIOģʽ               */
    INT16         ATADRIVE_sSingleDmaMode;                              /*  ֧�����ĵ���DMAģʽ       */
    INT16         ATADRIVE_sMultiDmaMode;                               /*  ֧�����Ķ�����DMAģʽ     */
    INT16         ATADRIVE_sUltraDmaMode;                               /*  ֧������ultra��DMAģʽ    */
    INT16         ATADRIVE_sRwMode;                                     /*  RWģʽ: PIO[0~4]��DMA[0~2]  */
    INT16         ATADRIVE_sRwBits;                                     /*  RWλ:8��16                  */
    INT16         ATADRIVE_sRwPio;                                      /*  RW PIO ��Ԫ:�������������  */
    INT16         ATADRIVE_sRwDma;                                      /*  RW DMA ��Ԫ:�������������  */

    UINT8         ATADRIVE_ucState;                                     /*  device state                */
    UINT8         ATADRIVE_ucDiagCode;                                  /*  diagnostic code             */
    UINT8         ATADRIVE_ucType;                                      /*  device type                 */
    UINT32        ATADRIVE_uiCapacity;                                  /*  LBAģʽ�¿�Ѱַ����������   */
    UINT32        ATADRIVE_uiSignature;
} __ATA_DRIVE;
typedef __ATA_DRIVE    *__PATA_DRIVE;
/*********************************************************************************************************
  ATA��������Ϣ
*********************************************************************************************************/
struct  __ata_ctrl;
typedef struct __ata_ctrl    __ATA_CTRL;
typedef __ATA_CTRL          *__PATA_CTRL;

struct __ata_ctrl {
    ATA_CHAN         *ATACTRL_pataChan;                                 /*  ata�豸ͨ��                 */
    __ATA_DRIVE       ATACTRL_ataDrive[__ATA_MAX_DRIVES];               /*  ata�豸������               */
    ATA_REG           ATACTRL_ataReg;                                   /*  ata�����ļ��Ĵ���           */
    
    LW_OBJECT_HANDLE  ATACTRL_ulSyncSem;                                /*  ͬ���ź���                  */
    LW_OBJECT_HANDLE  ATACTRL_ulMuteSem;                                /*  �����ź���                  */

    INT               ATACTRL_iCtrl;                                    /*  ��������                    */
    INT               ATACTRL_iDrives;                                  /*  ����������                  */
    INT               ATACTRL_iBytesPerSector;                          /*  ÿ����������                */
    INT               ATACTRL_iIntStatus;                               /*  �ж�״̬                    */
    INT               ATACTRL_iConfigType;                              /*  ��������������              */
    ULONG             ATACTRL_ulSyncSemTimeout;                         /*  ͬ���źų�ʱʱ��            */

    BOOL              ATACTRL_bPreadBeSwap;                             /*  ��� CPU Pread �Ƿ���Ҫ��ת */
    BOOL              ATACTRL_bIntDisable;                              /*  �����ֹ���ж�,��ΪLW_TRUE  */
    BOOL              ATACTRL_bIsExist;                                 /*  ���������, ��ΪLW_TRUE     */

    LW_SPINLOCK_DEFINE  (ATACTRL_slLock);                               /*  ������                      */
};
/*********************************************************************************************************
  ATA����������
*********************************************************************************************************/

#define __ATA_CTRL_OUTBYTE(patactrl, ulIoAddr, uiData)                                          \
        (patactrl)->ATACTRL_pataChan->pDrvFuncs->ioOutByte((patactrl)->ATACTRL_pataChan,        \
                                                           ulIoAddr,                            \
                                                           uiData)

#define __ATA_CTRL_INBYTE(patactrl, ulIoAddr)                                                   \
        (patactrl)->ATACTRL_pataChan->pDrvFuncs->ioInByte((patactrl)->ATACTRL_pataChan,         \
                                                           ulIoAddr)

#define __ATA_CTRL_OUTSTRING(patactrl, ulIoAddr, psBuff, iWord)                                 \
        (patactrl)->ATACTRL_pataChan->pDrvFuncs->ioOutWordString((patactrl)->ATACTRL_pataChan,  \
                                                                  ulIoAddr,                     \
                                                                  psBuff,                       \
                                                                  iWord)

#define __ATA_CTRL_INSTRING(patactrl, ulIoAddr, psBuff, iWord)                                  \
        (patactrl)->ATACTRL_pataChan->pDrvFuncs->ioInWordString((patactrl)->ATACTRL_pataChan,   \
                                                                 ulIoAddr,                      \
                                                                 psBuff,                        \
                                                                 iWord)

#define __ATA_CTRL_IOCTRL(patactrl, iCmd, pvArg)                                                \
        (patactrl)->ATACTRL_pataChan->pDrvFuncs->ioctl((patactrl)->ATACTRL_pataChan,            \
                                                        iCmd,                                   \
                                                        pvArg)

#define __ATA_CTRL_CBINSTALL(patactrl, iCallbackType, callback, pvCallbackArg)                  \
        (patactrl)->ATACTRL_pataChan->pDrvFuncs->callbackInstall((patactrl)->ATACTRL_pataChan,  \
                                                                  iCallbackType,                \
                                                                  callback,                     \
                                                                  pvCallbackArg)

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
#endif                                                                  /*  __ATALIB_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
