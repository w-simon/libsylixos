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
** ��   ��   ��: ataLib.c
**
** ��   ��   ��: Hou.XiaoLong (��С��)
**
** �ļ���������: 2010 �� 02 �� 01 ��
**
** ��        ��: ATA�豸��

** BUG:
2010.03.29  __ataWait()����ʹ���µ���ʱ�ȴ�����.
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
static INT  _G_iAtaRetry    = __ATA_RETRY_TIMES;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT        __ataCmd(__PATA_CTRL patactrler,
                   INT         iDrive,
                   INT         iCmd,
                   INT         iArg0,
                   INT         iArg1);
INT        __ataDevIdentify(__PATA_CTRL patactrler, INT iDrive);
INT        __ataWait(__PATA_CTRL patactrler, INT iRequest);
static INT __ataPread(__PATA_CTRL patactrler,
                      INT         iDrive,
                      PVOID       pvBuffer);
/*********************************************************************************************************
** ��������: __ataIdString
** ��������: �� ID ֵת��Ϊ�ַ�����Ϣ
** �䡡��  : pusId      ID ����
**           pcBuff     �ַ�������
**           uiLen      �����С
** �䡡��  : �ַ�����Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PCHAR  __ataIdString (const UINT16 *pusId, PCHAR  pcBuff, UINT32  uiLen)
{
    REGISTER INT    i;
    UINT8           ucChar   = 0;                                       /* �ֽ�����                     */
    INT             iOffset  = 0;                                       /* ��ƫ��                       */
    PCHAR           pcString = pcBuff;                                  /* �ַ���������                 */

    /*
     *  ��������ת��Ϊ�ַ�������
     */
    iOffset   = 0;
    pcString = pcBuff;
    for (i = 0; i < (uiLen - 1); ) {
        ucChar = (UINT8)(pusId[iOffset] >> 8);
        *pcString = ucChar;
        pcString += 1;

        ucChar = (UINT8)(pusId[iOffset] & 0xff);
        *pcString = ucChar;
        pcString += 1;

        iOffset += 1;
        i       += 2;
    }

    /*
     *  ɾ����Ч�ո���Ϣ
     */
    pcString = pcBuff + lib_strnlen(pcBuff, uiLen - 1);
    while ((pcString > pcBuff) && (pcString[-1] == ' ')) {
        pcString--;
    }
    *pcString = '\0';

    iOffset   = 0;
    pcString = pcBuff;
    for (i = 0; i < uiLen; i++) {
        if (pcString[iOffset] == ' ') {
            pcString += 1;
        } else {
            break;
        }
    }

    return  (pcString);
}
/*********************************************************************************************************
** ��������: __ataDriveSerialInfoGet
** ��������: ���к���Ϣ
** �䡡��  : hDrive     ���������
**           pcBuf      ������
**           stLen      ��������С
** �䡡��  : �ַ�����Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PCHAR  __ataDriveSerialInfoGet (__PATA_DRIVE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    __ATA_PARAM    *hParam;                                             /* �������                     */
    PCHAR           pcSerial;                                           /* �豸����                     */
    CHAR            cSerial[21] = { 0 };                                /* ���кŻ�����                 */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->ATADRIVE_ataParam;
    lib_bzero(&cSerial[0], 21);
    pcSerial = __ataIdString((const UINT16 *)&hParam->ATAPAR_cSerial[0], &cSerial[0], 21);
    lib_strncpy(pcBuf, pcSerial, __MIN(stLen, lib_strlen(pcSerial) + 1));

    return  (pcSerial);
}
/*********************************************************************************************************
** ��������: __ataDriveFwRevInfoGet
** ��������: �̼��汾����Ϣ
** �䡡��  : hDrive     ���������
**           pcBuf      ������
**           stLen      ��������С
** �䡡��  : �ַ�����Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PCHAR  __ataDriveFwRevInfoGet (__PATA_DRIVE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    __ATA_PARAM    *hParam;                                             /* �������                     */
    PCHAR           pcFirmware;                                         /* �̼��汾                     */
    CHAR            cFirmware[9] = { 0 };                               /* �̼��汾������               */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->ATADRIVE_ataParam;
    lib_bzero(&cFirmware[0], 9);
    pcFirmware =__ataIdString((const UINT16 *)&hParam->ATAPAR_cRev[0], &cFirmware[0], 9);
    lib_strncpy(pcBuf, pcFirmware, __MIN(stLen, lib_strlen(pcFirmware) + 1));

    return  (pcFirmware);
}
/*********************************************************************************************************
** ��������: __ataDriveModelInfoGet
** ��������: �豸��ϸ��Ϣ
** �䡡��  : hDrive     ���������
**           pcBuf      ������
**           stLen      ��������С
** �䡡��  : �ַ�����Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PCHAR  __ataDriveModelInfoGet (__PATA_DRIVE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    __ATA_PARAM    *hParam;                                             /* �������                     */
    PCHAR           pcProduct;                                          /* ��Ʒ��Ϣ                     */
    CHAR            cProduct[41] = { 0 };                               /* ��Ʒ��Ϣ������               */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->ATADRIVE_ataParam;
    lib_bzero(&cProduct[0], 41);
    pcProduct = __ataIdString((const UINT16 *)&hParam->ATAPAR_cModel[0], &cProduct[0], 41);
    lib_strncpy(pcBuf, pcProduct, __MIN(stLen, lib_strlen(pcProduct) + 1));

    return  (pcProduct);
}
/*********************************************************************************************************
** ��������: __ataCmd
** ��������: ATA����
** �䡡��  : patactrler  ATA�������ṹָ��
**           iDrive      ������
**           iCmd        ATA����
**           iArg0       ����0
**           iArg1       ����1
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __ataCmd (__PATA_CTRL patactrler,
              INT         iDrive,
              INT         iCmd,
              INT         iArg0,
              INT         iArg1)
{
    __PATA_CTRL   patactrl    = LW_NULL;
    __PATA_DRIVE  patadrive   = LW_NULL;
    __PATA_TYPE   patatype    = LW_NULL;

    INT           iRetry      = 1;
    INT           iRetryCount = 0;
    ULONG         ulSemStatus = 0;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl    = patactrler;
    patadrive   = &patactrl->ATACTRL_ataDrive[iDrive];
    patatype    = patadrive->ATADRIVE_patatypeDriverInfo;

    while (iRetry) {
        if (__ataWait(patactrler, __ATA_STAT_READY)) {                  /*  ��ʱ,���󷵻�               */
            ATA_DEBUG_MSG(("__ataCmd() error : time out"));
            return  (PX_ERROR);
        }

        switch (iCmd) {

        case __ATA_CMD_DIAGNOSE:                                        /*  �������������              */
        case __ATA_CMD_RECALIB:                                         /*  У׼                        */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),
                               (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
            break;

        case __ATA_CMD_INITP:                                           /*  ������������ʼ��            */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLLO(patactrl), (UINT8)patatype->ATATYPE_iCylinders);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLHI(patactrl),  \
                               (UINT8)(patatype->ATATYPE_iCylinders >> 8));
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), (UINT8)patatype->ATATYPE_iSectors);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),    \
                               (UINT8)(__ATA_SDH_LBA         |   \
                               (iDrive << __ATA_DRIVE_BIT)   |   \
                               ((patatype->ATATYPE_iHeads - 1) & 0x0f)));
            break;

        case __ATA_CMD_SEEK:                                            /*  ��ѯ��λ                    */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLLO(patactrl), (UINT8)iArg0);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLHI(patactrl), (UINT8)(iArg0 >> 8));
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                               (UINT8)(__ATA_SDH_LBA         |  \
                               (iDrive << __ATA_DRIVE_BIT)   |  \
                               (iArg1 & 0xf)));
            break;

        case __ATA_CMD_SET_FEATURE:                                     /*  ��������                    */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), (UINT8)iArg1);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_FEATURE(patactrl), (UINT8)iArg0);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                               (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
            break;

        case __ATA_CMD_SET_MULTI:                                       /*  ����ģʽ�趨                */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), (UINT8)iArg0);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                               (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
            break;

        default:
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                               (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
            break;
        }

        __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), (UINT8)iCmd);
                                                                        /*  д����Ĵ���                */
        if (patactrl->ATACTRL_bIntDisable == LW_FALSE) {
            ulSemStatus = API_SemaphoreBPend(patactrl->ATACTRL_ulSyncSem,  \
                                patactrl->ATACTRL_ulSyncSemTimeout);    /*  �ȴ�ͬ���ź�                */
        }

        if ((patactrl->ATACTRL_iIntStatus & __ATA_STAT_ERR) || (ulSemStatus != ERROR_NONE)) {
            ATA_DEBUG_MSG(("__ataCmd() error : status=0x%x ulSemStatus=%d err=0x%x\n",
                           patactrl->ATACTRL_iIntStatus, ulSemStatus,
                           __ATA_CTRL_INBYTE(patactrl, __ATA_ERROR(patactrl))));
            if (++iRetryCount > _G_iAtaRetry) {                         /*  ���Դ���Ĭ�ϴ���,���󷵻�   */
                return  (PX_ERROR);
            }
        } else {
            iRetry = 0;
        }
    }

    if (iCmd == __ATA_CMD_SEEK) {
        if (__ataWait(patactrler, __ATA_STAT_SEEKCMPLT) != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    ATA_DEBUG_MSG(("__ataCmd() end : - iCtrl %d, iDrive %d: Ok\n", patactrl->ATACTRL_iCtrl, iDrive));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ataWait
** ��������: �ȴ�ATA�豸׼����
** �䡡��  : patactrler  ATA�������ṹָ��
**           iRequest    �ȴ�״̬
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __ataWait (__PATA_CTRL patactrler, INT iRequest)
{
    __PATA_CTRL       patactrl = LW_NULL;
    struct timespec   tvOld;
    struct timespec   tvNow;

    volatile INT  i;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = patactrler;
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);

    switch (iRequest) {

    case __ATA_STAT_READY:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if ((__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_BUSY) == 0) {
                                                                        /*  �ȴ��豸��æ                */
                break;
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  ��ʱ�˳�                    */
                break;
            }
        }

        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if (__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_READY) {
                                                                        /*  �豸׼����                  */
                return  (0);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  ��ʱ�˳�                    */
                break;
            }
        }
        break;

    case __ATA_STAT_BUSY:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if ((__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_BUSY) == 0) {
                                                                        /*  �ȴ��豸��æ                */
                return  (ERROR_NONE);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  ��ʱ�˳�                    */
                break;
            }
        }
        break;

    case __ATA_STAT_DRQ:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if (__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_DRQ) {
                                                                        /*  �豸׼���ô�������          */
                return  (ERROR_NONE);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  ��ʱ�˳�                    */
                break;
            }
        }
        break;

    case __ATA_STAT_SEEKCMPLT:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if (__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_SEEKCMPLT) {
                                                                        /*  �豸����                    */
                return  (ERROR_NONE);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  ��ʱ�˳�                    */
                break;
            }
        }
        break;

    case __ATA_STAT_IDLE:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if ((__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) &  \
                (__ATA_STAT_BUSY | __ATA_STAT_DRQ)) == 0) {             /*  �豸����                    */
                return  (ERROR_NONE);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  ��ʱ�˳�                    */
                break;
            }
        }
        break;

    default:
        break;
    }

    return  (PX_ERROR);                                                 /*  ���󷵻�                    */
}
/*********************************************************************************************************
** ��������: __ataPread
** ��������: ��ȡ�豸�Ĳ���
** �䡡��  : patactrler  ATA�������ṹָ��
**                       iDrive    ������
**                       pvBuffer  ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __ataPread (__PATA_CTRL patactrler,
                       INT         iDrive,
                       PVOID       pvBuffer)
{
    __PATA_CTRL patactrl = LW_NULL;

    INT         iRetry      = 1;
    INT         iRetryCount = 0;
    ULONG       ulSemStatus = 0;

#if LW_CFG_CPU_ENDIAN == 1
    INT         i;
#endif

    INT16      *psBuf = LW_NULL;

    if ((!patactrler) || (!pvBuffer)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = patactrler;

    while (iRetry) {
        if (__ataWait(patactrl, __ATA_STAT_READY) != ERROR_NONE) {
            return  (PX_ERROR);
        }

        __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                           (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
        __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_READP);
                                                                        /*  д��ȷ������                */
        if (patactrl->ATACTRL_bIntDisable == LW_FALSE) {
            ulSemStatus = API_SemaphoreBPend(patactrl->ATACTRL_ulSyncSem,   \
                                patactrl->ATACTRL_ulSyncSemTimeout);    /*  �ȴ�ͬ���ź�                */
        }

        if ((patactrl->ATACTRL_iIntStatus & __ATA_STAT_ERR) || (ulSemStatus != ERROR_NONE)) {
            ATA_DEBUG_MSG(("__ataPread() error : status=0x%x intStatus=0x%x "     \
                           "error=0x%x ulSemStatus=%d\n",                         \
                           __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)),  \
                           patactrl->ATACTRL_iIntStatus,                          \
                           __ATA_CTRL_INBYTE(patactrl, __ATA_ERROR(patactrl)), ulSemStatus));
            if (++iRetryCount > _G_iAtaRetry) {
                return  (PX_ERROR);
            }
        
        } else {
            iRetry = 0;
        }
    }

    if (__ataWait(patactrl, __ATA_STAT_DRQ) != ERROR_NONE) {            /*  �ȴ��豸׼���ô�������      */
        return  (PX_ERROR);
    }

    psBuf = (INT16 *)pvBuffer;
    __ATA_CTRL_INSTRING(patactrl, __ATA_DATA(patactrl), psBuf, 256);

#if LW_CFG_CPU_ENDIAN == 1                                              /*  big-endian                  */
#define __ATA_DATA_MSB(x)   (((x) >> 8) & 0xff)
#define __ATA_DATA_LSB(x)   ((x) & 0xff)
#define __ATA_DATA_SWAP(x)  ((__ATA_DATA_LSB(x) << 8) | __ATA_DATA_MSB(x))
    
    if (patactrl->ATACTRL_bPreadBeSwap) {
        for (i = 0; i < 256; i++) {
            psBuf[i] = (INT16)__ATA_DATA_SWAP(psBuf[i]);
        }
    }
#endif

    ATA_DEBUG_MSG(("__ataPread() end\n"));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ataDriveInit
** ��������: ��ʼ��ATA�豸
** �䡡��  : patactrler  ATA�������ṹָ��
**           iDrive      ��������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __ataDriveInit (__PATA_CTRL patactrler, INT iDrive)
{
    __ATA_CTRL   *patactrl       = LW_NULL;
    __ATA_DRIVE  *patadrive      = LW_NULL;
    __ATA_PARAM  *pataparam      = LW_NULL;
    __ATA_TYPE   *patatype       = LW_NULL;

    INT           iConfigType    = 0;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ATA_DEBUG_MSG(("Enter into __ataiDriveInit()\n"));

    patactrl    = patactrler;
    patadrive   = &patactrl->ATACTRL_ataDrive[iDrive];
    pataparam   = &patadrive->ATADRIVE_ataParam;
    patatype    = patadrive->ATADRIVE_patatypeDriverInfo;
    iConfigType = patactrl->ATACTRL_iConfigType;

    if (__ataDevIdentify(patactrler, iDrive) != ERROR_NONE) {           /*  ȷ���豸�Ƿ�����Լ��豸����*/
        patadrive->ATADRIVE_ucState = __ATA_DEV_NONE;
        goto    __error_handle;
    }

    if (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATA) {                 /*  �豸ΪATA�����豸           */
        /*
         *  �ó��豸�洢�ļ��νṹ
         */
        if ((iConfigType & __ATA_GEO_MASK) == ATA_GEO_FORCE) {
            __ataCmd(patactrl, iDrive, __ATA_CMD_INITP, 0, 0);
            __ataPread(patactrl, iDrive, (INT8 *)pataparam);
        
        } else if ((iConfigType & __ATA_GEO_MASK) == ATA_GEO_PHYSICAL) {
            patatype->ATATYPE_iCylinders = pataparam->ATAPAR_sCylinders;
            patatype->ATATYPE_iHeads     = pataparam->ATAPAR_sHeads;
            patatype->ATATYPE_iSectors   = pataparam->ATAPAR_sSectors;
        
        } else if ((iConfigType & __ATA_GEO_MASK) == ATA_GEO_CURRENT) {
            if ((pataparam->ATAPAR_sCurrentCylinders != 0) &&
                (pataparam->ATAPAR_sCurrentHeads     != 0) &&
                (pataparam->ATAPAR_sCurrentSectors   != 0)) {
                patatype->ATATYPE_iCylinders = pataparam->ATAPAR_sCurrentCylinders;
                patatype->ATATYPE_iHeads     = pataparam->ATAPAR_sCurrentHeads;
                patatype->ATATYPE_iSectors   = pataparam->ATAPAR_sCurrentSectors;
            
            } else {
                patatype->ATATYPE_iCylinders = pataparam->ATAPAR_sCylinders;
                patatype->ATATYPE_iHeads     = pataparam->ATAPAR_sHeads;
                patatype->ATATYPE_iSectors   = pataparam->ATAPAR_sSectors;
            }
        }

        patatype->ATATYPE_iBytes = patactrl->ATACTRL_iBytesPerSector;   /*  ÿ�����Ĵ�С                */

        if (pataparam->ATAPAR_sCapabilities & __ATA_IOLBA_MASK) {       /*  �豸֧��LBAģʽ,������������*/
            patadrive->ATADRIVE_uiCapacity =
            (UINT)((((UINT)((pataparam->ATAPAR_usSectors0) & 0x0000ffff)) << 0) |
            (((UINT)((pataparam->ATAPAR_usSectors1) & 0x0000ffff)) << 16));

            if (patadrive->ATADRIVE_uiCapacity == __ATA_MAX_28LBA) {
                patadrive->ATADRIVE_uiCapacity = ((INT64)pataparam->ATAPAR_sMaxLBA[0]) |
                                                 ((INT64)pataparam->ATAPAR_sMaxLBA[1] << 16) |
                                                 ((INT64)pataparam->ATAPAR_sMaxLBA[2] << 32) |
                                                 ((INT64)pataparam->ATAPAR_sMaxLBA[3] << 48);

            }

            if (patadrive->ATADRIVE_uiCapacity > __ATA_MAX_48LBA) {
                return  (PX_ERROR);
            }

            ATA_DEBUG_MSG(("ID_iDrive reports LBA (60-61) as 0x%x\n",
                           patadrive->ATADRIVE_uiCapacity));
        }
    } else if (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATAPI) {
        /*
         *  TODO: ��ATAPI�����豸��֧��
         */
        return  (PX_ERROR);
    
    } else {
        ATA_DEBUG_MSG(("__ataDriveInit() error: Unknow ata drive type!\r\n"));
        return  (PX_ERROR);
    }
    
    /*
     *  �õ��豸��֧�ֵ�����
     */
    patadrive->ATADRIVE_sMultiSecs = (INT16)(pataparam->ATAPAR_sMultiSecs & __ATA_MULTISEC_MASK);
    patadrive->ATADRIVE_sOkMulti   = (INT16)((patadrive->ATADRIVE_sMultiSecs != 0) ? 1 : 0);
    patadrive->ATADRIVE_sOkIordy   = (INT16)((pataparam->ATAPAR_sCapabilities &   \
                                     __ATA_IORDY_MASK) ? 1 : 0);
    patadrive->ATADRIVE_sOkLba     = (INT16)((pataparam->ATAPAR_sCapabilities &   \
                                     __ATA_IOLBA_MASK) ? 1 : 0);
    patadrive->ATADRIVE_sOkDma     = (INT16)((pataparam->ATAPAR_sCapabilities &   \
                                     __ATA_DMA_CAP_MASK) ? 1 : 0);
    /*
     *  �õ��豸��֧�ֵ����PIOģʽ
     */
    patadrive->ATADRIVE_sPioMode = (INT16)((pataparam->ATAPAR_sPioMode >> 8) & __ATA_PIO_MASK_012);
                                                                        /*  PIO 0,1,2                   */
    if (patadrive->ATADRIVE_sPioMode > __ATA_SET_PIO_MODE_2) {
        patadrive->ATADRIVE_sPioMode = __ATA_SET_PIO_MODE_0;
    }

    if ((patadrive->ATADRIVE_sOkIordy) && (pataparam->ATAPAR_sValid & __ATA_PIO_MASK_34)) {
                                                                        /*  PIO 3,4                     */
        if (pataparam->ATAPAR_sAdvancedPio & __ATA_BIT_MASK0) {
            patadrive->ATADRIVE_sPioMode = __ATA_SET_PIO_MODE_3;
        }

        if (pataparam->ATAPAR_sAdvancedPio & __ATA_BIT_MASK1) {
            patadrive->ATADRIVE_sPioMode = __ATA_SET_PIO_MODE_4;
        }
    }
    /*
     *  �õ��豸��֧�ֵ�DMAģʽ
     */
    if ((patadrive->ATADRIVE_sRwDma) && (pataparam->ATAPAR_sValid & __ATA_WORD64_70_VALID)) {
        /*
         *  TODO: ��ATA�豸DMAģʽ��֧��, ĿǰΪ�ղ���
         */
    }
    /*
     *  �õ���ʹ�õĴ���ģʽ
     */
    patadrive->ATADRIVE_sRwBits = (INT16)(iConfigType & __ATA_BITS_MASK);
    patadrive->ATADRIVE_sRwPio  = (INT16)(iConfigType & __ATA_PIO_MASK);
    patadrive->ATADRIVE_sRwDma  = (INT16)(iConfigType & __ATA_DMA_MASK);
    patadrive->ATADRIVE_sRwMode = ATA_PIO_DEF_0;                      /*  Ĭ�ϵ�PIOģʽ               */
    /*
     *  �豸����������дģʽ
     */
    if ((patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) &&
        (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATA)) {
        if (patadrive->ATADRIVE_sOkMulti) {
            (VOID)__ataCmd(patactrler, iDrive, __ATA_CMD_SET_MULTI,
                           patadrive->ATADRIVE_sMultiSecs, 0);
        
        } else {
            patadrive->ATADRIVE_sRwPio = ATA_PIO_SINGLE;
        }
    }

    switch (iConfigType & __ATA_MODE_MASK) {

    case ATA_PIO_DEF_0:
    case ATA_PIO_DEF_1:
    case ATA_PIO_0:
    case ATA_PIO_1:
    case ATA_PIO_2:
    case ATA_PIO_3:
    case ATA_PIO_4:
        patadrive->ATADRIVE_sRwMode = (INT16)(iConfigType & __ATA_MODE_MASK);
        break;

    case ATA_PIO_AUTO:
        patadrive->ATADRIVE_sRwMode = (INT16)(ATA_PIO_DEF_0 + patadrive->ATADRIVE_sPioMode);
        break;

    case ATA_DMA_0:
    case ATA_DMA_1:
    case ATA_DMA_2:
    case ATA_DMA_AUTO:
        /*
         *  TODO:��ATA�豸DMA���䷽ʽ��֧��
         */
        break;

    default:
        break;
    }
    
    /*
     *  ���ô���ģʽ
     */
    (VOID)__ataCmd(patactrler, iDrive, __ATA_CMD_SET_FEATURE, __ATA_SUB_SET_RWMODE,
                   patadrive->ATADRIVE_sRwMode);

    patadrive->ATADRIVE_ucState = __ATA_DEV_OK;

__error_handle:
    if (patadrive->ATADRIVE_ucState != __ATA_DEV_OK) {
        ATA_DEBUG_MSG(("__ataiDriveInit() %d/%d: ERROR: state=%d iDev=0x%x "
                       "status=0x%x error=0x%x\n", patactrl->ATACTRL_iCtrl, iDrive,
                       patadrive->ATADRIVE_ucState,
                       __ATA_CTRL_INBYTE(patactrl, __ATA_SDH(patactrl)),
                       __ATA_CTRL_INBYTE(patactrl, __ATA_STATUS(patactrl)),
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ERROR(patactrl))));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ataDevIdentify
** ��������: ȷ��ATA�豸
** �䡡��  : patactrler  ATA�������ṹָ��
**           iDrive      ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __ataDevIdentify (__PATA_CTRL patactrler, INT iDrive)
{
    __ATA_CTRL    *patactrl  = LW_NULL;
    __ATA_DRIVE   *patadrive = LW_NULL;
    __ATA_PARAM   *pataparam = LW_NULL;

    INT            istatus   = 0;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ATA_DEBUG_MSG(("Enter into __ataDevIdentify()\n"));

    patactrl  = patactrler;
    patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
    pataparam = &patadrive->ATADRIVE_ataParam;

    patadrive->ATADRIVE_ucType = __ATA_TYPE_NONE;

    istatus = __ataWait(patactrl, __ATA_STAT_IDLE);
    if (istatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataDevIdentify() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                       (UINT8)(__ATA_SDH_LBA |          \
                       (iDrive << __ATA_DRIVE_BIT)));                   /*  ѡ���豸                    */

    istatus = __ataWait(patactrl, __ATA_STAT_BUSY);
    if (istatus != ERROR_NONE) {                                        /*  �ȴ��豸ѡ�����            */
        ATA_DEBUG_MSG(("__ataDevIdentify() %d/%d: status = 0x%x read timed out\n",
                      patactrl->ATACTRL_iCtrl, iDrive, 
                      __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DCONTROL(patactrl), (UINT8)(patactrl->ATACTRL_bIntDisable << 1));
                                                                        /*  �Ƿ�ʹ���ж�                */
    istatus = __ataWait(patactrl, __ATA_STAT_IDLE);
    if (istatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataDevIdentify() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), 0x23);
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECTOR(patactrl), 0x55);

    if (__ATA_CTRL_INBYTE(patactrl, __ATA_SECCNT(patactrl)) == 0x23) {
        /*
         *  ��ȷ���豸����, Ȼ���ҳ��豸����, uiSignature �� __ataCtrlReset() ����ȷ��
         */
        patactrl->ATACTRL_bIsExist = LW_TRUE;

        if (patadrive->ATADRIVE_uiSignature == __ATAPI_SIGNATURE) {
            patadrive->ATADRIVE_ucType = __ATA_TYPE_ATAPI;

        } else if (patadrive->ATADRIVE_uiSignature == __ATA_SIGNATURE) {
            patadrive->ATADRIVE_ucType = __ATA_TYPE_ATA;

        } else {
            patadrive->ATADRIVE_ucType      = __ATA_TYPE_NONE;
            patadrive->ATADRIVE_uiSignature = 0;
            ATA_DEBUG_MSG(("__ataDeviceIdentify(): Unknown device found on %d/%d\n",
                           patactrl->ATACTRL_iCtrl, iDrive));
        }

        if (patadrive->ATADRIVE_ucType != __ATA_TYPE_NONE) {
            istatus = __ataPread(patactrler, iDrive, pataparam);
            if (istatus != ERROR_NONE) {
                return  (PX_ERROR);
            }
        }

    } else {                                                            /*  �豸������                  */
        patadrive->ATADRIVE_ucType      = __ATA_TYPE_NONE;
        patadrive->ATADRIVE_uiSignature = 0xffffffff;

        ATA_DEBUG_MSG(("__ataDeviceIdentify() error: Unknown device found on %d/%d\n",
                       patactrl->ATACTRL_iCtrl, iDrive));

        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ataCtrlReset
** ��������: ��λATA������
** �䡡��  : patactrler    �������ṹָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __ataCtrlReset (__PATA_CTRL patactrler)
{
    __PATA_CTRL    patactrl  = LW_NULL;
    __PATA_DRIVE   patadrive = LW_NULL;

    INT            iDrive;
    volatile INT   i;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = patactrler;

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DCONTROL(patactrl), __ATA_CTL_RST | __ATA_CTL_IDS);
    __ATA_DELAYMS(1);

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DCONTROL(patactrl),  \
                       (UINT8)(patactrl->ATACTRL_bIntDisable << 1));    /*  �����λ,�������ж�λ       */
    __ATA_DELAYMS(1);

    i = 0;
    while (i < 3) {
        i++;
        if (__ataWait(patactrl, __ATA_STAT_BUSY) == ERROR_NONE) {
            break;
        }
    }

    for (iDrive = 0; iDrive < patactrl->ATACTRL_iDrives; iDrive++) {
        patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
        __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl), (iDrive << __ATA_DRIVE_BIT));

        patadrive->ATADRIVE_uiSignature =                                         \
                    (__ATA_CTRL_INBYTE(patactrl, __ATA_SECCNT(patactrl)) << 24) | \
                    (__ATA_CTRL_INBYTE(patactrl, __ATA_SECTOR(patactrl)) << 16) | \
                    (__ATA_CTRL_INBYTE(patactrl, __ATA_CYLLO(patactrl))  << 8)  | \
                    (__ATA_CTRL_INBYTE(patactrl, __ATA_CYLHI(patactrl)));
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
