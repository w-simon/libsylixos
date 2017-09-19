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
** ��   ��   ��: ata.c
**
** ��   ��   ��: Hou.XiaoLong (��С��)
**
** �ļ���������: 2010 �� 02 �� 23 ��
**
** ��        ��: ATA �豸����

** BUG:
2010.05.10  �޸�__ataRW()����,�ں����Ŀ�ʼ���жϿ��Ƿ����,���ڶ�__ataWait()�ķ���ֵ�����ж�.
            �����ڽ���ͬ���ź�ǰ�ȵȴ����豸��æ
2011.04.06  Ϊ����64λCPU���޸�__ataBlkRW()��__ataBlkRd()��__ataBlkWrt()�Ĳ�������.
2014.03.03  �Ż�����.
2017.09.12  ֧�ֻ�ȡ������Ϣ. Gong.YuJian (�����)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "ataLib.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_ATA_EN > 0)
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static __PATA_CTRL  _G_patactrlTable[LW_CFG_ATA_MAX_CTLS];
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT                __ataCmd(__PATA_CTRL pAtaCtrl,
                            INT         iDrive,
                            INT         iCmd,
                            INT         iArg0,
                            INT         iArg1);
INT                __ataWait(__PATA_CTRL patactrler, INT iRequest);
INT                __ataCtrlReset(__PATA_CTRL pAtaCtrl);
INT                __ataDriveInit(__PATA_CTRL patactrler, INT iDrive);
PCHAR              __ataDriveSerialInfoGet(__PATA_DRIVE hDrive, PCHAR  pcBuf, size_t stLen);
PCHAR              __ataDriveFwRevInfoGet(__PATA_DRIVE hDrive, PCHAR  pcBuf, size_t stLen);
PCHAR              __ataDriveModelInfoGet(__PATA_DRIVE hDrive, PCHAR  pcBuf, size_t stLen);
static __PATA_CTRL __ataSearchNode(INT iCtrl);
static INT         __ataAddNode(__PATA_CTRL patactrl);
static INT         __ataWrite(__PATA_CTRL patactrl);
static INT         __ataRW(__PATA_CTRL patactrler,
                           INT         iDrive,
                           INT         iCylinder,
                           INT         iHead,
                           INT         iSector,
                           PVOID       pBuffer,
                           INT         iNSecs,
                           INT         iDirection,
                           INT         iRetry);
static INT         __ataBlkRW(__ATA_DEV  *patadev,
                              VOID       *pvBuffer,
                              ULONG       ulStartBlk,
                              ULONG       ulNBlks,
                              INT         iDirection);
static INT         __ataBlkRd(__PATA_DEV  patadev,
                              VOID       *pvBuffer,
                              ULONG       ulStartBlk,
                              ULONG       ulNBlks);
static INT         __ataBlkWrt(__PATA_DEV  patadev,
                               VOID       *pvBuffer,
                               ULONG       ulStartBlk,
                               ULONG       ulNBlks);
static INT         __ataIoctl(__PATA_DEV patadev,
                              INT        iCmd,
                              LONG       lArg);
static INT         __ataStatus(__PATA_DEV  patadev);
static INT         __ataReset(__PATA_DEV  patadev);
/*********************************************************************************************************
  IO ����
*********************************************************************************************************/
#define __ATA_CMD_USERBASE          0x2000                              /*  �û�������ʼֵ              */
#define __ATA_CMD_ENB_POWERMANAGE   (0x01 + __ATA_CMD_USERBASE)         /*  ʹ�ܸ߼���Դ����            */
#define __ATA_CMD_DIS_POWERMANAGE   (0x02 + __ATA_CMD_USERBASE)         /*  ��ֹ�߼���Դ����            */
#define __ATA_CMD_IDLEIMMEDIATE     (0x03 + __ATA_CMD_USERBASE)         /*  �豸��������                */
#define __ATA_CMD_STANDBYIMMEDIATE  (0x04 + __ATA_CMD_USERBASE)         /*  �豸����׼����              */
#define __ATA_CMD_HARDRESET         (0x05 + __ATA_CMD_USERBASE)         /*  �豸Ӳ����λ                */
/*********************************************************************************************************
  ATACTRL LOCK
*********************************************************************************************************/
#define ATACTRL_LOCK(atactrl)       \
        API_SemaphoreMPend(patactrl->ATACTRL_ulMuteSem, LW_OPTION_WAIT_INFINITE)
#define ATACTRL_UNLOCK(atactrl)     \
        API_SemaphoreMPost(patactrl->ATACTRL_ulMuteSem)
/*********************************************************************************************************
** ��������: __ataSearchNode
** ��������: ���ݿ�������,����ATA�豸�ڵ�
** �䡡��  : iCtrl       ��������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __PATA_CTRL __ataSearchNode (INT iCtrl)
{
    if (iCtrl < LW_CFG_ATA_MAX_CTLS) {
        return  (_G_patactrlTable[iCtrl]);

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __ataAddNode
** ��������: ���ATA�ڵ�
** �䡡��  : patactrl       ATA�ڵ�ṹָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataAddNode (__PATA_CTRL patactrl)
{
    _G_patactrlTable[patactrl->ATACTRL_iCtrl] = patactrl;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ataWrite
** ��������: ata�豸д�����ص�
** �䡡��  : patactrl   ATA�������ڵ�ṹָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ataWrite (__PATA_CTRL patactrl)
{
    INTREG        iregInterLevel;

    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&patactrl->ATACTRL_slLock, &iregInterLevel);
    patactrl->ATACTRL_iIntStatus =  __ATA_CTRL_INBYTE(patactrl, __ATA_STATUS(patactrl));
    LW_SPIN_UNLOCK_QUICK(&patactrl->ATACTRL_slLock, iregInterLevel);

    API_SemaphoreBPost(patactrl->ATACTRL_ulSyncSem);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ataDevChk
** ��������: ata�豸��⺯��
** �䡡��  : patactrl    ATA�������ṹָ��
**           bDevIsExist �豸�Ƿ���ڱ�־
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ataDevChk (__PATA_CTRL patactrl, BOOL bDevIsExist)
{
    INTREG        iregInterLevel;

    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&patactrl->ATACTRL_slLock, &iregInterLevel);
    patactrl->ATACTRL_bIsExist = bDevIsExist;
    LW_SPIN_UNLOCK_QUICK(&patactrl->ATACTRL_slLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ataRW
** ��������: ��д��ǰ�ŵ�һ��������������
** �䡡��  : iCtrl       ��������
**           iDrive      �豸������
**           iCylinder   ����
**           iHead       ��ͷ
**           iSector     ����
**           pusBuf      ������
**           iNSecs      ������
**           iDirection  ��������
**           iRetry      ���Դ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataRW (__PATA_CTRL patactrler,
                    INT         iDrive,
                    INT         iCylinder,
                    INT         iHead,
                    INT         iSector,
                    PVOID       pvBuffer,
                    INT         iNSecs,
                    INT         iDirection,
                    INT         iRetry)
{
    __PATA_CTRL  patactrl  = LW_NULL;
    __PATA_DRIVE patadrive = LW_NULL;
    __PATA_TYPE  patatype  = LW_NULL;

    INT     iRetryCount    = 0;
    INT     iBlock         = 1;
    INT     iNSectors      = iNSecs;
    INT     iNWords        = 0;
    INT     iStatus        = 0;
    ULONG   ulSemStatus    = ERROR_NONE;
    volatile INT i         = 0;

    INT16 *psBuf           = LW_NULL;

    if ((!patactrler) || (!pvBuffer)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "buffer address error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl  = patactrler;
    patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
    patatype  = patadrive->ATADRIVE_patatypeDriverInfo;

    ATA_DEBUG_MSG(("__ataRW(): ctrl=%d drive=%d c=%d h=%d s=%d n=%d dir=%d\n",
                  patactrl->ATACTRL_iCtrl, iDrive, iCylinder, iHead, iSector, iNSecs, iDirection));

    if (patactrl->ATACTRL_bIsExist != LW_TRUE) {                        /*  ATA�豸������               */
        return  (PX_ERROR);
    }

__retry_rw:
    iStatus = __ataWait(patactrl, __ATA_STAT_IDLE);                     /*  �ȴ�BSY��DRQΪ0             */
    if (iStatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    psBuf     = (INT16 *)pvBuffer;
    iNSectors = iNSecs;

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECTOR(patactrl), (UINT8)(iSector >> 8));
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECTOR(patactrl), (UINT8)iSector);
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), (UINT8)iNSecs);
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLLO(patactrl), (UINT8)iCylinder);
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLHI(patactrl), (UINT8)(iCylinder >> 8));

    iStatus = __ataWait(patactrl, __ATA_STAT_IDLE);
    if (iStatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    if (patadrive->ATADRIVE_sOkLba) {
        __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),
                           (UINT8)(__ATA_SDH_LBA        |         \
                           (iDrive << __ATA_DRIVE_BIT)  |         \
                           (iHead & 0xf)));

    } else {
        __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),
                           (UINT8)(__ATA_SDH_CHS        |         \
                           (iDrive << __ATA_DRIVE_BIT)  |         \
                           (iHead & 0xf)));
    }

    if (patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) {
        iBlock = patadrive->ATADRIVE_sMultiSecs;
    }

    iNWords = (patatype->ATATYPE_iBytes * iBlock) >> 1;                 /*  �����д�����ݵ�������      */

    iStatus = __ataWait(patactrl, __ATA_STAT_IDLE);
    if (iStatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    if (iDirection == O_WRONLY) {                                       /*  д����                      */
        if (patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) {
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_WRITE_MULTI);
                                                                        /*  ���Ͷ���д��������          */
        } else {
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_WRITE);
                                                                        /*  ���͵�����д��������        */
        }

        while (iNSectors > 0) {
            if ((patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) &&
                (iNSectors < iBlock)) {                                 /*  �������������������Ĭ������
                                                                            �Ķ��ض�д������            */
                iBlock  = iNSectors;
                iNWords = (patatype->ATATYPE_iBytes * iBlock) >> 1;     /*  һ�ζ�д����������          */
            }

            iStatus = __ataWait(patactrl, __ATA_STAT_DRQ);
            if (iStatus != ERROR_NONE) {
                ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                               patactrl->ATACTRL_iCtrl, iDrive, 
                               __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
                return  (PX_ERROR);
            }

            if (patadrive->ATADRIVE_sRwBits == ATA_BITS_8) {            /*  8λ����                     */
                INT8   *pcBuff      = (INT8 *)psBuf;
                INT     iNByteWords =  (1 << iNWords);

                for (i = 0; i < iNByteWords; i++) {
                    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DATA(patactrl), *pcBuff);
                    pcBuff++;
                }

            } else if (patadrive->ATADRIVE_sRwBits == ATA_BITS_16) {    /*  16λ����                    */
                __ATA_CTRL_OUTSTRING(patactrl, __ATA_DATA(patactrl), psBuf, iNWords);
            
            } else {                                                    /*  32λ����                    */
               /*
                *   TODO: 32λ����
                */
                ATA_DEBUG_MSG(("__ataRW() error: 32 bits write operation\n"));
                return  (PX_ERROR);
            }

            iStatus = __ataWait(patactrl, __ATA_STAT_BUSY);
            if (iStatus != ERROR_NONE) {
                ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                               patactrl->ATACTRL_iCtrl, iDrive, 
                               __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
                return  (PX_ERROR);
            }

            if (patactrl->ATACTRL_bIntDisable == LW_FALSE) {            /*  ����ʹ�����ж�              */
                ulSemStatus = API_SemaphoreBPend(patactrl->ATACTRL_ulSyncSem,   \
                                    patactrl->ATACTRL_ulSyncSemTimeout);/*  �ȴ�ͬ���ź�                */
            }

            if ((patactrl->ATACTRL_iIntStatus & __ATA_STAT_ERR) || (ulSemStatus != ERROR_NONE)) {
                goto __error_handle;
            }

            psBuf     += iNWords;                                       /*  ������������ָ��            */
            iNSectors -= iBlock;                                        /*  ���㻹�������������        */
        }

    } else {                                                            /*  ������                      */
        if (patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) {
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_READ_MULTI);
                                                                        /*  ���Ͷ��ض���������          */
        } else {
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_READ);
                                                                        /*  ���͵���������������        */
        }

        while (iNSectors > 0) {
            if ((patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) &&
                (iNSectors < iBlock)) {                                 /*  �������������������Ĭ������
                                                                            �Ķ��ض�д������            */
                iBlock  = iNSectors;
                iNWords = (patatype->ATATYPE_iBytes * iBlock) >> 1;
            }

            iStatus = __ataWait(patactrl, __ATA_STAT_BUSY);             /*  �ȴ����ٴ����豸            */
            if (iStatus != ERROR_NONE) {
                ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                               patactrl->ATACTRL_iCtrl, iDrive, 
                               __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
                return  (PX_ERROR);
            }

            iStatus = __ataWait(patactrl, __ATA_STAT_DRQ);
            if (iStatus != ERROR_NONE) {
                ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                               patactrl->ATACTRL_iCtrl, iDrive, 
                               __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
                return  (PX_ERROR);
            }

            if (patactrl->ATACTRL_bIntDisable == LW_FALSE) {
                ulSemStatus = API_SemaphoreBPend(patactrl->ATACTRL_ulSyncSem,   \
                                    patactrl->ATACTRL_ulSyncSemTimeout);/*  �ȴ�ͬ���ź�                */
            }

            if ((patactrl->ATACTRL_iIntStatus & __ATA_STAT_ERR) || (ulSemStatus != ERROR_NONE)) {
                goto __error_handle;
            }

            if (patadrive->ATADRIVE_sRwBits == ATA_BITS_8) {            /*  8λ����                     */
                INT8   *pcBuff      = (INT8 *)psBuf;
                INT     iNByteWords =  (1 << iNWords);

                for (i = 0; i < iNByteWords; i++) {
                    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DATA(patactrl), *pcBuff);
                    pcBuff++;
                }

            } else if (patadrive->ATADRIVE_sRwBits == ATA_BITS_16) {    /*  16λ����                    */
                __ATA_CTRL_INSTRING(patactrl, __ATA_DATA(patactrl), psBuf, iNWords);
            
            } else {                                                    /*  32λ����                    */
                /*
                 *   TODO: 32λ����
                 */
                ATA_DEBUG_MSG(("__ataRW() error: 32 bits read operation\n"));
                return  (PX_ERROR);
            }

            psBuf     += iNWords;                                       /*  ������������ָ��            */
            iNSectors -= iBlock;                                        /*  ���㻹�������������        */
        }
    }

    ATA_DEBUG_MSG(("__ataRW(): end\n"));
    return  (ERROR_NONE);

__error_handle:
    ATA_DEBUG_MSG(("__ataRW err: stat=0x%x 0x%x  error=0x%x\n",
                   patactrl->ATACTRL_iIntStatus,
                   __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)),
                   __ATA_CTRL_INBYTE(patactrl, __ATA_ERROR(patactrl))));
    (VOID)__ataCmd(patactrl, iDrive, __ATA_CMD_RECALIB, 0, 0);

    if (++iRetryCount < iRetry) {                                       /*  ����                        */
        goto    __retry_rw;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __ataBlkRW
** ��������: ATA�豸��д����
** �䡡��  : patadev     ���豸���ݽṹָ��
**           pvBuffer    ������
**           ulStartBlk  ����������ʼ��
**           ulNBlks     ���Ŀ���
**           iDirection  ��������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataBlkRW (__ATA_DEV  *patadev,
                       VOID       *pvBuffer,
                       ULONG       ulStartBlk,
                       ULONG       ulNBlks,
                       INT         iDirection)
{
    __PATA_CTRL   patactrl    = LW_NULL;
    __PATA_DRIVE  patadrive   = LW_NULL;
    LW_BLK_DEV   *pblkdev     = LW_NULL;
    __PATA_TYPE   patatype    = LW_NULL;

    ULONG         ulNSecs     = 0;
    INT           iRetryNum0  = 0;
    INT           iRretrySeek = 0;
    INT           iCylinder   = 0;
    INT           iHead       = 0;
    INT           iSector     = 0;
    INT           iReturn     = PX_ERROR;

    PCHAR         pcBuf       = LW_NULL;
    volatile INT  i;

    if ((!patadev) || (!pvBuffer)) {                                    /*  ��������                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = __ataSearchNode(patadev->ATAD_iCtrl);
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (patactrl->ATACTRL_bIsExist != LW_TRUE) {                        /*  ATA�豸������               */
        return  (PX_ERROR);
    }

    patadrive = &patactrl->ATACTRL_ataDrive[patadev->ATAD_iDrive];
    pblkdev   = &patadev->ATAD_blkdBlkDev;
    patatype  = patadrive->ATADRIVE_patatypeDriverInfo;

    ulNSecs   = pblkdev->BLKD_ulNSector;                                /*  ATA�豸������               */

    if ((ulStartBlk + ulNBlks) > ulNSecs) {                             /*  ��д���������豸��������    */
        ATA_DEBUG_MSG(("Error, startBlk=%d nBlks=%d: 0 - %d\n",
                       ulStartBlk, ulNBlks, ulNSecs));
        return  (PX_ERROR);
    }

    ulStartBlk += patadev->ATAD_iBlkOffset;
    pcBuf       = (PCHAR)pvBuffer;

    ATACTRL_LOCK(patactrl);                                             /*  �ȴ��ź���                  */
    
    for (i = 0; i < ulNBlks; i += ulNSecs) {
        if (patadrive->ATADRIVE_sOkLba) {                               /*  ʹ���߼���Ѱַ��ʽ          */
            iHead     = (ulStartBlk & __ATA_LBA_HEAD_MASK) >> 24;
            iCylinder = (ulStartBlk & __ATA_LBA_CYL_MASK) >> 8;
            iSector   = (ulStartBlk & __ATA_LBA_SECTOR_MASK);
        
        } else {                                                        /*  ʹ������/��ͷ/������ʽ      */
            iCylinder = (INT)(ulStartBlk / (patatype->ATATYPE_iSectors * patatype->ATATYPE_iHeads));
            iSector   = (INT)(ulStartBlk % (patatype->ATATYPE_iSectors * patatype->ATATYPE_iHeads));
            iHead     = iSector / patatype->ATATYPE_iSectors;
            iSector   = iSector % patatype->ATATYPE_iSectors + 1;
        }

        ulNSecs = ((ulNBlks - i) < __ATA_MAX_RW_SECTORS) ? (ulNBlks - i) : __ATA_MAX_RW_SECTORS;

        while (__ataRW(patactrl, patadev->ATAD_iDrive, iCylinder,
               iHead, iSector, pcBuf, ulNSecs, iDirection, pblkdev->BLKD_iRetry)) {
            (VOID)__ataCmd(patactrl, patadev->ATAD_iDrive, __ATA_CMD_RECALIB, 0, 0);
                                                                        /*  У׼                        */
            if (++iRetryNum0 > pblkdev->BLKD_iRetry) {
                iReturn = PX_ERROR;
                goto __error_handle;
            }

            iRretrySeek = 0;

            while (__ataCmd(patactrl, patadev->ATAD_iDrive, __ATA_CMD_SEEK,
                   iCylinder, iHead)) {                                 /*  ��ѯ��λ                    */
                if (++iRretrySeek > pblkdev->BLKD_iRetry) {
                    iReturn = PX_ERROR;
                    goto __error_handle;
                }
            }
        }

        ulStartBlk += ulNSecs;                                          /*  ���������ȡ��������ʼλ��  */
        pcBuf      += pblkdev->BLKD_ulBytesPerSector * ulNSecs;         /*  ���������λ��              */
    }

    iReturn = ERROR_NONE;

__error_handle:
    ATACTRL_UNLOCK(patactrl);                                           /*  �����ź���                  */

    return  (iReturn);
}
/*********************************************************************************************************
** ��������: __ataBlkRd
** ��������: ATA������
** �䡡��  : patadev     ���豸���ݽṹָ��
**           ulStartBlk  д��������ʼ��
**           ulNBlks     д�Ŀ���
**           pvBuffer    ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataBlkRd (__PATA_DEV  patadev,
                       VOID       *pvBuffer,
                       ULONG       ulStartBlk,
                       ULONG       ulNBlks)
{
    return  (__ataBlkRW(patadev, pvBuffer, ulStartBlk, ulNBlks, O_RDONLY));
}
/*********************************************************************************************************
** ��������: __ataBlkWrt
** ��������: ATAд����
** �䡡��  : patadev     ���豸���ݽṹָ��
**           ulStartBlk  д��������ʼ��
**           ulNBlks     д�Ŀ���
**           pvBuffer    ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataBlkWrt (__PATA_DEV  patadev,
                        VOID       *pvBuffer,
                        ULONG       ulStartBlk,
                        ULONG       ulNBlks)
{
    return  (__ataBlkRW(patadev, pvBuffer, ulStartBlk, ulNBlks, O_WRONLY));
}
/*********************************************************************************************************
** ��������: __ataIoctl
** ��������: ���豸���ƺ���
** �䡡��  : patadev     ���豸���ݽṹָ��
**           iCmd        ����
**           lArg        ��������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataIoctl (__PATA_DEV patadev,
                       INT        iCmd,
                       LONG       lArg)
{
    __PATA_CTRL     patactrl     = LW_NULL;
    ATA_DRV_FUNCS  *patadevfuscs = LW_NULL;
    struct timeval *timevalTemp  = LW_NULL;
    __PATA_DRIVE    hDrive;                                             /* ���������                   */
    PLW_BLK_INFO    hBlkInfo;                                           /* �豸��Ϣ                     */

    INT             iReturn     = ERROR_NONE;

    if (!patadev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = __ataSearchNode(patadev->ATAD_iCtrl);
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (patactrl->ATACTRL_bIsExist != LW_TRUE){                         /*  ATA�豸������               */
        return  (PX_ERROR);
    }

    patadevfuscs = patactrl->ATACTRL_pataChan->pDrvFuncs;

    ATACTRL_LOCK(patactrl);                                             /*  �ȴ��ź���                  */
    
    switch (iCmd) {

    case FIOSYNC:
    case FIODATASYNC:
    case FIOSYNCMETA:
    case FIOFLUSH:                                                      /*  �������д������            */
    case FIOUNMOUNT:                                                    /*  ж�ؾ�                      */
    case FIODISKINIT:                                                   /*  ��ʼ��ATA�豸               */
        break;

    case FIODISKCHANGE:                                                 /*  ����ý�ʷ����仯            */
        patadev->ATAD_blkdBlkDev.BLKD_bDiskChange = LW_TRUE;
        break;

    case LW_BLKD_GET_SECSIZE:
        *(ULONG *)lArg = patadev->ATAD_blkdBlkDev.BLKD_ulBytesPerSector;
        break;

    case LW_BLKD_GET_BLKSIZE:
        *(ULONG *)lArg = patadev->ATAD_blkdBlkDev.BLKD_ulBytesPerBlock;
        break;

    case LW_BLKD_CTRL_INFO:
        hDrive = &patactrl->ATACTRL_ataDrive[patadev->ATAD_iDrive];
        hBlkInfo = (PLW_BLK_INFO)lArg;
        if (!hBlkInfo) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
        hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_ATA;
        __ataDriveSerialInfoGet(hDrive, hBlkInfo->BLKI_cSerial,   LW_BLKD_CTRL_INFO_STR_SZ);
        __ataDriveFwRevInfoGet(hDrive,  hBlkInfo->BLKI_cFirmware, LW_BLKD_CTRL_INFO_STR_SZ);
        __ataDriveModelInfoGet(hDrive,  hBlkInfo->BLKI_cProduct,  LW_BLKD_CTRL_INFO_STR_SZ);
        break;

    case FIOWTIMEOUT:
        if (lArg) {
            timevalTemp = (struct timeval *)lArg;
            patactrl->ATACTRL_ulSyncSemTimeout = __timevalToTick(timevalTemp);
                                                                        /*  ת��Ϊϵͳʱ��              */
        } else {
            patactrl->ATACTRL_ulSyncSemTimeout = LW_OPTION_WAIT_INFINITE;
        }
        break;

    case FIORTIMEOUT:
        if (lArg) {
            timevalTemp = (struct timeval *)lArg;
            patactrl->ATACTRL_ulSyncSemTimeout = __timevalToTick(timevalTemp);
                                                                        /*  ת��Ϊϵͳʱ��              */
        } else {
            patactrl->ATACTRL_ulSyncSemTimeout = LW_OPTION_WAIT_INFINITE;
        }
        break;

    case __ATA_CMD_ENB_POWERMANAGE:
        iReturn = __ataCmd(patactrl,
                           patadev->ATAD_iDrive,
                           __ATA_CMD_SET_FEATURE,
                           __ATA_SUB_ENABLE_APM,
                           (INT)lArg);
        break;

    case __ATA_CMD_DIS_POWERMANAGE:
        iReturn = __ataCmd(patactrl,
                           patadev->ATAD_iDrive,
                           __ATA_CMD_SET_FEATURE,
                           __ATA_SUB_DISABLE_APM,
                           (INT)lArg);
        break;

    case __ATA_CMD_IDLEIMMEDIATE:                                       /*  ʹָ���豸��������          */
        iReturn = __ataCmd(patactrl,
                           patadev->ATAD_iDrive,
                           __ATA_CMD_IDLE_IMMEDIATE,
                           0, 0);
        break;

    case __ATA_CMD_STANDBYIMMEDIATE:                                    /*  ʹָ���豸��������          */
        iReturn = __ataCmd(patactrl,
                           patadev->ATAD_iDrive,
                           __ATA_CMD_STANDBY_IMMEDIATE,
                           0, 0);
        break;

    case __ATA_CMD_HARDRESET:
        if (patadevfuscs->sysReset) {
            patadevfuscs->sysReset(patactrl->ATACTRL_pataChan, (INT)lArg);
        }
        break;

    default:
        iReturn = patadevfuscs->ioctl(patactrl->ATACTRL_pataChan, iCmd, (PVOID)lArg);
        if (iReturn != ERROR_NONE) {                                    /*  ����                        */
            _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        }
        break;
    }

    ATACTRL_UNLOCK(patactrl);                                           /*  �����ź���                  */

    return  (iReturn);
}
/*********************************************************************************************************
** ��������: ataStatus
** ��������: ���ATA�豸״̬
** �䡡��  : patadev       ���豸���ݽṹָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataStatus (__PATA_DEV  patadev)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ataReset
** ��������: ��λATA�豸
** �䡡��  : patadev       ���豸���ݽṹָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataReset (__PATA_DEV  patadev)
{
    __PATA_CTRL  patactrl  = LW_NULL;
    __PATA_DRIVE patadrive = LW_NULL;

    if (!patadev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl  = __ataSearchNode(patadev->ATAD_iCtrl);
    patadrive = &patactrl->ATACTRL_ataDrive[patactrl->ATACTRL_iDrives];

    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (patactrl->ATACTRL_bIsExist != LW_TRUE) {                        /*  ATA�豸������               */
        return  (PX_ERROR);
    }

    if (patadrive->ATADRIVE_ucState != __ATA_DEV_MED_CH) {
        ATACTRL_LOCK(patactrl);

        __ATA_CTRL_OUTBYTE(patactrl, __ATA_DCONTROL(patactrl), (__ATA_CTL_RST | __ATA_CTL_IDS));

        __ATA_DELAYMS(100);

        __ATA_CTRL_OUTBYTE(patactrl,                                        \
                           __ATA_DCONTROL(patactrl),                        \
                          (UINT8)(patactrl->ATACTRL_bIntDisable << 1)); /*  �����λ,�������ж�λ       */
        __ATA_DELAYMS(100);

        if (__ataDriveInit(patactrl, patadev->ATAD_iDrive) != ERROR_NONE) {
            ATACTRL_UNLOCK(patactrl);
            return  (PX_ERROR);
        }

        ATACTRL_UNLOCK(patactrl);
    }

    patadrive->ATADRIVE_ucState = __ATA_DEV_OK;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AtaDrv
** ��������: ��װ ATA �豸�������� (ÿ�β忨����Ҫ����, Ȼ����ܴ����豸)
** �䡡��  : patachan ATA�豸ͨ���ṹָ��
**           patacp   ATA�豸ͨ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_AtaDrv (ATA_CHAN  *patachan, ATA_CHAN_PARAM *patacp)
{
    __PATA_CTRL      patactrl   = LW_NULL;
    __PATA_DRIVE     patadrive  = LW_NULL;
    __PATA_TYPE      patatype   = LW_NULL;

    INT              iDrive;
    INT              iError     = 0;

    if ((!patachan) || (!patacp)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((!patachan->pDrvFuncs->ioctl)           ||
        (!patachan->pDrvFuncs->ioOutByte)       ||
        (!patachan->pDrvFuncs->ioInByte)        ||
        (!patachan->pDrvFuncs->ioOutWordString) ||
        (!patachan->pDrvFuncs->ioInWordString)  ||
        (!patachan->pDrvFuncs->callbackInstall)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (patacp->ATACP_iCtrlNum >= LW_CFG_ATA_MAX_CTLS) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "IDE controller number error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((patacp->ATACP_iDrives >= __ATA_MAX_DRIVES) ||
        (patacp->ATACP_iDrives <= 0)) {
        patacp->ATACP_iDrives = 1;
    }

    patactrl = __ataSearchNode(patacp->ATACP_iCtrlNum);
    if (patactrl != LW_NULL) {                                          /*  ��Ϊ��,���Ѱ�װ������       */
        goto    __drive_init;
    }

    patactrl = (__PATA_CTRL)__SHEAP_ALLOC(sizeof(__ATA_CTRL));          /*  Ϊ�ÿ����������ڴ�          */
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(patactrl, sizeof(__ATA_CTRL));

    /*
     *  ����ؼ�����
     */
    patactrl->ATACTRL_iCtrl           = patacp->ATACP_iCtrlNum;         /*  �����������                */
    patactrl->ATACTRL_iDrives         = patacp->ATACP_iDrives;          /*  �����豸����                */
    patactrl->ATACTRL_iBytesPerSector = patacp->ATACP_iBytesPerSector;
                                                                        /*  ÿ�����Ĵ�С                */
    patactrl->ATACTRL_iConfigType     = patacp->ATACP_iConfigType;
    patactrl->ATACTRL_ataReg          = patacp->ATACP_atareg;

    patactrl->ATACTRL_pataChan = patachan;

    patactrl->ATACTRL_ulMuteSem = API_SemaphoreMCreate("ata_mutesem",
                                                       LW_PRIO_DEF_CEILING,
                                                       LW_OPTION_WAIT_PRIORITY |
                                                       LW_OPTION_INHERIT_PRIORITY | 
                                                       LW_OPTION_DELETE_SAFE |
                                                       LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);        /*  ���������ź���              */
    if (!(patactrl->ATACTRL_ulMuteSem)) {
        __SHEAP_FREE(patactrl);                                         /*  �ͷ��ڴ�                    */
        return  (PX_ERROR);
    }

    patactrl->ATACTRL_ulSyncSem = API_SemaphoreBCreate("ata_syncsem",
                                                       LW_TRUE,
                                                       LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);        /*  ����ͬ���ź���              */
    if (!(patactrl->ATACTRL_ulSyncSem)) {
        API_SemaphoreMDelete(&patactrl->ATACTRL_ulMuteSem);
        __SHEAP_FREE(patactrl);                                         /*  �ͷ��ڴ�                    */
        return  (PX_ERROR);
    }

    patactrl->ATACTRL_ulSyncSemTimeout = patacp->ATACP_ulSyncSemTimeout;/*  ����ͬ���ź�����ʱʱ��      */

    LW_SPIN_INIT(&patactrl->ATACTRL_slLock);                            /*  ��ʼ��������                */

    __ATA_CTRL_CBINSTALL(patactrl, ATA_CALLBACK_CHECK_DEV,
                         (ATA_CALLBACK)__ataDevChk, (PVOID)patactrl);   /*  ��װ��⺯���ص�            */

    patactrl->ATACTRL_bPreadBeSwap = patacp->ATACP_bPreadBeSwap;
    
    if (patacp->ATACP_bIntEnable == LW_TRUE) {
        patactrl->ATACTRL_bIntDisable = LW_FALSE;                       /*  �����ж�                    */
        __ATA_CTRL_CBINSTALL(patactrl, ATA_CALLBACK_WRITE_DATA,    \
                            (ATA_CALLBACK)__ataWrite, (PVOID)patactrl); /*  ��װд���������ص�          */

    } else {
        patactrl->ATACTRL_bIntDisable = LW_TRUE;                        /*  ��ֹ�ж�                    */
    }

    /*
     *  Ϊ�ÿ����������ڴ�
     */
    patatype = (__PATA_TYPE)__SHEAP_ALLOC(sizeof(__ATA_TYPE) * (size_t)patactrl->ATACTRL_iDrives);
                                                                        /*  Ϊ�ÿ����������ڴ�          */
    if (!patatype) {
        API_SemaphoreMDelete(&patactrl->ATACTRL_ulMuteSem);
        API_SemaphoreBDelete(&patactrl->ATACTRL_ulSyncSem);
        __SHEAP_FREE(patactrl);                                         /*  �ͷ��ڴ�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "invalid controller number or number of drives.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(patatype, (sizeof(__ATA_TYPE) * patactrl->ATACTRL_iDrives));

    for (iDrive = 0; iDrive < patactrl->ATACTRL_iDrives; iDrive++) {
        patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
        patadrive->ATADRIVE_ucState = __ATA_DEV_INIT;
        patadrive->ATADRIVE_ucType  = __ATA_TYPE_INIT;

        patadrive->ATADRIVE_patatypeDriverInfo = patatype;              /*  �����豸��Ϣָ��            */
        patatype++;
    }

    /*
     *  ���豸��������
     */
    __ataAddNode(patactrl);

__drive_init:
    if (__ataCtrlReset(patactrl) != ERROR_NONE) {                       /*  ��λ������                  */
        ATA_DEBUG_MSG(("API_AtaDrv(): Controller %d reset failed\r\n", patactrl->ATACTRL_iCtrl));
        return  (PX_ERROR);
    }

    for (iDrive = 0; iDrive < patactrl->ATACTRL_iDrives; iDrive++) {
        if ((__ataDriveInit(patactrl, iDrive)) != ERROR_NONE) {
            iError++;
            ATA_DEBUG_MSG(("API_AtaDrv(): ataDriveInit failed on Channel %d Device %d\n",
                           patactrl->ATACTRL_iCtrl, iDrive));
            if (iError == patactrl->ATACTRL_iDrives) {                  /*  �����������豸��ʼ����ʧ��  */
                return  (PX_ERROR);
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AtaDevCreate
** ��������: ����ATA���豸
** �䡡��  : iCtrl       ��������,0Ϊ��һ��������
**           iDrive      ������,0Ϊ���豸
**           iNBlocks    �豸�Ŀ�����,0Ϊʹ����������
**           iBlkOffset  �豸��ƫ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PLW_BLK_DEV API_AtaDevCreate (INT iCtrl,
                              INT iDrive,
                              INT iNBlocks,
                              INT iBlkOffset)
{
    __PATA_CTRL  patactrl   = LW_NULL;
    __PATA_DRIVE patadrive  = LW_NULL;
    __PATA_TYPE  patatype   = LW_NULL;
    __PATA_DEV   patadev    = LW_NULL;
    PLW_BLK_DEV  pblkdev    = LW_NULL;

    UINT         uiMaxBlks;

    if (iDrive >= __ATA_MAX_DRIVES) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
    }

    patactrl = __ataSearchNode(iCtrl);
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
    patatype  = patadrive->ATADRIVE_patatypeDriverInfo;

    patadev = (__PATA_DEV)__SHEAP_ALLOC(sizeof(__ATA_DEV));
    if (!patadev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(patadev, sizeof(__ATA_DEV));                              /*  ����ڴ�                    */

    pblkdev = &patadev->ATAD_blkdBlkDev;

    if ((patadrive->ATADRIVE_ucState == __ATA_DEV_OK) ||
        (patadrive->ATADRIVE_ucState == __ATA_DEV_MED_CH)) {
        if (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATA) {
            if ((patadrive->ATADRIVE_sOkLba == 1)      &&
                (patadrive->ATADRIVE_uiCapacity != 0)  &&
                (patadrive->ATADRIVE_uiCapacity >
                (patatype->ATATYPE_iCylinders *
                 patatype->ATATYPE_iHeads     *
                 patatype->ATATYPE_iSectors))) {                        /*  LBAģʽ                     */
                uiMaxBlks = (UINT)(patadrive->ATADRIVE_uiCapacity - iBlkOffset);
            
            } else {                                                    /*  ʹ��CHSģʽ                 */
                uiMaxBlks = ((patatype->ATATYPE_iCylinders *
                              patatype->ATATYPE_iHeads     *
                              patatype->ATATYPE_iSectors)  - iBlkOffset);
            }

            if (iNBlocks == 0) {
                iNBlocks = uiMaxBlks;
            }

            if (iNBlocks > uiMaxBlks) {
                iNBlocks = uiMaxBlks;
            }

            pblkdev->BLKD_pcName            = "ATA-Hard Disk";
            pblkdev->BLKD_pfuncBlkRd        = __ataBlkRd;               /*  ����������                  */
            pblkdev->BLKD_pfuncBlkWrt       = __ataBlkWrt;              /*  д��������                  */
            pblkdev->BLKD_pfuncBlkIoctl     = __ataIoctl;               /*  IOCTRL����                  */
            pblkdev->BLKD_pfuncBlkReset     = __ataReset;               /*  ��λ����                    */
            pblkdev->BLKD_pfuncBlkStatusChk = __ataStatus;              /*  ״̬���                    */
            pblkdev->BLKD_ulNSector         = iNBlocks;                 /*  ��������                    */
            pblkdev->BLKD_ulBytesPerSector  = patactrl->ATACTRL_iBytesPerSector;
                                                                        /*  ÿ�����ֽ���                */
            pblkdev->BLKD_ulBytesPerBlock   = 4096;
                                                                        /*  ÿ���ֽ���                  */
            pblkdev->BLKD_bRemovable        = LW_TRUE;                  /*  ���ƶ�                      */
            pblkdev->BLKD_bDiskChange       = LW_FALSE;                 /*  ý��û�иı�                */
            pblkdev->BLKD_iRetry            = __ATA_RETRY_TIMES;        /*  ���Դ���                    */
            pblkdev->BLKD_iFlag             = O_RDWR;                   /*  ��д����                    */
            pblkdev->BLKD_iLogic            = 0;
            pblkdev->BLKD_uiLinkCounter     = 0;
            pblkdev->BLKD_pvLink            = LW_NULL;
            pblkdev->BLKD_uiPowerCounter    = 0;
            pblkdev->BLKD_uiInitCounter     = 0;

            patadev->ATAD_iCtrl             = iCtrl;                    /*  ��������                    */
            patadev->ATAD_iDrive            = iDrive;                   /*  �豸��                      */
            patadev->ATAD_iBlkOffset        = iBlkOffset;               /*  ƫ����                      */
            patadev->ATAD_iNBlocks          = iNBlocks;                 /*  �ܿ���                      */

            return  (pblkdev);                                          /*  ���ؿ��豸                  */

        } else if (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATAPI) {
            /*
             * TODO:  ATAPI�����豸��֧��
             */
        }
    }
    __SHEAP_FREE(patadev);                                              /*  �ͷ��ڴ�                    */

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_AtaDevDelete
** ��������: ɾ��ATA���豸
** �䡡��  : pblkdev       ���豸
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_AtaDevDelete (PLW_BLK_DEV pblkdev)
{
    __PATA_CTRL patactrl = LW_NULL;
    __PATA_DEV  patadev  = LW_NULL;

    if (!pblkdev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patadev  = (__PATA_DEV)pblkdev;
    patactrl = __ataSearchNode(patadev->ATAD_iCtrl);
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ATACTRL_LOCK(patactrl);
    __SHEAP_FREE(pblkdev);                                              /*  �ͷ��ڴ�                    */
    ATACTRL_UNLOCK(patactrl);                                           /*  �����ź���                  */

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
