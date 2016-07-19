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
** ��   ��   ��: ahci.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 01 �� 04 ��
**
** ��        ��: AHCI ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "linux/compat.h"
#include "ahci.h"
#include "ahciLib.h"
#include "ahciPort.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciCtrl.h"
#include "ahciPm.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT32   _GuiAhciBitMask[AHCI_DRIVE_MAX] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
};
static INT  _GiAhciConfigType[AHCI_DRIVE_MAX] = {
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL
};
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static PVOID            __ahciMonitorThread(PVOID pvArg);
static irqreturn_t      __ahciIsr(PVOID pvArg, ULONG ulVector);
static INT              __ahciDiskCtrlInit(AHCI_CTRL_HANDLE hCtrl, INT iDrive);
/*********************************************************************************************************
** ��������: __ahciCmdWaitForResource
** ��������: �ȴ���Դ
** �䡡��  : hDrive     ���������
**           bQueued    �Ƿ�ʹ�ܶ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ahciCmdWaitForResource (AHCI_DRIVE_HANDLE  hDrive, BOOL  bQueued)
{
             INTREG iregInterLevel;
    REGISTER INT    i = 0;                                              /* ѭ������                     */

    if (hDrive->AHCIDRIVE_bNcq == LW_FALSE) {                           /* �� NCQ ģʽ                  */
        API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
    
    } else {                                                            /* NCQ ģʽ                     */
        if (bQueued) {                                                  /* ����ģʽ                     */
            API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem, LW_OPTION_WAIT_INFINITE);
                                                                        /* �ȴ�����Ȩ                   */
            LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel);
            if (hDrive->AHCIDRIVE_bQueued == LW_FALSE) {                /* �Ƕ���ģʽ                   */
                hDrive->AHCIDRIVE_bQueued = LW_TRUE;                    /* ����ģʽ                     */
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                                                                        /* �Ƿ����Ȩ                   */
                                                                        /* �ͷŶ��вۿ���Ȩ             */
                for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth - 1; i++) {
                    API_SemaphoreCPost(hDrive->AHCIDRIVE_hQueueSlotCSem);
                }
            } else {                                                    /* ����ģʽ                     */
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                                                                        /* �Ƿ����Ȩ                   */
            }
        
        } else {                                                        /* �Ƕ���ģʽ                   */
            API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
            
            if (hDrive->AHCIDRIVE_bQueued == LW_TRUE) {                 /* ����ģʽ                     */
                for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth; i++) {  /* �ͷŶ�����Դ                 */
                    API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem, LW_OPTION_WAIT_INFINITE);
                }
                hDrive->AHCIDRIVE_bQueued = LW_FALSE;                   /* ���Ϊ�Ƕ���ģʽ             */
            
            } else {                                                    /* �Ƕ���ģʽ                   */
                API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem, LW_OPTION_WAIT_INFINITE);
            }
        }
    }
}
/*********************************************************************************************************
** ��������: __ahciCmdReleaseResource
** ��������: �ͷ���Դ
** �䡡��  : hDrive     ���������
**           bQueued    �Ƿ�ʹ�ܶ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ahciCmdReleaseResource (AHCI_DRIVE_HANDLE  hDrive, BOOL  bQueued)
{
    if (hDrive->AHCIDRIVE_bNcq == LW_FALSE) {                           /* �Ƕ���ģʽ                   */
        API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
    
    } else {                                                            /* ����ģʽ                     */
        API_SemaphoreCPost(hDrive->AHCIDRIVE_hQueueSlotCSem);
        
        if (bQueued == LW_FALSE) {                                      /* �Ƕ���ģʽ                   */
            API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
        }
    }
}
/*********************************************************************************************************
** ��������: __ahciPrdtSetup
** ��������: ���� Physical Region Descriptor Table (PRDT)
** �䡡��  : pcDataBuf      ���ݻ�����
**           ulLen          ���ݳ���
**           hPrdtHandle    PRDT ���ƿ���
** �䡡��  : �����ֽڳ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static size_t  __ahciPrdtSetup (UINT8 *pcDataBuf, ULONG  ulLen, AHCI_PRDT_HANDLE  hPrdtHandle)
{
    size_t      stPrdtCount = 0;                                        /* PRD ����                     */
    ULONG       ulByteCount = 0;                                        /* ���ݳ��ȼ���                 */
    ULONG       ulSize      = 0;                                        /* ���ݳ���                     */
    PVOID       pvAddr      = LW_NULL;                                  /* ���ݻ�����                   */

    AHCI_CMD_LOG(AHCI_LOG_PRT, "buff 0x%x len %ld.", pcDataBuf, ulLen);

    pvAddr = (PVOID)pcDataBuf;
    if ((ULONG)pvAddr & 1) {                                            /* ��ַ�������                 */
        AHCI_CMD_LOG(AHCI_LOG_ERR, "dma buffer not word aligned.", 0);
        return  ((size_t)PX_ERROR);
    }

    stPrdtCount = 0;
    ulSize = ulLen;
    while (ulSize) {                                                    /* ���� PRDT                    */
        if (++stPrdtCount > AHCI_PRDT_MAX) {                            /* PRDT ����������              */
            AHCI_CMD_LOG(AHCI_LOG_ERR, "dma table too small.", 0);
            return  ((size_t)PX_ERROR);
        
        } else {                                                        /* �п��õ� PRDT ��             */
            if (ulSize > AHCI_PRDT_BYTE_MAX) {                          /* ���ݴ�С����                 */
                ulByteCount = AHCI_PRDT_BYTE_MAX;
            } else {                                                    /* ���ݴ�Сδ����               */
                ulByteCount = ulSize;
            }

            /*
             *  ���� PRDT ��ַ��Ϣ
             */
            hPrdtHandle->AHCIPRDT_uiDataBaseAddrLow  = AHCI_SWAP(AHCI_ADDR_LOW32(pvAddr));
            hPrdtHandle->AHCIPRDT_uiDataBaseAddrHigh = AHCI_SWAP(AHCI_ADDR_HIGH32(pvAddr));

            /*
             *  ���µ�ַ���С����Ϣ
             */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "table addr %p byte count %ld.", pvAddr, ulByteCount);
            pvAddr  = (PVOID)((ULONG)pvAddr + ulByteCount);
            ulSize -= ulByteCount;

            ulByteCount--;
            if (ulSize == 0) {
                ulByteCount |= AHCI_PRDT_I;
            }

            hPrdtHandle->AHCIPRDT_uiInterruptDataByteCount = AHCI_SWAP(ulByteCount);
            AHCI_CMD_LOG(AHCI_LOG_PRT, "table count : 0x%x.", ulByteCount);
            hPrdtHandle++;
        }
    }

    API_CacheDmaFlush(hPrdtHandle, stPrdtCount * sizeof(AHCI_PRDT_CB)); /* ��д PRDT ������Ϣ           */

    return  (stPrdtCount);
}
/*********************************************************************************************************
** ��������: __ahciDiskCommandSend
** ��������: �������������������
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           hCmd       ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskCommandSend (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, AHCI_CMD_HANDLE  hCmd)
{
    ULONG                   ulRet            = PX_ERROR;                /* �������                     */
    AHCI_DRIVE_HANDLE       hDrive           = LW_NULL;                 /* ���������                   */
    AHCI_PRDT_HANDLE        hPrdt            = LW_NULL;                 /* PRDT ���                    */
    UINT8                  *pucCommandFis    = LW_NULL;                 /* ����ṹ����                 */
    UINT8                  *pucPktCmd        = LW_NULL;                 /* �����                     */
    AHCI_CMD_LIST_HANDLE    hCommandList     = LW_NULL;                 /* �������о��                 */
    UINT32                  uiFlagsPrdLength = 0;                       /* PRDT ��־                    */
    size_t                  stPrdtCount      = 0;                       /* PRDT ������Ϣ                */
    INTREG                  iregInterLevel   = 0;                       /* �жϼĴ���                   */
    INT                     iTag             = 0;                       /* �����Ϣ                     */
    ULONG                   ulWait           = 0;                       /* ��ʱ����                     */
    AHCI_MSG_CB             tCtrlMsg;                                   /* ��Ϣ���ƿ�                   */
    AHCI_MSG_HANDLE         hCtrlMsg         = LW_NULL;                 /* ��Ϣ���                     */
    UINT32                  uiTagBit         = 0;                       /* ���λ��Ϣ                   */
    UINT32                  uiTemp           = 0;                       /* ��ʱ����                     */
    BOOL                    bQueued          = LW_FALSE;                /* ����ģʽ                     */

    UINT16                  usCylinder       = 0;                       /* ����                         */
    UINT16                  usHead           = 0;                       /* ��ͷ                         */
    UINT16                  usSector         = 0;                       /* ����                         */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ��ȡ���������               */
    hCtrlMsg = &tCtrlMsg;
    bQueued = (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ);               /* �Ƿ�ʹ�ܶ���ģʽ             */
    uiTemp = hCmd->AHCICMD_iFlags & (AHCI_CMD_FLAG_SRST_ON | AHCI_CMD_FLAG_SRST_OFF);
    if (uiTemp == 0) {                                                  /* �ǿ�������                   */
        __ahciCmdWaitForResource(hDrive, bQueued);                      /* �ȴ�����Ȩ                   */
    }

    if (bQueued) {                                                      /* ����ģʽʹ��                 */
        API_SemaphoreMPend(hDrive->AHCIDRIVE_hTagMuteSem, LW_OPTION_WAIT_INFINITE);

        do {                                                            /* ������Ч��־λ               */
            iTag = hDrive->AHCIDRIVE_iNextTag;
            hDrive->AHCIDRIVE_iNextTag =(hDrive->AHCIDRIVE_iNextTag + 1) % hDrive->AHCIDRIVE_uiQueueDepth;
        } while (hDrive->AHCIDRIVE_uiCmdStarted & (_GuiAhciBitMask[iTag]));

        uiTagBit = _GuiAhciBitMask[iTag];                               /* ��ȡ��־λ��Ϣ               */
        API_SemaphoreMPost(hDrive->AHCIDRIVE_hTagMuteSem);              /* �ͷſ���Ȩ                   */
    
    } else {                                                            /* �Ƕ���ģʽ                   */
        iTag     = 0;                                                   /* �������                     */
        uiTagBit = 1;                                                   /* ���λ��Ϣ                   */
    }

    /*
     *  ��ȡ PRDT ���ƿ����������п��ƿ�
     */
    hPrdt = &hDrive->AHCIDRIVE_hCmdTable[iTag].AHCICMDTABL_tPrdt[0];
    pucCommandFis = hDrive->AHCIDRIVE_hCmdTable[iTag].AHCICMDTABL_ucCommandFis;
    pucPktCmd = hDrive->AHCIDRIVE_hCmdTable[iTag].AHCICMDTABL_ucAtapiCommand;
    hCommandList = &hDrive->AHCIDRIVE_hCmdList[iTag];

    pucCommandFis[0] = 0x27;
    if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_SRST_ON) {                 /* ��λ������                   */
        AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd srst on.", 0);
        lib_bzero(&pucCommandFis[1], 19);
        pucCommandFis[15] = AHCI_CTL_4BIT | AHCI_CTL_RST;
        hCommandList->AHCICMDLIST_uiPrdtFlags =  AHCI_SWAP(AHCI_CMD_LIST_C | AHCI_CMD_LIST_R | 5);
        API_CacheDmaFlush(pucCommandFis, 20);
        AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
        return  (ERROR_NONE);
    
    } else if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_SRST_OFF) {         /* ��λ���������               */
        AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd srst off.", 0);
        lib_bzero(&pucCommandFis[1], 19);
        pucCommandFis[15] = AHCI_CTL_4BIT;
        hCommandList->AHCICMDLIST_uiPrdtFlags =  AHCI_SWAP(5);
        API_CacheDmaFlush(pucCommandFis, 20);
        AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
        return  (ERROR_NONE);
    
    } else {                                                            /* ��������                     */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI ����ģʽ               */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd flag atapi.", 0);
            pucCommandFis[1] = 0x80;
            pucCommandFis[2] = AHCI_PI_CMD_PKTCMD;
            pucCommandFis[3] = hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature;
            pucCommandFis[4] = 0;
            pucCommandFis[5] = (UINT8)hCmd->AHCICMD_ulDataLen;
            pucCommandFis[6] = (UINT8)(hCmd->AHCICMD_ulDataLen >> 8);
            pucCommandFis[7] = AHCI_SDH_LBA;
            lib_bzero(&pucCommandFis[8], 12);
            lib_memcpy(pucPktCmd, hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt,AHCI_ATAPI_CMD_LEN_MAX);
        
        } else {                                                        /* ATA ����ģʽ                 */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd flag ata.", 0);
            pucCommandFis[1] = 0x80;
            pucCommandFis[2] = hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand;
            
            if ((hDrive->AHCIDRIVE_bLba == LW_TRUE) ||
                (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NON_SEC_DATA)) {  /* LBA ģʽ������չ����         */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "lba true flag non sec data.", 0);
                pucCommandFis[4] = (UINT8)hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba;
                pucCommandFis[5] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 8);
                pucCommandFis[6] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 16);
                pucCommandFis[7] = AHCI_SDH_LBA;
                
                if (hDrive->AHCIDRIVE_bLba48 == LW_TRUE) {              /* LBA 48 ģʽ                  */
                    pucCommandFis[ 8] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 24);
                    pucCommandFis[ 9] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 32);
                    pucCommandFis[10] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 40);
                
                } else {                                                /* �� LBA 48 ģʽ               */
                    uiTemp = (UINT8)(pucCommandFis[7] |
                                     ((hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 24) & 0x0F));
                    pucCommandFis[ 7] = (UINT8)uiTemp;
                    pucCommandFis[ 8] = 0;
                    pucCommandFis[ 9] = 0;
                    pucCommandFis[10] = 0;
                }
            
            } else {                                                    /* �� LBA ģʽ                  */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "lba false flag sec data.", 0);
                usCylinder = (UINT16)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba /
                                      (hDrive->AHCIDRIVE_uiSector * hDrive->AHCIDRIVE_uiHead));
                usSector = (UINT16)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba %
                                    (hDrive->AHCIDRIVE_uiSector * hDrive->AHCIDRIVE_uiHead));
                usHead = usSector / hDrive->AHCIDRIVE_uiSector;
                usSector = usSector % hDrive->AHCIDRIVE_uiSector + 1;

                pucCommandFis[ 4] = (UINT8)usSector;
                pucCommandFis[ 5] = (UINT8)(usCylinder >> 8);
                pucCommandFis[ 6] = (UINT8)usCylinder;
                pucCommandFis[ 7] = (UINT8)(AHCI_SDH_IBM | (usHead & 0x0F));
                pucCommandFis[ 8] = 0;
                pucCommandFis[ 9] = 0;
                pucCommandFis[10] = 0;
            }

            if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ) {             /* ʹ�� NCQ                     */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "flag ncq.", 0);
                pucCommandFis[ 3] = (UINT8)hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount;
                pucCommandFis[11] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount >> 8);
                pucCommandFis[12] = (UINT8)(iTag << 3);
                pucCommandFis[13] = 0;
                pucCommandFis[ 7] = 0x40;
            
            } else {                                                    /* ���� NCQ                     */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "flag no ncq.", 0);
                pucCommandFis[ 3] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature);
                pucCommandFis[11] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature >> 8);
                pucCommandFis[12] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount);
                pucCommandFis[13] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount >> 8);
            }
            
            pucCommandFis[14] = 0;
            pucCommandFis[15] = AHCI_CTL_4BIT;
            pucCommandFis[16] = 0;
            pucCommandFis[17] = 0;
            pucCommandFis[18] = 0;
            pucCommandFis[19] = 0;
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x",
                     pucCommandFis[0], pucCommandFis[1],pucCommandFis[2], pucCommandFis[3],
                     pucCommandFis[4], pucCommandFis[5],pucCommandFis[6], pucCommandFis[7]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x",
                     pucCommandFis[ 8], pucCommandFis[ 9], pucCommandFis[10], pucCommandFis[11],
                     pucCommandFis[12], pucCommandFis[13], pucCommandFis[14], pucCommandFis[15]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x",
                     pucCommandFis[16], pucCommandFis[17], pucCommandFis[18], pucCommandFis[19],
                     pucCommandFis[20], pucCommandFis[21], pucCommandFis[22], pucCommandFis[23]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x",
                     pucCommandFis[24], pucCommandFis[25], pucCommandFis[26], pucCommandFis[27],
                     pucCommandFis[28], pucCommandFis[29], pucCommandFis[30], pucCommandFis[31]);

        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_NONE) {           /* ��Ч����                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "data dir none.", 0);
            uiFlagsPrdLength = 5;
        
        } else {                                                        /* ��Ч����                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT,
                         "data dir %s.", hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_IN ? "read" : "write");
                                                                        /* ���� PRDT                    */
            stPrdtCount = __ahciPrdtSetup(hCmd->AHCICMD_pucDataBuf, hCmd->AHCICMD_ulDataLen, hPrdt);
            if (stPrdtCount == (size_t)PX_ERROR) {                      /* ����ʧ��                     */
                __ahciCmdReleaseResource(hDrive, bQueued);              /* �Ƿ����Ȩ                   */
                return  (PX_ERROR);
            }
                                                                        /* ���ñ�־                     */
            uiFlagsPrdLength = (UINT32)(stPrdtCount << AHCI_CMD_LIST_PRDTL_SHFT) | 5;
            if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_OUT) {        /* ���ģʽ                     */
                uiFlagsPrdLength |= AHCI_CMD_LIST_W;                    /* д������־                   */
            }
            if ((hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ) == 0) {      /* �� NCQ ģʽ                  */
                uiFlagsPrdLength |= AHCI_CMD_LIST_P;                    /* ���� NCQ                     */
            }
        }
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI ģʽ                   */
            uiFlagsPrdLength |= (AHCI_CMD_LIST_A | AHCI_CMD_LIST_P);
        }
        hCommandList->AHCICMDLIST_uiPrdtFlags = AHCI_SWAP(uiFlagsPrdLength);
        hCommandList->AHCICMDLIST_uiByteCount = 0;

        API_CacheDmaFlush(hCommandList, sizeof(hCommandList));          /* ��д�����б���Ϣ             */
        API_CacheDmaFlush(pucCommandFis, 20);                           /* ��д������Ϣ                 */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI ģʽ                   */
            API_CacheDmaFlush(pucPktCmd, AHCI_ATAPI_CMD_LEN_MAX);       /* ��д������Ϣ                 */
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "flag prdt length 0x%08x.", hCommandList->AHCICMDLIST_uiPrdtFlags);
        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_OUT) {            /* ���ģʽ                     */
                                                                        /* ��д������                   */
            API_CacheDmaFlush(hCmd->AHCICMD_pucDataBuf, (size_t)hCmd->AHCICMD_ulDataLen);
        }

        LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel); /* �ȴ�����Ȩ                   */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ) {                 /* NCQ ģʽ                     */
            AHCI_PORT_WRITE(hDrive, AHCI_PxSACT, uiTagBit);
            AHCI_PORT_READ(hDrive, AHCI_PxSACT);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
            AHCI_PORT_READ(hDrive, AHCI_PxCI);
            hDrive->AHCIDRIVE_uiCmdStarted |= uiTagBit;
        
        } else {                                                        /* �� NCQ ģʽ                  */
            AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
            AHCI_PORT_READ(hDrive, AHCI_PxCI);
            hDrive->AHCIDRIVE_uiCmdStarted |= uiTagBit;
        }
        LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);/* �Ƿ����Ȩ                   */

        AHCI_CMD_LOG(AHCI_LOG_PRT, "start command.", 0);
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_WAIT_SPINUP) {         /* ���³�ʱʱ�����             */
            ulWait = API_TimeGetFrequency() * 20;
        } else {                                                        /* ʹ�ó�ʼ��ʱʱ�����         */
            ulWait = hCtrl->AHCICTRL_ulSemTimeout;
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "sem take tag 0x%08x wait 0x%08x.", iTag, ulWait);
                                                                        /* �ȴ��������                 */
        ulRet = API_SemaphoreBPend(hDrive->AHCIDRIVE_hSyncBSem[iTag], ulWait);
        if (ulRet != ERROR_NONE) {                                      /* ����ʧ��                     */
            AHCI_CMD_LOG(AHCI_LOG_ERR,
                         "sync sem timeout ata cmd %02x ctrl %d drive %d.",
                         hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand, hCtrl->AHCICTRL_uiIndex, uiDrive);
            /*
             *  ���ʹ�����Ϣ�Ա����߳̽��д���
             */
            hDrive->AHCIDRIVE_uiTimeoutErrorCount++;
            hCtrlMsg->AHCIMSG_uiMsgId = AHCI_MSG_ERROR;
            hCtrlMsg->AHCIMSG_uiDrive = uiDrive;
            hCtrlMsg->AHCIMSG_hCtrl   = hCtrl;
            API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)hCtrlMsg, AHCI_MSG_SIZE);

            __ahciCmdReleaseResource(hDrive, bQueued);                  /* �ͷ�ʹ��Ȩ                   */
            return  (PX_ERROR);
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "state check.", 0);
        if (((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
             (hDrive->AHCIDRIVE_ucState != AHCI_DEV_INIT)) ||
            (hDrive->AHCIDRIVE_bPortError)) {                           /* ������״̬����               */
            AHCI_CMD_LOG(AHCI_LOG_ERR,
                         "port error cmd %02x ctrl %d drive %d.",
                         hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand, hCtrl->AHCICTRL_uiIndex, uiDrive);

            __ahciCmdReleaseResource(hDrive, bQueued);                  /* �Ƿ�ʹ��Ȩ                   */
            return  (PX_ERROR);
        }

        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_IN) {             /* ����ģʽ                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "read buffer invalidate.", 0);
                                                                        /* ��Ч������                   */
            API_CacheDmaInvalidate(hCmd->AHCICMD_pucDataBuf, (size_t)hCmd->AHCICMD_ulDataLen);
        }
    }

    __ahciCmdReleaseResource(hDrive, bQueued);                          /* �Ƿ����Ȩ                   */

    return  (ERROR_NONE);                                               /* ��ȷ����                     */
}
/*********************************************************************************************************
** ��������: __ahciDiskAtaParamGet
** ��������: ��ȡ ATA ����
** �䡡��  : hCtrl      ���������
**           iDrive     ��������
**           pvBuff     ����������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskAtaParamGet (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive, PVOID  pvBuff)
{
    INT                 iRet   = PX_ERROR;                              /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd   = LW_NULL;                               /* ������                     */

    AHCI_LOG(AHCI_LOG_PRT, "ata parameter get.", 0);

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* ������������               */
    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */
    if ((!hCtrl->AHCICTRL_bDrvInstalled) ||
        (!hCtrl->AHCICTRL_bInstalled)) {
        return  (PX_ERROR);
    }
    
    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen = 512;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_READP;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_NON_SEC_DATA;

    lib_bzero(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);
    iRet = __ahciDiskCommandSend(hCtrl, iDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    lib_memcpy(pvBuff, hDrive->AHCIDRIVE_pucAlignDmaBuf, 512);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDiskAtapiParamGet
** ��������: ��ȡ ATAPI ����
** �䡡��  : hCtrl      ���������
**           iDrive     ��������
**           pvBuf      ����������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskAtapiParamGet (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive, PVOID  pvBuf)
{
    INT                 iRet   = PX_ERROR;                              /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* ���������                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd   = LW_NULL;                               /* ������                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* ������������               */
    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */
    
    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen = 512;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_PI_CMD_IDENTD;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_NON_SEC_DATA;

    iRet = __ahciDiskCommandSend(hCtrl, iDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    lib_memcpy(pvBuf, hDrive->AHCIDRIVE_pucAlignDmaBuf, 512);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciLbaRangeEntriesSet
** ��������: ����������Ŀ
** �䡡��  : hDev           �豸���
**           ulStartLba     ��ʼ����
**           ulSectors      ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if AHCI_TRIM_EN > 0                                                    /* AHCI_TRIM_EN                 */

static ULONG  __ahciLbaRangeEntriesSet (PVOID  pvBuffer, ULONG  ulBuffSize, ULONG ulSector, ULONG ulCount)
{
    REGISTER INT    i = 0;
    UINT64         *pullBuffer;
    UINT64          ullEntry = 0;
    ULONG           ulBytes = 0;

    /*
     *  6-byte LBA + 2-byte range per entry
     */
    pullBuffer = (UINT64 *)pvBuffer;
    while (i < ulBuffSize / AHCI_TRIM_TCB_SIZE) {
        ullEntry = ulSector |
                   ((UINT64)(ulCount > AHCI_TRIM_BLOCK_MAX ? AHCI_TRIM_BLOCK_MAX : ulCount) << 48);

        pullBuffer[i++] = AHCI_SWAP64(ullEntry);
        if (ulCount <= AHCI_TRIM_BLOCK_MAX) {
            break;
        }
        ulCount -= AHCI_TRIM_BLOCK_MAX;
        ulSector += AHCI_TRIM_BLOCK_MAX;
    }

    ulBytes = ALIGN(i * AHCI_TRIM_TCB_SIZE, (AHCI_TRIM_TCB_SIZE * AHCI_TRIM_TCB_MAX));
    lib_bzero(pullBuffer + i, ulBytes - (i * AHCI_TRIM_TCB_SIZE));

    return  (ulBytes);
}
/*********************************************************************************************************
** ��������: __ahciDiskTrimSet
** ��������: TRIM ����
** �䡡��  : hDev               �豸���
**           ulStartSector      ��ʼ����
**           ulEndSector        ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskTrimSet (AHCI_DEV_HANDLE  hDev, ULONG  ulStartSector, ULONG  ulEndSector)
{
    INT                 iRet;                                           /* �������                     */
    REGISTER INT        i;                                              /* ѭ������                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    ULONG               ulDataLen;                                      /* ���ݳ���                     */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */
    UINT32              uiCmdCount;                                     /* ������Ŀ����                 */
    UINT32              uiSector;                                       /* ����������                   */
    ULONG               ulSectors;                                      /* ��������                     */

    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */

    if (hDrive->AHCIDRIVE_bTrim != LW_TRUE) {                           /* TRIM ��֧��                  */
        AHCI_LOG(AHCI_LOG_PRT, "trim not support.", 0);
        return  (ERROR_NONE);
    }

    AHCI_LOG(AHCI_LOG_PRT,
             "trim ctrl %d drive %d start %lu end %lu offset %lu count %lu.",
             hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
             ulStartSector, ulEndSector, hDev->AHCIDEV_ulBlkOffset, hDev->AHCIDEV_ulBlkCount);
    /*
     *  ������������Ƿ���ȷ
     */
    if ((ulStartSector > ulEndSector) ||
        (ulStartSector < hDev->AHCIDEV_ulBlkOffset) ||
        (ulEndSector > (hDev->AHCIDEV_ulBlkOffset + hDev->AHCIDEV_ulBlkCount - 1))) {
        AHCI_LOG(AHCI_LOG_ERR,
                 "sector error start sector %lu end sector %lu [%lu ~ %lu].\n",
                 ulStartSector, ulEndSector,
                 hDev->AHCIDEV_ulBlkOffset, (hDev->AHCIDEV_ulBlkOffset + hDev->AHCIDEV_ulBlkCount - 1));
        return  (PX_ERROR);
    }
    /*
     *  ������������
     */
    ulSectors = ulEndSector - ulStartSector + 1;
    uiCmdCount = (ulSectors + AHCI_TRIM_CMD_BLOCK_MAX - 1) / AHCI_TRIM_CMD_BLOCK_MAX;
    AHCI_LOG(AHCI_LOG_PRT, "sectors %lu cmd count %lu.\n", ulSectors, uiCmdCount);
    for (i = 0; i < uiCmdCount; i++) {
        uiSector = (ulSectors >= AHCI_TRIM_CMD_BLOCK_MAX) ? AHCI_TRIM_CMD_BLOCK_MAX : ulSectors;
        ulDataLen = __ahciLbaRangeEntriesSet(hDrive->AHCIDRIVE_pucAlignDmaBuf,
                                             hDrive->AHCIDRIVE_ulByteSector,
                                             ulStartSector, uiSector);
#if (AHCI_LOG_PRT & AHCI_LOG_LEVEL)                                     /* AHCI_LOG_LEVEL               */
        {
            REGISTER INT        j;
            
            API_CacheDmaFlush(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);

            AHCI_LOG(AHCI_LOG_PRT,
                     "start sector %lu one sector %lu sectors %lu data len %lu.",
                     ulStartSector, uiSector, ulSectors, ulDataLen);
            AHCI_LOG(AHCI_LOG_PRT,
                     "start sector %08x one sector %08x sectors %08x data len %08x.",
                     ulStartSector, uiSector, ulSectors, ulDataLen);
            
            for (j = 0; j < hDrive->AHCIDRIVE_ulByteSector / 8; j++) {
                AHCI_LOG(AHCI_LOG_PRT,
                         "%02d  %02x%02x %02x%02x%02x%02x%02x%02x",
                         j,
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 7],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 6],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 5],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 4],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 3],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 2],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 1],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 0]);
            }
        }
#endif                                                                  /* AHCI_LOG_LEVEL               */
        /*
         *  ��������
         */
        lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
        hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
        hCmd->AHCICMD_ulDataLen = ulDataLen;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_DSM;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = 1;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0x01;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = 0;
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_OUT;
        hCmd->AHCICMD_iFlags = 0;

        AHCI_LOG(AHCI_LOG_PRT, "send trim cmd\n", 0);
        iRet = __ahciDiskCommandSend(hDev->AHCIDEV_hCtrl, hDev->AHCIDEV_uiDrive, hCmd);
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "trim failed\n", 0);
            return  (PX_ERROR);
        }

        ulSectors -= uiSector;
        ulStartSector += uiSector;
    }

    return  (ERROR_NONE);
}

#endif                                                                  /* AHCI_TRIM_EN                 */
/*********************************************************************************************************
** ��������: API_AhciDiskTrimSet
** ��������: ��ָ��������д����
** �䡡��  : hCtrl          ���������
**           uiDrive        ��������
**           ullStartLba    ��ʼ����
**           ullSectors     ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciDiskTrimSet (AHCI_CTRL_HANDLE  hCtrl,
                          UINT              uiDrive,
                          ULONG             ulStartSector,
                          ULONG             ulEndSector)
{
#if (AHCI_TRIM_EN > 0)                                                  /* AHCI_TRIM_EN                 */
    INT                 iRet;
    AHCI_DRIVE_HANDLE   hDrive;

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];

    iRet = __ahciDiskTrimSet(hDrive->AHCIDRIVE_hDev, ulStartSector, ulEndSector);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
#endif                                                                  /* AHCI_TRIM_EN                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciReadWrite
** ��������: ��ָ��������д����
** �䡡��  : hCtrl          ���������
**           uiDrive        ��������
**           pvBuf          ���ݻ�����
**           ulLba          ��ʼ����
**           ulSectors      ������
**           uiDirection    ���ݴ��䷽��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
static INT  __ahciReadWrite (AHCI_CTRL_HANDLE  hCtrl,
                             UINT              uiDrive,
                             PVOID             pvBuf,
                             ULONG             ulLba,
                             ULONG             ulSectors,
                             UINT              uiDirection)
{
    INT                 iRet;                                           /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* ��ȡ���������               */
    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */
    /*
     *  ��������
     */
    hCmd->AHCICMD_ulDataLen = ulSectors * hDrive->AHCIDRIVE_ulByteSector;
    hCmd->AHCICMD_pucDataBuf = (UINT8 *)pvBuf;
    hCmd->AHCICMD_iFlags = 0;
    /*
     *  ��ȡ����ģʽ
     */
    if (uiDirection == O_WRONLY) {
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_OUT;
        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                      (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_MULTI_EXT : AHCI_CMD_WRITE_MULTI);
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                                  (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_EXT : AHCI_CMD_WRITE);
            }
        
        } else {
            if (hDrive->AHCIDRIVE_bNcq) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =  AHCI_CMD_WRITE_FPDMA_QUEUED;
                hCmd->AHCICMD_iFlags |= AHCI_CMD_FLAG_NCQ;
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                          (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_DMA_EXT : AHCI_CMD_WRITE_DMA);
            }
        }
    
    } else {
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                        (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_MULTI_EXT : AHCI_CMD_READ_MULTI);
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                                    (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_EXT : AHCI_CMD_READ);
            }
        
        } else {
            if (hDrive->AHCIDRIVE_bNcq) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_READ_FPDMA_QUEUED;
                hCmd->AHCICMD_iFlags |= AHCI_CMD_FLAG_NCQ;
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                            (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_DMA_EXT : AHCI_CMD_READ_DMA);
            }
        }
    }
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = (UINT16)ulSectors;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature= 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = (UINT64)ulLba;

    iRet = __ahciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciBlkReadWrite
** ��������: ���豸��д����
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ
**           ulBlkCount     ����
**           uiDirection    ��дģʽ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkReadWrite (AHCI_DEV_HANDLE  hDev,
                                PVOID            pvBuffer,
                                ULONG            ulBlkStart,
                                ULONG            ulBlkCount,
                                UINT             uiDirection)
{
    REGISTER UINT32     i;                                              /* ѭ������                     */
    INT                 iRet = PX_ERROR;                                /* �������                     */
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    PLW_BLK_DEV         hBlkdev;                                        /* ���豸���                   */
    INT                 iRetry  = 0;                                    /* ���Բ���                     */
    ULONG               ulLba   = 0;                                    /* LBA ����                     */
    ULONG               ulSector = 0;                                   /* ��������                     */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* ��ȡ���������               */
    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    hBlkdev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ȡ���豸���               */
    if ((hCtrl->AHCICTRL_bInstalled == LW_FALSE) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {                   /* ״̬����                     */
        return  (PX_ERROR);
    }

    AHCI_CMD_LOG(AHCI_LOG_PRT,
                 "ctrl %d drive %d start %d blks %d buff %p dir %d\n",
                 hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                 ulBlkStart, ulBlkCount, pvBuffer, uiDirection);

    iRet = API_AhciPmActive(hCtrl, hDev->AHCIDEV_uiDrive);              /* ʹ�ܵ�Դ                     */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    ulSector = (ULONG)hBlkdev->BLKD_ulNSector;
    if ((ulBlkStart + ulBlkCount) > (ULONG)ulSector) {                  /* ������������                 */
        AHCI_LOG(AHCI_LOG_ERR, "start blk %lu blks %d [0 ~ %lu]\n", ulBlkStart, ulBlkCount, ulSector);
        return  (PX_ERROR);
    }

    ulBlkStart += (ULONG)hDev->AHCIDEV_ulBlkOffset;
    for (i = 0; i < (UINT32)ulBlkCount; i += ulSector) {                /* ѭ��������������             */
        iRetry = 0;
        ulLba = (ULONG)ulBlkStart;

        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                ulSector = __MIN((UINT32)ulBlkCount - i, (UINT32)hDrive->AHCIDRIVE_usMultiSector);
            } else {
                ulSector = 1;
            }
        } else if ((hDrive->AHCIDRIVE_bLba48) ||
                   (hDrive->AHCIDRIVE_bLba)) {
            ulSector = __MIN((UINT32)ulBlkCount - i, (UINT32)hDrive->AHCIDRIVE_ulSectorMax);
        }

        if (uiDirection == O_WRONLY) {
            lib_memcpy(&hDev->AHCIDEV_pucAlignDmaBuf[0], pvBuffer,
                       (hDrive->AHCIDRIVE_ulByteSector * ulSector));
            API_CacheDmaFlush((PVOID)&hDev->AHCIDEV_pucAlignDmaBuf[0],
                              (size_t)(hDrive->AHCIDRIVE_ulByteSector * ulSector));
        }
        iRet = __ahciReadWrite(hCtrl, hDev->AHCIDEV_uiDrive,
                               &hDev->AHCIDEV_pucAlignDmaBuf[0], ulLba, ulSector, uiDirection);
        while (iRet != ERROR_NONE) {                                    /* ��������                     */
            if (iRetry > AHCI_RETRY_NUM) {
                goto   __error_handle;
            }
            API_TimeSleep(1);

            iRet = __ahciReadWrite(hCtrl, hDev->AHCIDEV_uiDrive,
                                   &hDev->AHCIDEV_pucAlignDmaBuf[0], ulLba, ulSector, uiDirection);
            iRetry += 1;
        }
        
        if (uiDirection == O_RDONLY) {
            API_CacheDmaInvalidate((PVOID)&hDev->AHCIDEV_pucAlignDmaBuf[0],
                                   (size_t)(hDrive->AHCIDRIVE_ulByteSector * ulSector));
            lib_memcpy(pvBuffer, &hDev->AHCIDEV_pucAlignDmaBuf[0],
                       (size_t)(hDrive->AHCIDRIVE_ulByteSector * ulSector));
        }

        ulBlkStart += (ULONG)ulSector;                                  /* ������ʼ����                 */
        pvBuffer    = (UINT8 *)pvBuffer
                    + (hBlkdev->BLKD_ulBytesPerSector
                    *  ulSector);                                       /* ���»�����                   */
    }

    return  (ERROR_NONE);

__error_handle:
    _ErrorHandle(EIO);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __ahciBlkReset
** ��������: ���豸��λ
** �䡡��  : hDev  �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkReset (AHCI_DEV_HANDLE  hDev)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* ��ȡ���������               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
         return  (PX_ERROR);                                            /* ���󷵻�                     */
    }

    return  (ERROR_NONE);                                               /* ��ȷ����                     */
}
/*********************************************************************************************************
** ��������: __ahciBlkStatusChk
** ��������: ���豸���
** �䡡��  : hDev  �豸���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkStatusChk (AHCI_DEV_HANDLE  hDev)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* ��ȡ���������               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
         return  (PX_ERROR);                                            /* ���󷵻�                     */
    }

    return  (ERROR_NONE);                                               /* ��ȷ����                     */
}
/*********************************************************************************************************
** ��������: __ahciBlkIoctl
** ��������: ���豸 ioctl
** �䡡��  : hDev   �豸���
**           iCmd   ��������
**           lArg   ���Ʋ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkIoctl (AHCI_DEV_HANDLE  hDev, INT  iCmd, LONG  lArg)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* ��ȡ���������               */
    hDrive = hDev->AHCIDEV_hDrive;                                      /* ��ȡ���������               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
         return  (PX_ERROR);                                            /* ���󷵻�                     */
    }

    switch (iCmd) {

    /*
     *  ����Ҫ֧�ֵ�����
     */
    case FIOSYNC:
    case FIODATASYNC:
    case FIOSYNCMETA:
    case FIOFLUSH:                                                      /*  ������д�����              */
    case FIOUNMOUNT:                                                    /*  ж�ؾ�                      */
    case FIODISKINIT:                                                   /*  ��ʼ������                  */
    case FIODISKCHANGE:                                                 /*  ����ý�ʷ����仯            */
        break;

    case FIOTRIM:
#if AHCI_TRIM_EN > 0                                                    /* AHCI_TRIM_EN                 */
    {
        INT                 iRet;                                       /* �������                     */
        PLW_BLK_RANGE       hBlkRange;                                  /* ���豸������Χ����           */

        hBlkRange = (PLW_BLK_RANGE)lArg;
        iRet = __ahciDiskTrimSet(hDev, hBlkRange->BLKR_ulStartSector, hBlkRange->BLKR_ulEndSector);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /* AHCI_TRIM_EN                 */
        break;
    /*
     *  �ͼ���ʽ��
     */
    case FIODISKFORMAT:                                                 /*  ��ʽ����                    */
        return  (PX_ERROR);                                             /*  ��֧�ֵͼ���ʽ��            */

    /*
     *  FatFs ��չ����
     */
    case LW_BLKD_CTRL_POWER:
    case LW_BLKD_CTRL_LOCK:
    case LW_BLKD_CTRL_EJECT:
        break;

    case LW_BLKD_GET_SECSIZE:
    case LW_BLKD_GET_BLKSIZE:
        *((LONG *)lArg) = hDrive->AHCIDRIVE_ulByteSector;
        break;

    case LW_BLKD_GET_SECNUM:
        *((ULONG *)lArg) = (ULONG)API_AhciDriveSectorCountGet(hCtrl, hDev->AHCIDEV_uiDrive);
        break;

    case FIOWTIMEOUT:
    case FIORTIMEOUT:
        break;

    case AHCI_IOCTL_APM_ENABLE:
        return  (API_AhciApmEnable(hCtrl,  hDev->AHCIDEV_uiDrive, (UINT8)lArg));
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciBlkWr
** ��������: ���豸д����
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ
**           ulBlkCount     ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkWr (AHCI_DEV_HANDLE  hDev, PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount)
{
    return  (__ahciBlkReadWrite(hDev, pvBuffer, ulBlkStart, ulBlkCount, O_WRONLY));
}
/*********************************************************************************************************
** ��������: __ahciBlkRd
** ��������: ���豸������
** �䡡��  : hDev           �豸���
**           pvBuffer       ��������ַ
**           ulBlkStart     ��ʼ
**           ulBlkCount     ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkRd (AHCI_DEV_HANDLE  hDev, PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount)
{
    return  (__ahciBlkReadWrite(hDev, pvBuffer, ulBlkStart, ulBlkCount, O_RDONLY));
}
/*********************************************************************************************************
** ��������: __ahciBlkDevRemove
** ��������: �Ƴ����豸
** �䡡��  : hCtrl      ���������
**           iDrive     ���������
** �䡡��  : ���豸���ƿ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciBlkDevRemove (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive)
{
    AHCI_LOG(AHCI_LOG_ERR, "blk device remove failed.", 0);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __ahciBlkDevCreate
** ��������: �������豸
** �䡡��  : hCtrl          ���������
**           iDrive         ���������
**           ulBlkOffset    ƫ��
**           ulBlkCount     ����
** �䡡��  : ���豸���ƿ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_BLK_DEV  __ahciBlkDevCreate (AHCI_CTRL_HANDLE  hCtrl,
                                        INT               iDrive,
                                        ULONG             ulBlkOffset,
                                        ULONG             ulBlkCount)
{
    INT                 iRet          = PX_ERROR;                       /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive        = LW_NULL;                        /* ���������                   */
    AHCI_DRV_HANDLE     hDrv          = LW_NULL;                        /* �������                     */
    PLW_BLK_DEV         hBlkDev       = LW_NULL;                        /* ���豸���                   */
    AHCI_DEV_HANDLE     hDev          = LW_NULL;                        /* �豸���                     */
    UINT64              ullBlkMax     = 0;                              /* ���������                   */
    ULONG               ulBlkMax      = 0;                              /* ���������                   */
    ULONG               ulCacheSize   = AHCI_CACHE_SIZE;                /* �����С                     */
    ULONG               ulBurstSizeRd = AHCI_CACHE_BURST_RD;            /* ⧷�����С                   */
    ULONG               ulBurstSizeWr = AHCI_CACHE_BURST_WR;            /* ⧷�д��С                   */

    if (!hCtrl) {                                                       /* �����������Ч               */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl handle.", 0);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE) {                    /* ����������δ��װ             */
        AHCI_LOG(AHCI_LOG_ERR, "ahci driver invalid.", 0);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* ������δ��װ                 */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl is not installed.", 0);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if ((iDrive < 0) ||
        (iDrive >= hCtrl->AHCICTRL_uiImpPortNum)) {                     /* ��������������               */
        AHCI_LOG(AHCI_LOG_ERR,
                 "drive %d is out of range (0-%d).", iDrive, (hCtrl->AHCICTRL_uiImpPortNum - 1));
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* ��ȡ���������               */
    hDev   = hDrive->AHCIDRIVE_hDev;                                    /* ����豸���                 */
    if (!hDev) {                                                        /* �豸�����Ч                 */
        hDev = (AHCI_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_DEV_CB));    /* �����豸���ƿ�               */
        if (!hDev) {                                                    /* ������ƿ�ʧ��               */
            AHCI_LOG(AHCI_LOG_ERR, "alloc ahci dev tcb failed.", 0);
            return  (LW_NULL);
        }
    }
    hDrive->AHCIDRIVE_hDev = hDev;                                      /* �������������豸���         */
    hDrv = hCtrl->AHCICTRL_hDrv;                                        /* ����������                 */
    /*
     *  ���������������С������� DMA �������ڴ�
     */
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_SECTOR_BUFF_SIZE_GET,
                                      (LONG)((ULONG *)&hDev->AHCIDEV_ulAlignDmaBufLen));
    if (iRet != ERROR_NONE) {
        hDev->AHCIDEV_ulAlignDmaBufLen = AHCI_SECTOR_BUFF_LEN;
    }
    hDev->AHCIDEV_pucAlignDmaBuf = (UINT8 *)API_CacheDmaMalloc((size_t)(hDev->AHCIDEV_ulAlignDmaBufLen +
                                                                        hDrive->AHCIDRIVE_ulByteSector));
    if (!hDev->AHCIDEV_pucAlignDmaBuf) {
        AHCI_LOG(AHCI_LOG_ERR, "dma alloc sector buff failed.", 0);
        return  (LW_NULL);
    }
    hDev->AHCIDEV_hCtrl       = hCtrl;                                  /* ���¿��������               */
    hDev->AHCIDEV_hDrive      = hDrive;                                 /* �������������               */
    hDev->AHCIDEV_uiCtrl      = hCtrl->AHCICTRL_uiIndex;                /* ���¿�����ȫ������           */
    hDev->AHCIDEV_uiDrive     = iDrive;                                 /* ��������������               */
    hDev->AHCIDEV_ulBlkOffset = ulBlkOffset;                            /* ��������ƫ��                 */

    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* ��ÿ��豸���               */
    if ((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH)) {               /* ������״̬����               */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;                      /* ��λ������״̬               */
        AHCI_LOG(AHCI_LOG_ERR, "drive state error.", 0);
        return  (LW_NULL);
    }

    hDrive->AHCIDRIVE_ucState = AHCI_DEV_MED_CH;                        /* �����豸״̬                 */
    switch (hDrive->AHCIDRIVE_ucType) {                                 /* ����豸����                 */

    case AHCI_TYPE_ATA:                                                 /* ATA �豸                     */
        if (hDrive->AHCIDRIVE_bLba48 == LW_TRUE) {                      /* LBA 48 ģʽ                  */
            ullBlkMax = ((hCtrl->AHCICTRL_ullLba48TotalSecs[iDrive]) - (UINT64)ulBlkOffset);
            ulBlkMax = (ULONG)ullBlkMax;
        } else if ((hDrive->AHCIDRIVE_bLba == LW_TRUE) &&
                   (hCtrl->AHCICTRL_uiLbaTotalSecs[iDrive] != 0) &&
                   (hCtrl->AHCICTRL_uiLbaTotalSecs[iDrive] >
                   (UINT32)(hDrive->AHCIDRIVE_uiCylinder *
                            hDrive->AHCIDRIVE_uiHead *
                            hDrive->AHCIDRIVE_uiSector))) {             /* LBA ģʽ                     */
            ulBlkMax = (ULONG)(hCtrl->AHCICTRL_uiLbaTotalSecs[iDrive] - (UINT32)ulBlkOffset);
        } else {                                                        /* CHS ģʽ                     */
            ulBlkMax = (ULONG)((hDrive->AHCIDRIVE_uiCylinder *
                                hDrive->AHCIDRIVE_uiHead *
                                hDrive->AHCIDRIVE_uiSector) - ulBlkOffset);
        }
        if ((ulBlkCount == 0) ||
            (ulBlkCount > ulBlkMax)) {                                  /* ȫ����������               */
            hDev->AHCIDEV_ulBlkCount = (ULONG)ulBlkMax;
        }
        /*
         *  ���ÿ��豸����
         */
        hBlkDev->BLKD_pcName            = &hDrive->AHCIDRIVE_cDevName[0];
        hBlkDev->BLKD_ulNSector         = hDev->AHCIDEV_ulBlkCount;
        hBlkDev->BLKD_ulBytesPerSector  = hDrive->AHCIDRIVE_ulByteSector;
        hBlkDev->BLKD_ulBytesPerBlock   = hDrive->AHCIDRIVE_ulByteSector;
        hBlkDev->BLKD_bRemovable        = LW_TRUE;
        hBlkDev->BLKD_iRetry            = 1;
        hBlkDev->BLKD_iFlag             = O_RDWR;
        hBlkDev->BLKD_bDiskChange       = LW_TRUE;
        hBlkDev->BLKD_pfuncBlkRd        = __ahciBlkRd;
        hBlkDev->BLKD_pfuncBlkWrt       = __ahciBlkWr;
        hBlkDev->BLKD_pfuncBlkIoctl     = __ahciBlkIoctl;
        hBlkDev->BLKD_pfuncBlkReset     = __ahciBlkReset;
        hBlkDev->BLKD_pfuncBlkStatusChk = __ahciBlkStatusChk;

        hBlkDev->BLKD_iLogic            = 0;                            /*  �����豸                    */
        hBlkDev->BLKD_uiLinkCounter     = 0;
        hBlkDev->BLKD_pvLink            = LW_NULL;

        hBlkDev->BLKD_uiPowerCounter    = 0;
        hBlkDev->BLKD_uiInitCounter     = 0;

        hDrive->AHCIDRIVE_ucState = AHCI_DEV_OK;                        /* ����������״̬               */
        /*
         *  ��ȡ���������豸������Ϣ
         */
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, iDrive, AHCI_OPT_CMD_CACHE_SIZE_GET,
                                          (LONG)((ULONG *)&ulCacheSize));
        if (iRet != ERROR_NONE) {
            ulCacheSize = AHCI_CACHE_SIZE;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, iDrive, AHCI_OPT_CMD_CACHE_BURST_RD_GET,
                                          (LONG)((ULONG *)&ulBurstSizeRd));
        if (iRet != ERROR_NONE) {
            ulBurstSizeRd = AHCI_CACHE_BURST_RD;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, iDrive, AHCI_OPT_CMD_CACHE_BURST_WR_GET,
                                          (LONG)((ULONG *)&ulBurstSizeWr));
        if (iRet != ERROR_NONE) {
            ulBurstSizeWr = AHCI_CACHE_BURST_WR;
        }
                                                                        /* �����豸                     */
        hDev->AHCIDEV_pvOemdisk = (PVOID)API_OemDiskMount2(AHCI_MEDIA_NAME,
                                                           hBlkDev,
                                                           LW_NULL,
                                                           ulCacheSize,
                                                           ulBurstSizeRd,
                                                           ulBurstSizeWr);
        if (!hDev->AHCIDEV_pvOemdisk) {                                 /* ����ʧ��                     */
            AHCI_LOG(AHCI_LOG_ERR, "oem disk mount failed.", 0);
        }
        return  (hBlkDev);                                              /* ���ؿ��豸���               */
        break;

    case AHCI_TYPE_ATAPI:                                               /* ATAPI �豸                   */
        break;

    default:                                                            /* �豸���ʹ���                 */
        return  (LW_NULL);
        break;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __ahciDiskConfig
** ��������: ���ô�������
** �䡡��  : hCtrl          ���������
**           iDrive         ��������
**           cpcDevName     �豸����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskConfig (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive, CPCHAR  cpcDevName)
{
    PLW_BLK_DEV         hBlkDev = LW_NULL;                              /* ���豸���                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */

    if (!hCtrl) {                                                       /* �����������Ч               */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl handle.", 0);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((!cpcDevName) ||
        (cpcDevName[0] == PX_EOS)) {                                    /* �豸������Ч                 */
        AHCI_LOG(AHCI_LOG_ERR, "invalid device name.", 0);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((hCtrl->AHCICTRL_bInstalled == LW_FALSE) ||
        (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE)) {                  /* �����������δ��װ           */
        AHCI_LOG(AHCI_LOG_ERR, "ctrl or driver is not installed.", 0);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* ��ȡ���������               */
    lib_strncpy(&hDrive->AHCIDRIVE_cDevName[0], cpcDevName, AHCI_DEV_NAME_MAX);
    if ((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH)) {               /* �豸״̬����                 */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
        return  (ERROR_NONE);
    }
                                                                        /* �������豸                   */
    hBlkDev = __ahciBlkDevCreate(hCtrl, iDrive, hDrive->AHCIDRIVE_ulStartSector, 0);
    if (!hBlkDev) {                                                     /* �������豸ʧ��               */
        AHCI_LOG(AHCI_LOG_ERR, "create blk dev error %s", cpcDevName);
        return  (PX_ERROR);
    }
    API_AhciDevAdd(hCtrl, iDrive);                                      /* ����豸                     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDiskCtrlInit
** ��������: ���̿�������ʼ��
** �䡡��  : hCtrl      ���������
**           iDrive     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskCtrlInit (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive)
{
    REGISTER INT        i;                                              /* ѭ������                     */
    INT                 iRet;                                           /* �������                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */
    UINT32              uiReg;                                          /* �Ĵ���                       */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d disk ctrl init.", hCtrl->AHCICTRL_uiIndex, iDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* ��ȡ���������               */
    hCmd   = &tCtrlCmd;                                                 /* ������                     */

    __ahciCmdWaitForResource(hDrive, LW_FALSE);                         /* �Ƕ���ģʽ�ȴ�               */
    hDrive->AHCIDRIVE_iInitActive = LW_TRUE;                            /* �����������״̬           */

    iRet = API_AhciDriveEngineStop(hDrive);                             /* ֹͣ DMA                     */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port engine stop failed.", 0);
        return  (PX_ERROR);
    }

    iRet = API_AhciDriveRecvFisStop(hDrive);                            /* ֹͣ���մ���                 */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port recv fis stop failed.", 0);
        return  (PX_ERROR);
    }

    API_AhciDrivePowerUp(hDrive);                                       /* ��Դʹ��                     */
    /*
     *  ����ָ���˿�
     */
    AHCI_LOG(AHCI_LOG_PRT, "port det reset, partial and slumber disable.", 0);
    AHCI_PORT_WRITE(hDrive, AHCI_PxSCTL, AHCI_PSCTL_DET_RESET | AHCI_PSCTL_IPM_PARSLUM_DISABLED);
    API_TimeMSleep(200);
    AHCI_PORT_WRITE(hDrive, AHCI_PxSCTL, 0);
    iRet = API_AhciDriveRegWait(hDrive,
                                AHCI_PxSSTS, AHCI_PSSTS_DET_MSK, LW_FALSE, AHCI_PSSTS_DET_PHY,
                                1, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port sctl reset failed.", 0);
        return  (PX_ERROR);
    }

    AHCI_LOG(AHCI_LOG_PRT, "restart ctrl %d drive %d port %d.",
             hCtrl->AHCICTRL_uiIndex, iDrive, hDrive->AHCIDRIVE_uiPort);
    AHCI_LOG(AHCI_LOG_PRT, "port : active, recv fis start, power on, spin up.", 0);
    AHCI_PORT_WRITE(hDrive,
                    AHCI_PxCMD, AHCI_PCMD_ICC_ACTIVE | AHCI_PCMD_FRE | AHCI_PCMD_POD | AHCI_PCMD_SUD);
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 3, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "wait link reactivate failed.", 0);
        return  (PX_ERROR);
    }

    API_AhciDriveRegWait(hDrive, AHCI_PxTFD, AHCI_STAT_ACCESS, LW_FALSE, 0, 20, 50, &uiReg);

    AHCI_LOG(AHCI_LOG_PRT, "port start.", 0);
    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD) | AHCI_PCMD_ST;
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    API_AhciDriveRegWait(hDrive, AHCI_PxTFD, AHCI_STAT_ACCESS, LW_FALSE, 0, 20, 50, &uiReg);

    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_SRST_ON;
    __ahciDiskCommandSend(hCtrl, iDrive, hCmd);
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCI, 0x01, LW_TRUE, 0x01, 3, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_SRST_OFF;
    __ahciDiskCommandSend(hCtrl, iDrive, hCmd);
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCI, 0x01, LW_TRUE, 0x01, 3, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d port %d stat 0x%08x.",
             hCtrl->AHCICTRL_uiIndex, iDrive, hDrive->AHCIDRIVE_uiPort,
             AHCI_PORT_READ(hDrive, AHCI_PxTFD));

    for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth; i++) {              /* ����ģʽ                     */
        hDrive->AHCIDRIVE_hSyncBSem[i] = API_SemaphoreBCreate("ahci_sync",
                                                              LW_FALSE,
                                                              (LW_OPTION_WAIT_FIFO |
                                                               LW_OPTION_OBJECT_GLOBAL),
                                                              LW_NULL);
    }

    hDrive->AHCIDRIVE_uiCmdStarted = 0;
    hDrive->AHCIDRIVE_bQueued = LW_FALSE;
    hDrive->AHCIDRIVE_iInitActive = LW_FALSE;

    __ahciCmdReleaseResource(hDrive, LW_FALSE);                         /* �ԷǶ���ģʽ�ͷ�             */

    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDiskDriveInit
** ��������: ������������ʼ��
** �䡡��  : hCtrl      ���������
**           iDrive     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDiskDriveInit (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive)
{
    INT                 iRet;                                           /* ѭ������                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    AHCI_PARAM_HANDLE   hParam;                                         /* �������                     */
    AHCI_DRV_HANDLE     hDrv;                                           /* �������                     */
    INT                 iConfigType;                                    /* ��������                     */
    UINT32              uiReg;                                          /* �Ĵ���                       */
    UINT16              usDma;                                          /* DMA ģʽ                     */
    PVOID               pvBuff;                                         /* ������                       */
    INT                 iFlag;                                          /* ���                         */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d port %d disk drive init.", hCtrl->AHCICTRL_uiIndex, iDrive);
    /*
     *  ��ȡ������Ϣ
     */
    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];
    hParam = &hDrive->AHCIDRIVE_tParam;
    iConfigType = hCtrl->AHCICTRL_piConfigType[iDrive];

    if ((!hCtrl->AHCICTRL_bDrvInstalled) ||
        (!hCtrl->AHCICTRL_bInstalled)) {                                /* �����������δ��װ           */
        return  (PX_ERROR);
    }
                                                                        /* �ȴ�����Ȩ                   */
    API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
    /*
     *  ��ȡ����������
     */
    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSIG);
    if (uiReg == AHCI_PSIG_ATAPI) {
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_ATAPI;
    } else {
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_ATA;
    }
    /*
     *  ��ȡ��������
     */
    hDrv = hCtrl->AHCICTRL_hDrv;
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, iDrive, AHCI_OPT_CMD_SECTOR_SIZE_GET,
                                      (LONG)((ULONG *)&hDrive->AHCIDRIVE_ulByteSector));
    if (iRet != ERROR_NONE) {
        hDrive->AHCIDRIVE_ulByteSector = AHCI_SECTOR_SIZE;
    }
    hDrive->AHCIDRIVE_uiAlignSize = (size_t)API_CacheLine(DATA_CACHE);
    pvBuff = API_CacheDmaMallocAlign((size_t)hDrive->AHCIDRIVE_ulByteSector,
                                     (size_t)hDrive->AHCIDRIVE_uiAlignSize);
    hDrive->AHCIDRIVE_pucAlignDmaBuf = (UINT8 *)pvBuff;
    if (!hDrive->AHCIDRIVE_pucAlignDmaBuf) {
        AHCI_LOG(AHCI_LOG_ERR, "alloc aligned vmm dma buffer failed.", 0);
    } else {
        AHCI_LOG(AHCI_LOG_PRT,
                 "align dma buf addr %p size %lu align size %lu.",
                 hDrive->AHCIDRIVE_pucAlignDmaBuf,
                 hDrive->AHCIDRIVE_ulByteSector, hDrive->AHCIDRIVE_uiAlignSize);
    }

    if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATA) {                    /* ATA ģʽ                     */
        hDrive->AHCIDRIVE_pfuncReset = __ahciDiskCtrlInit;              /* ��λ����                     */

        iRet = __ahciDiskAtaParamGet(hCtrl, iDrive, (PVOID)hParam);     /* ��ȡ����������               */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR,
                     "ctrl %d drive %d read ata parameters failed.", hCtrl->AHCICTRL_uiIndex, iDrive);
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_PREAD_F;
            goto  __error_handle;
        }
                                                                        /* ����豸                     */
        iRet = API_AhciNoDataCommandSend(hCtrl, iDrive, AHCI_CMD_DIAGNOSE, 0, 0, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
            AHCI_LOG(AHCI_LOG_ERR, "no data command failed.", 0);
            return  (PX_ERROR);
        }
        /*
         *  ��ȡ����������Ϣ
         */
        hDrive->AHCIDRIVE_uiCylinder = hParam->AHCIPARAM_usCylinders - 1;
        hDrive->AHCIDRIVE_uiHead     = hParam->AHCIPARAM_usHeads;
        hDrive->AHCIDRIVE_uiSector   = hParam->AHCIPARAM_usSectors;
        if (hParam->AHCIPARAM_usCapabilities & 0x0200) {
            hCtrl->AHCICTRL_uiLbaTotalSecs[iDrive] =
                                (UINT32)((((UINT32)((hParam->AHCIPARAM_usSectors0) & 0x0000ffff)) <<  0) |
                                         (((UINT32)((hParam->AHCIPARAM_usSectors1) & 0x0000ffff)) << 16));
        }
    
    } else if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATAPI) {           /* ATAPI ����                   */
        hDrive->AHCIDRIVE_pfuncReset = __ahciDiskCtrlInit;              /* ��������ʼ��                 */

        iRet = __ahciDiskAtapiParamGet(hCtrl, iDrive, (PVOID)hParam);   /* ��ȡ ATAPI ����              */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "read atapi parameters failed.", 0);
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_PREAD_F;
            goto  __error_handle;
        }
    }
    /*
     *  ���²�����Ϣ
     */
    hDrive->AHCIDRIVE_usMultiSector = hParam->AHCIPARAM_usMultiSecs & 0x00ff;
    hDrive->AHCIDRIVE_bMulti        = (hDrive->AHCIDRIVE_usMultiSector != 0) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bIordy        = (hParam->AHCIPARAM_usCapabilities & 0x0800) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bLba          = (hParam->AHCIPARAM_usCapabilities & 0x0200) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bDma          = (hParam->AHCIPARAM_usCapabilities & 0x0100) ? LW_TRUE : LW_FALSE;

    if ((hCtrl->AHCICTRL_bNcq == LW_TRUE) &&
        (iConfigType & AHCI_NCQ_MODE)) {
        hDrive->AHCIDRIVE_bNcq = (hParam->AHCIPARAM_usSataCapabilities & 0x0100) ? LW_TRUE : LW_FALSE;
    } else {
        hDrive->AHCIDRIVE_bNcq = LW_FALSE;
    }
    
    if (hDrive->AHCIDRIVE_bNcq) {
        hDrive->AHCIDRIVE_uiQueueDepth = __MIN((hParam->AHCIPARAM_usQueueDepth + 1),
                                               hCtrl->AHCICTRL_uiCmdSlotNum);
    } else {
        hDrive->AHCIDRIVE_uiQueueDepth = 1;
    }

    if (hParam->AHCIPARAM_usFeaturesEnabled1 & 0x0400) {
        hDrive->AHCIDRIVE_bLba48 = LW_TRUE;
        hCtrl->AHCICTRL_ullLba48TotalSecs[iDrive] =
                            (UINT64)((((UINT64)((hParam->AHCIPARAM_usLba48Size[0]) & 0x0000ffff)) <<  0) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[1]) & 0x0000ffff)) << 16) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[2]) & 0x0000ffff)) << 24) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[3]) & 0x0000ffff)) << 32));
        hDrive->AHCIDRIVE_ulSectorMax = AHCI_MAX_RW_48LBA_SECTORS;
    
    } else {
        hDrive->AHCIDRIVE_bLba48 = LW_FALSE;
        hCtrl->AHCICTRL_ullLba48TotalSecs[iDrive] = (UINT64)0;
        hDrive->AHCIDRIVE_ulSectorMax = AHCI_MAX_RW_SECTORS;
    }
    /*
     *  ��ȡ����ģʽ
     */
    hDrive->AHCIDRIVE_usPioMode = (UINT16)((hParam->AHCIPARAM_usPioMode >> 8) & 0x03);
    if (hDrive->AHCIDRIVE_usPioMode > 2) {
        hDrive->AHCIDRIVE_usPioMode = 0;
    }
    if ((hDrive->AHCIDRIVE_bIordy) &&
        (hParam->AHCIPARAM_usValid & 0x02)) {
        if (hParam->AHCIPARAM_usAdvancedPio & 0x01) {
            hDrive->AHCIDRIVE_usPioMode = 3;
        }
        if (hParam->AHCIPARAM_usAdvancedPio & 0x02) {
            hDrive->AHCIDRIVE_usPioMode = 4;
        }
    }

    if ((hDrive->AHCIDRIVE_bDma == LW_TRUE) &&
        (hParam->AHCIPARAM_usValid & 0x02)) {
        hDrive->AHCIDRIVE_usSingleDmaMode = (UINT16)((hParam->AHCIPARAM_usDmaMode >> 8) & 0x03);

        if (hDrive->AHCIDRIVE_usSingleDmaMode >= 2) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 0;
        }
        hDrive->AHCIDRIVE_usMultiDmaMode = 0;

        if (hParam->AHCIPARAM_usSingleDma & 0x04) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 2;
        } else if (hParam->AHCIPARAM_usSingleDma & 0x02) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 1;
        } else if (hParam->AHCIPARAM_usSingleDma & 0x01) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 0;
        }

        if (hParam->AHCIPARAM_usMultiDma & 0x04) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 2;
        } else if (hParam->AHCIPARAM_usMultiDma & 0x02) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 1;
        } else if (hParam->AHCIPARAM_usMultiDma & 0x01) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 0;
        }

        if (hParam->AHCIPARAM_usUltraDma & 0x4000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 6;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x2000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 5;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x1000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 4;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0800) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 3;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0400) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 2;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0200) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 1;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0100) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 0;
        }
    }

    hDrive->AHCIDRIVE_usRwPio = (UINT16)(iConfigType & AHCI_PIO_MASK);
    hDrive->AHCIDRIVE_usRwDma = (UINT16)(iConfigType & AHCI_DMA_MASK);
    hDrive->AHCIDRIVE_usRwMode = AHCI_PIO_DEF_W;

    switch (iConfigType & AHCI_MODE_MASK) {

    case AHCI_PIO_0:
    case AHCI_PIO_1:
    case AHCI_PIO_2:
    case AHCI_PIO_3:
    case AHCI_PIO_4:
    case AHCI_PIO_DEF_0:
    case AHCI_PIO_DEF_1:
        hDrive->AHCIDRIVE_usRwMode = (UINT16)(iConfigType & AHCI_MODE_MASK);
        break;

    case AHCI_PIO_AUTO:
        hDrive->AHCIDRIVE_usRwMode = (UINT16)(AHCI_PIO_W_0 + hDrive->AHCIDRIVE_usPioMode);
        break;

    case AHCI_DMA_0:
    case AHCI_DMA_1:
    case AHCI_DMA_2:
    case AHCI_DMA_3:
    case AHCI_DMA_4:
    case AHCI_DMA_5:
    case AHCI_DMA_6:
        if (hDrive->AHCIDRIVE_bDma) {
            usDma = (UINT16)((iConfigType & AHCI_MODE_MASK) - AHCI_DMA_0);
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_SINGLE) {
                if (usDma > hDrive->AHCIDRIVE_usSingleDmaMode) {
                    usDma = hDrive->AHCIDRIVE_usSingleDmaMode;
                }
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_SINGLE_0 + usDma;
            } else if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_MULTI) {
                if (usDma > hDrive->AHCIDRIVE_usMultiDmaMode) {
                    usDma = hDrive->AHCIDRIVE_usMultiDmaMode;
                }
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + usDma;
            } else if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_ULTRA) {
                if (hParam->AHCIPARAM_usUltraDma == 0) {
                    if (usDma > hDrive->AHCIDRIVE_usMultiDmaMode) {
                        usDma = hDrive->AHCIDRIVE_usMultiDmaMode;
                    }
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + usDma;
                } else {
                    if (usDma > hDrive->AHCIDRIVE_usUltraDmaMode) {
                        usDma = hDrive->AHCIDRIVE_usUltraDmaMode;
                    }
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_ULTRA_0 + usDma;
                }
            }
        } else {
            hDrive->AHCIDRIVE_usRwMode = AHCI_PIO_W_0 + hDrive->AHCIDRIVE_usPioMode;
        }
        break;

    case AHCI_DMA_AUTO:
        if (hDrive->AHCIDRIVE_bDma) {
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_SINGLE) {
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_SINGLE_0 + hDrive->AHCIDRIVE_usSingleDmaMode;
            }
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_MULTI) {
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + hDrive->AHCIDRIVE_usMultiDmaMode;
            }
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_ULTRA) {
                if (hParam->AHCIPARAM_usUltraDma != 0) {
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_ULTRA_0 + hDrive->AHCIDRIVE_usUltraDmaMode;
                } else {
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + hDrive->AHCIDRIVE_usMultiDmaMode;
                    hDrive->AHCIDRIVE_usRwDma = AHCI_DMA_MULTI;
                }
            }
        }
        break;

    default:
        break;
    }

    hDrive->AHCIDRIVE_bTrim = (hParam->AHCIPARAM_usDataSetManagement & 0x01) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_usTrimBlockNumMax = hParam->AHCIPARAM_usTrimBlockNumMax;
    iFlag = (hParam->AHCIPARAM_usAdditionalSupported >> 14) & 0x01;
    hDrive->AHCIDRIVE_bDrat = iFlag ? LW_TRUE : LW_FALSE;
    iFlag = (hParam->AHCIPARAM_usAdditionalSupported >> 5) & 0x01;
    hDrive->AHCIDRIVE_bRzat = iFlag ? LW_TRUE : LW_FALSE;

    API_AhciDriveInfoShow(hCtrl, iDrive, hParam);                       /* ��ӡ��������Ϣ               */
                                                                        /* ���ô���ģʽ                 */
    API_AhciNoDataCommandSend(hCtrl, iDrive,
                              AHCI_CMD_SET_FEATURE, AHCI_SUB_SET_RWMODE,
                              (UINT8)hDrive->AHCIDRIVE_usRwMode, 0, 0, 0, 0);
    /*
     *  �������Բ���
     */
    if (hParam->AHCIPARAM_usFeaturesSupported0 & AHCI_LOOK_SUPPORTED) {
        API_AhciNoDataCommandSend(hCtrl, iDrive, AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_LOOK, 0,0,0,0,0);
    }
    if (hParam->AHCIPARAM_usFeaturesSupported0 & AHCI_WCACHE_SUPPORTED) {
        API_AhciNoDataCommandSend(hCtrl, iDrive, AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_WCACHE, 0,0,0,0,0);
    }
    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        API_AhciNoDataCommandSend(hCtrl, iDrive, AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_APM, 0xFE,0,0,0,0);
    }

    if ((hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) &&
        (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATA)) {
        if (hDrive->AHCIDRIVE_bMulti) {
            API_AhciNoDataCommandSend(hCtrl, iDrive,
                                      AHCI_CMD_SET_MULTI, 0, hDrive->AHCIDRIVE_usMultiSector, 0, 0, 0, 0);
        } else {
            hDrive->AHCIDRIVE_usRwPio = AHCI_PIO_SINGLE;
        }
    }

    hDrive->AHCIDRIVE_ucState = AHCI_DEV_OK;
    hDrive->AHCIDRIVE_iPwmState = AHCI_PM_ACTIVE_IDLE;

__error_handle:
    API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);

    if (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d state %d status 0x%x error 0x%x.",
                 hCtrl->AHCICTRL_uiIndex, iDrive,
                 hDrive->AHCIDRIVE_ucState, hDrive->AHCIDRIVE_uiIntStatus, hDrive->AHCIDRIVE_uiIntError);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDriveNoBusyWait
** ��������: �ȴ���������æ, ��еӲ�̴��ϵ絽״̬��ȷ��Ҫһ��ʱ��
** �䡡��  : hDrive    ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDriveNoBusyWait (AHCI_DRIVE_HANDLE  hDrive)
{
    REGISTER INT    i;
    UINT32          uiReg;

    for (i = 0; i < hDrive->AHCIDRIVE_uiProbTimeCount; i++) {
        uiReg = AHCI_PORT_READ(hDrive, AHCI_PxTFD);
        if (uiReg & AHCI_STAT_ACCESS) {
            API_TimeMSleep(hDrive->AHCIDRIVE_uiProbTimeUnit);
        } else {
            break;
        }
    }

    if (i >= hDrive->AHCIDRIVE_uiProbTimeCount) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ahciDrvInit
** ��������: ��ʼ�� AHCI ����
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ahciDrvInit (AHCI_CTRL_HANDLE  hCtrl)
{
    REGISTER INT            i;                                          /* ѭ������                     */
    REGISTER INT            j;                                          /* ѭ������                     */
    INT                     iRet;                                       /* �������                     */
    LW_CLASS_THREADATTR     threadattr;                                 /* �߳̿��ƿ�                   */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* ���������                   */
    AHCI_DRV_HANDLE         hDrv;                                       /* �������                     */
    size_t                  stSizeTemp;                                 /* �ڴ��С                     */
    size_t                  stMemSize;                                  /* �ڴ��С                     */
    UINT8                  *pucCmdList;                                 /* �����б�                     */
    UINT8                  *pucRecvFis;                                 /* �����б�                     */
    INT                     iCurrPort;                                  /* ��ǰ�˿�                     */
    UINT32                  uiPortMap;                                  /* �˿�ӳ����Ϣ                 */
    UINT32                  uiReg;                                      /* �Ĵ���                       */

    AHCI_LOG(AHCI_LOG_PRT, "init ctrl %d name %s uint index %d reg addr 0x%llx.",
             hCtrl->AHCICTRL_uiIndex, hCtrl->AHCICTRL_cCtrlName, hCtrl->AHCICTRL_uiUnitIndex,
             hCtrl->AHCICTRL_pvRegAddr);

    hDrv = hCtrl->AHCICTRL_hDrv;                                        /* ����������                 */

    if (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE) {                    /* ����δ��װ                   */
        if (hDrv->AHCIDRV_pfuncVendorPlatformInit) {
            iRet = hDrv->AHCIDRV_pfuncVendorPlatformInit(hCtrl);
        } else {
            iRet = ERROR_NONE;
        }

        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d vendor platform init failed.", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bDrvInstalled = LW_TRUE;                        /* ��ʶ������װ                 */
    }

    if (hCtrl->AHCICTRL_bMonitorStarted == LW_FALSE) {                  /* ����߳�δ����               */
        hCtrl->AHCICTRL_hMsgQueue = API_MsgQueueCreate("ahci_msg",
                                                       AHCI_MSG_QUEUE_SIZE, AHCI_MSG_SIZE,
                                                       LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);
        if (hCtrl->AHCICTRL_hMsgQueue == LW_OBJECT_HANDLE_INVALID) {    /* ���������Ϣʧ��             */
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d create ahci msg queue failed.", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }

        /*
         * ��������߳�
         */
        API_ThreadAttrBuild(&threadattr,
                            AHCI_MONITOR_STK_SIZE, AHCI_MONITOR_PRIORITY,
                            LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL,
                            LW_NULL);
        threadattr.THREADATTR_pvArg = (PVOID)hCtrl;
        hCtrl->AHCICTRL_hMonitorThread = API_ThreadCreate("t_ahcimsg",
                                                          (PTHREAD_START_ROUTINE)__ahciMonitorThread,
                                                          (PLW_CLASS_THREADATTR)&threadattr,
                                                          LW_NULL);
        if (hCtrl->AHCICTRL_hMonitorThread == LW_OBJECT_HANDLE_INVALID) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d create ahci monitor thread failed.", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bMonitorStarted = LW_TRUE;
    }

    if (hCtrl->AHCICTRL_bInstalled != LW_FALSE) {                       /* �������Ѿ�����װ             */
        return  (ERROR_NONE);
    }

    /*
     *  ��ʼ��������
     */
    hCtrl->AHCICTRL_bInstalled = LW_TRUE;
    hCtrl->AHCICTRL_ulSemTimeout = AHCI_SEM_TIMEOUT_DEF;
    hCtrl->AHCICTRL_piConfigType = &_GiAhciConfigType[0];

    iRet = API_AhciCtrlReset(hCtrl);                                    /* ��λ������                   */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d control reset failed.", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    if (hDrv->AHCIDRV_pfuncVendorCtrlInit) {
        iRet = hDrv->AHCIDRV_pfuncVendorCtrlInit(hCtrl);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d vendor control init failed.", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    if (hCtrl->AHCICTRL_bIntConnect == LW_FALSE) {                      /* �ж�δ����                   */
        iRet = API_AhciCtrlIntConnect(hCtrl, __ahciIsr, "ahci_isr");    /* ���ӿ������ж�               */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d control int connect failed.", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bIntConnect = LW_TRUE;                          /* ��ʶ�ж��Ѿ�����             */
    }

    iRet = API_AhciCtrlAhciModeEnable(hCtrl);                           /* ʹ�ܿ����� AHCI ģʽ         */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d enable ahci mode failed.", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    iRet = API_AhciCtrlSssSet(hCtrl, LW_TRUE);                          /* ʹ�� Staggered Spin-up       */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d enable Staggered Spin-up failed.", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    API_AhciCtrlCapGet(hCtrl);                                          /* ��ȡ������������Ϣ           */
    API_AhciCtrlImpPortGet(hCtrl);                                      /* ��ȡ�˿ڲ���                 */
    API_AhciCtrlInfoShow(hCtrl);                                        /* չʾ��������ϸ��Ϣ           */

    /*
     *  �������������ƿ�
     */
    stSizeTemp = sizeof(AHCI_DRIVE_CB) * (size_t)hCtrl->AHCICTRL_uiImpPortNum;
    hCtrl->AHCICTRL_hDrive = (AHCI_DRIVE_HANDLE)__SHEAP_ZALLOC(stSizeTemp);
    if (!hCtrl->AHCICTRL_hDrive) {
        AHCI_LOG(AHCI_LOG_ERR, "alloc drive tcb failed.", 0);
        return  (PX_ERROR);
    }

    /*
     *  �������������������ڴ�ṹ
     */
    stMemSize = AHCI_CMD_LIST_ALIGN + AHCI_CMD_LIST_ALIGN +
                (sizeof(AHCI_RECV_FIS_CB) * AHCI_RCV_FIS_MAX) +
                (sizeof(AHCI_CMD_TABLE_CB) * (size_t)hCtrl->AHCICTRL_uiCmdSlotNum);
    stMemSize *= hCtrl->AHCICTRL_uiImpPortNum;
    if (!stMemSize) {
        AHCI_LOG(AHCI_LOG_ERR, "imp port number 0.", 0);
        return  (PX_ERROR);
    }
    pucCmdList = (UINT8 *)API_CacheDmaMalloc(stMemSize);                /* ���������ڴ�                 */
    if (!pucCmdList) {
        AHCI_LOG(AHCI_LOG_ERR, "alloc dma buf size 0x%08x failed.", stMemSize);
        return  (PX_ERROR);
    }
    hCtrl->AHCICTRL_pvMemory = (PVOID)pucCmdList;                       /* ���������ڴ���             */
    hCtrl->AHCICTRL_ulMemorySize = (ULONG)stMemSize;                    /* ���������ڴ��С             */
    lib_bzero(pucCmdList, stMemSize);                                   /* �����ڴ�����                 */
    API_CacheDmaFlush((PVOID)pucCmdList, stMemSize);                    /* ��д�����ڴ�                 */

    /*
     *  ��ȡ�����������ڴ�����
     */
    pucCmdList = (UINT8 *)(((ULONG)pucCmdList + AHCI_CMD_LIST_ALIGN) & ~(AHCI_CMD_LIST_ALIGN - 1));
    hCtrl->AHCICTRL_pucCmdList = pucCmdList;
    pucRecvFis = pucCmdList + hCtrl->AHCICTRL_uiImpPortNum * AHCI_CMD_LIST_ALIGN;
    AHCI_LOG(AHCI_LOG_PRT, "alloc cmd list addr %p size 0x%08x fis addr %p size 0x%08x.",
             pucCmdList, AHCI_CMD_LIST_ALIGN, pucRecvFis, sizeof(AHCI_RECV_FIS_CB) * AHCI_RCV_FIS_MAX);

    AHCI_LOG(AHCI_LOG_PRT, "port num %d active %d map 0x%08x.",
             hCtrl->AHCICTRL_uiPortNum, hCtrl->AHCICTRL_uiImpPortNum, hCtrl->AHCICTRL_uiImpPortMap);
    iCurrPort = 0;                                                      /* ��õ�ǰ�˿�                 */
    uiPortMap = hCtrl->AHCICTRL_uiImpPortMap;                           /* �˿ڷֲ���Ϣ                 */
    for (i = 0; i < hCtrl->AHCICTRL_uiPortNum; i++) {                   /* ��������˿�                 */
        if (uiPortMap & 1) {                                            /* �˿���Ч                     */
            AHCI_LOG(AHCI_LOG_PRT, "port %d current port %d.", i, iCurrPort);

            hDrive = &hCtrl->AHCICTRL_hDrive[iCurrPort];                /* ��ȡ���������ƾ��           */
            LW_SPIN_INIT(&hDrive->AHCIDRIVE_slLock);                    /* ��ʼ��������������           */
            hDrive->AHCIDRIVE_hCtrl  = hCtrl;                           /* ������������               */
            hDrive->AHCIDRIVE_uiPort = i;                               /* ��ȡ��ǰ�˿�����             */
                                                                        /* ��ȡ����������ַ             */
            hDrive->AHCIDRIVE_pvRegAddr = (PVOID)((ULONG)hCtrl->AHCICTRL_pvRegAddr + AHCI_DRIVE_BASE(i));
            hDrive->AHCIDRIVE_uiIntCount = 0;                           /* ��ʼ���жϼ���               */
            hDrive->AHCIDRIVE_uiTimeoutErrorCount = 0;                  /* ��ʼ����ʱ�������           */
            hDrive->AHCIDRIVE_uiTaskFileErrorCount = 0;                 /* ��ʼ��TF �������            */
            hDrive->AHCIDRIVE_iNextTag = 0;                             /* ��ʼ���������               */
            iCurrPort += 1;                                             /* ���µ�ǰ�˿�                 */
            /*
             *  ��ȡ��������ʼ����
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_SECTOR_START_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_ulStartSector));
            if (iRet != ERROR_NONE) {
                hDrive->AHCIDRIVE_ulStartSector = 0;
            }
            /*
             *  ��ȡ����������̽��ʱ��
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_PROB_TIME_UNIT_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_uiProbTimeUnit));
            if ((iRet != ERROR_NONE) ||
                (hDrive->AHCIDRIVE_uiProbTimeUnit == 0)) {
                hDrive->AHCIDRIVE_uiProbTimeUnit = AHCI_DRIVE_PROB_TIME_UNIT;
            }
            /*
             *  ��ȡ̽�����
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_PROB_TIME_COUNT_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_uiProbTimeCount));
            if ((iRet != ERROR_NONE) ||
                (hDrive->AHCIDRIVE_uiProbTimeCount == 0)) {
                hDrive->AHCIDRIVE_uiProbTimeCount = AHCI_DRIVE_PROB_TIME_COUNT;
            }

            if (hDrv->AHCIDRV_pfuncVendorDriveInit) {
                iRet = hDrv->AHCIDRV_pfuncVendorDriveInit(hDrive);
            } else {
                iRet = ERROR_NONE;
            }

            if (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "port %d vendor init failed.", hDrive->AHCIDRIVE_uiPort);
                return  (PX_ERROR);
            }
            /*
             *  ���������б��ַ��Ϣ
             */
            hDrive->AHCIDRIVE_hCmdList = (AHCI_CMD_LIST_HANDLE)pucCmdList;
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLB);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCLB, AHCI_ADDR_LOW32(pucCmdList));
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLB);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLBU);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCLBU, AHCI_ADDR_HIGH32(pucCmdList));
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLBU);
            pucCmdList += AHCI_CMD_LIST_ALIGN;
            /*
             *  ���½��� FIS ��ַ��Ϣ
             */
            hDrive->AHCIDRIVE_hRecvFis = (AHCI_RECV_FIS_HANDLE)pucRecvFis;
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFB);
            AHCI_PORT_WRITE(hDrive, AHCI_PxFB, AHCI_ADDR_LOW32(pucRecvFis));
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFB);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFBU);
            AHCI_PORT_WRITE(hDrive, AHCI_PxFBU, AHCI_ADDR_HIGH32(pucRecvFis));
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFBU);
            pucRecvFis += sizeof(AHCI_RECV_FIS_CB) * AHCI_RCV_FIS_MAX;
            /*
             *  ��������������ַ�������б�
             */
            hDrive->AHCIDRIVE_hCmdTable = (AHCI_CMD_TABLE_HANDLE)pucRecvFis;
            for (j = 0; j < hCtrl->AHCICTRL_uiCmdSlotNum; j++) {
                hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCmdTableAddrLow =
                                              AHCI_SWAP(AHCI_ADDR_LOW32(&hDrive->AHCIDRIVE_hCmdTable[j]));
                hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCmdTableAddrHigh =
                                             AHCI_SWAP(AHCI_ADDR_HIGH32(&hDrive->AHCIDRIVE_hCmdTable[j]));
                AHCI_LOG(AHCI_LOG_PRT,
                         "cmd list %2d addr %p low 0x%08x high 0x%08x.",
                         j, &hDrive->AHCIDRIVE_hCmdTable[j],
                         hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCmdTableAddrLow,
                         hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCmdTableAddrHigh);
            }
            pucRecvFis += (sizeof(AHCI_CMD_TABLE_CB) * (size_t)hCtrl->AHCICTRL_uiCmdSlotNum);
            API_CacheDmaFlush(hDrive->AHCIDRIVE_hCmdList, AHCI_CMD_LIST_ALIGN);
            
            /*
             *  ���������ͬ���ź���
             */
            for (j = 0; j < hCtrl->AHCICTRL_uiCmdSlotNum; j++) {
                hDrive->AHCIDRIVE_hSyncBSem[j] = API_SemaphoreBCreate("ahci_sync",
                                                                      LW_FALSE,
                                                                      (LW_OPTION_WAIT_FIFO |
                                                                       LW_OPTION_OBJECT_GLOBAL),
                                                                      LW_NULL);
                AHCI_LOG(AHCI_LOG_PRT, "slot %2d sync sem %p.", j, hDrive->AHCIDRIVE_hSyncBSem[j]);
            }

            hDrive->AHCIDRIVE_uiCmdStarted = 0;                         /* ��ʼ��������ʼ״̬           */
                                                                        /* ��ʼ���������               */
            hDrive->AHCIDRIVE_uiQueueDepth = hCtrl->AHCICTRL_uiCmdSlotNum;
            hDrive->AHCIDRIVE_bQueued = LW_FALSE;                       /* ��ʼ���Ƿ�ʹ�ܶ���ģʽ       */
            /*
             *  ��ʼ��ͬ����
             */
            hDrive->AHCIDRIVE_hLockMSem = API_SemaphoreMCreate("ahci_dlock",
                                                               LW_PRIO_DEF_CEILING,
                                                               (LW_OPTION_WAIT_PRIORITY |
                                                                LW_OPTION_DELETE_SAFE |
                                                                LW_OPTION_INHERIT_PRIORITY |
                                                                LW_OPTION_OBJECT_GLOBAL),
                                                               LW_NULL);
            hDrive->AHCIDRIVE_hTagMuteSem = API_SemaphoreMCreate("ahci_tag",
                                                                 LW_PRIO_DEF_CEILING,
                                                                 (LW_OPTION_WAIT_FIFO |
                                                                  LW_OPTION_DELETE_SAFE |
                                                                  LW_OPTION_INHERIT_PRIORITY |
                                                                  LW_OPTION_OBJECT_GLOBAL),
                                                                 LW_NULL);
            hDrive->AHCIDRIVE_hQueueSlotCSem = API_SemaphoreCCreate("ahci_slot",
                                                                    1,
                                                                    hCtrl->AHCICTRL_uiCmdSlotNum * 2,
                                                                    (LW_OPTION_WAIT_PRIORITY |
                                                                     LW_OPTION_OBJECT_GLOBAL),
                                                                    LW_NULL);
            /*
             *  �������
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSERR);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSERR);
            AHCI_PORT_WRITE(hDrive, AHCI_PxSERR, uiReg);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSERR);
            /*
             *  ����ж�״̬
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIS);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxIS);
            AHCI_PORT_WRITE(hDrive, AHCI_PxIS, uiReg);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIS);
            /*
             *  ʹ���ж�
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIE);
            AHCI_PORT_WRITE(hDrive, AHCI_PxIE,
                            AHCI_PIE_PRCE | AHCI_PIE_PCE | AHCI_PIE_PSE | AHCI_PIE_DSE | AHCI_PIE_DPE |
                            AHCI_PIE_DHRE | AHCI_PIS_SDBS);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIE);
            /*
             *  ��λ������״̬
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
            if (uiReg & (AHCI_PCMD_CR | AHCI_PCMD_FR | AHCI_PCMD_FRE | AHCI_PCMD_ST)) {
                uiReg &= ~AHCI_PCMD_ST;
                AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
                API_AhciDriveRegWait(hDrive,
                                     AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 20, 50, &uiReg);
            }
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCMD,
                            (AHCI_PCMD_ICC_ACTIVE | AHCI_PCMD_FRE | AHCI_PCMD_CLO | AHCI_PCMD_POD |
                             AHCI_PCMD_SUD));
            API_AhciDriveRegWait(hDrive,
                                 AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 20, 50, &uiReg);
            /*
             *  �ȴ��ȶ�״̬
             */
            iRet = API_AhciDriveRegWait(hDrive,
                                        AHCI_PxTFD, AHCI_STAT_ACCESS, LW_FALSE, 0, 20, 50, &uiReg);
        }

        uiPortMap >>= 1;
    }
    
    /*
     *  ʹ�ܿ������ж�
     */
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_GHC);
    AHCI_CTRL_WRITE(hCtrl, AHCI_GHC, uiReg | AHCI_GHC_IE);
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);

    for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum ; i++) {               /* ��ʼ��ָ��������             */
        hDrive = &hCtrl->AHCICTRL_hDrive[i];                            /* ��ȡ���������               */
        hDrive->AHCIDRIVE_hDev = LW_NULL;                               /* ��ʼ���豸���               */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_INIT;                      /* ��ʼ��������״̬             */
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_NONE;                      /* ��ʼ���豸����               */
        hDrive->AHCIDRIVE_bNcq = LW_FALSE;                              /* ��ʼ�� NCQ ��־              */
        hDrive->AHCIDRIVE_bPortError = LW_FALSE;                        /* ��ʼ���˿ڴ���״̬           */
        hDrive->AHCIDRIVE_iInitActive = LW_FALSE;                       /* ��ʼ���״̬               */

        for (j = 0; j < AHCI_RETRY_NUM; j++) {                          /* ̽���豸�Ƿ��Ѿ�����         */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSSTS);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSSTS) & AHCI_PSSTS_DET_MSK;
            if (uiReg == AHCI_PSSTS_DET_PHY) {
                break;
            }

            API_TimeMSleep(50);                                         /* ���µȴ�̽����             */
        }
        if ((j < AHCI_RETRY_NUM) &&
            (uiReg == AHCI_PSSTS_DET_PHY)) {                            /* �Ѿ�̽�⵽�豸               */
            AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d phy det.", hCtrl->AHCICTRL_uiIndex, i);
            __ahciDiskCtrlInit(hCtrl, i);                               /* ��ʼ�����̿�����             */
            __ahciDiskDriveInit(hCtrl, i);                              /* ��ʼ����������               */
        
        } else {                                                        /* û��̽�⵽�豸               */
            AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d phy not det.", hCtrl->AHCICTRL_uiIndex, i);

            hDrive->AHCIDRIVE_ucType = AHCI_TYPE_NONE;
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciNoDataCommandSend
** ��������: ���������ݴ��������
** �䡡��  : hCtrl          ���������
**           uiDrive        ��������
**           uiFeature      �ض�����
**           usSector       ������
**           ucLbaLow       LBA ����
**           ucLbaMid       LBA ����
**           ucLbaHigh      LBA ����
**           iFlags         ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciNoDataCommandSend (AHCI_CTRL_HANDLE  hCtrl,
                                UINT              uiDrive,
                                UINT8             ucCmd,
                                UINT32            uiFeature,
                                UINT16            usSector,
                                UINT8             ucLbaLow,
                                UINT8             ucLbaMid,
                                UINT8             ucLbaHigh,
                                INT               iFlags)
{
    INT                 iRet;                                           /* �������                     */
    AHCI_CMD_CB         tCtrlCmd;                                       /* ������ƿ�                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* ������                     */

    hCmd = &tCtrlCmd;                                                   /* ��ȡ������                 */
    /*
     *  ��������
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = ucCmd;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = usSector;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = uiFeature;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = (UINT64)((ucLbaHigh << 16) | (ucLbaMid << 8) | ucLbaLow);
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_NONE;
    hCmd->AHCICMD_iFlags = (INT)(iFlags | AHCI_CMD_FLAG_NON_SEC_DATA);

    iRet = __ahciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlFree
** ��������: �ͷ�һ�� AHCI ������
** �䡡��  : hCtrl     ���������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlFree (AHCI_CTRL_HANDLE  hCtrl)
{
    API_AhciCtrlDelete(hCtrl);                                          /* ɾ��������                   */

    if (hCtrl != LW_NULL) {
        __KHEAP_FREE(hCtrl);                                            /* �ͷſ�����                   */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciCtrlCreate
** ��������: ���� AHCI ������
** �䡡��  : pcName     ����������
**           iUnit      �������������
**           pvArg      ��չ����
** �䡡��  : AHCI ���������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
AHCI_CTRL_HANDLE  API_AhciCtrlCreate (CPCHAR  pcName, UINT  uiUnit, PVOID  pvArg)
{
    INT                 iRet     = PX_ERROR;                            /* �������                     */
    REGISTER INT        i        = 0;                                   /* ѭ������                     */
    AHCI_CTRL_HANDLE    hCtrl    = LW_NULL;                             /* ���������                   */
    AHCI_DRV_HANDLE     hDrv     = LW_NULL;                             /* �������                     */
    CHAR                cDriveName[AHCI_DEV_NAME_MAX] = {0};            /* �������豸����               */

    if ((!pcName   ) ||
        (uiUnit < 0)) {                                                 /* ��������������               */
        return  (LW_NULL);
    }

    hCtrl = (AHCI_CTRL_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_CTRL_CB));     /* ������������ƿ�             */
    if (!hCtrl) {                                                       /* ������ƿ�ʧ��               */
        AHCI_LOG(AHCI_LOG_ERR, "alloc ctrl %s unit %d tcb failed.", pcName, uiUnit);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }

    hDrv = API_AhciDrvHandleGet(pcName);                                /* ͨ�����ֻ���������         */
    if (!hDrv) {                                                        /* ����δע��                   */
        AHCI_LOG(AHCI_LOG_ERR, "ahci driver %s not register.", pcName);
        goto  __error_handle;
    }

    hCtrl->AHCICTRL_hDrv        = hDrv;                                 /* ����������                   */
    lib_strlcpy(&hCtrl->AHCICTRL_cCtrlName[0], &hDrv->AHCIDRV_cDrvName[0], AHCI_CTRL_NAME_MAX);
    hCtrl->AHCICTRL_uiCoreVer   = AHCI_CTRL_DRV_VER_NUM;                /* ���������İ汾               */
    hCtrl->AHCICTRL_uiUnitIndex = uiUnit;                               /* �������������               */
    hCtrl->AHCICTRL_uiIndex     = API_AhciCtrlIndexGet();               /* ����������                   */
    hCtrl->AHCICTRL_pvArg       = pvArg;
    API_AhciCtrlAdd(hCtrl);                                             /* ��ӿ�����                   */

    if (hDrv->AHCIDRV_pfuncVendorCtrlReadyWork) {
        iRet = hDrv->AHCIDRV_pfuncVendorCtrlReadyWork(hCtrl);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d vendor ready work failed.", pcName, uiUnit);
        goto  __error_handle;
    }

    if (!hCtrl->AHCICTRL_pvRegAddr) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d reg addr null.", 0);
        goto  __error_handle;
    }

    iRet = __ahciDrvInit(hCtrl);                                        /* ������ʼ��                   */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d driver init failed.", pcName, uiUnit);
        goto  __error_handle;
    }
    for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum; i++) {                /* ��������ʼ��                 */
        snprintf(cDriveName, AHCI_DEV_NAME_MAX, "/" AHCI_NAME "c%dd%d", hCtrl->AHCICTRL_uiIndex, i);
        __ahciDiskConfig(hCtrl, i, &cDriveName[0]);                     /* ��ʼ��ָ��������             */
    }

    return  (hCtrl);                                                    /* ���ؿ��������               */

__error_handle:                                                         /* ������                     */
    API_AhciCtrlFree(hCtrl);                                            /* �ͷſ�����                   */

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __ahciMonitorThread
** ��������: AHCI ����߳�
** �䡡��  : pvArg     �̲߳���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __ahciMonitorThread (PVOID  pvArg)
{
    ULONG               ulRet;                                          /* �������                     */
    AHCI_MSG_CB         tCtrlMsg;                                       /* ��Ϣ���ƿ�                   */
    size_t              stTemp = 0;                                     /* ��ʱ��С                     */
    INT                 iMsgId;                                         /* ��Ϣ��ʶ                     */
    INT                 iDrive;                                         /* ����������                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* ���������                   */
    AHCI_CTRL_HANDLE    hCtrl;                                          /* ���������                   */
    PLW_BLK_DEV         hBlkDev = LW_NULL;                              /* ���豸���                   */
    INT                 iRetry;                                         /* ���Բ���                     */
    INT                 iRet;                                           /* �������                     */

    for (;;) {
        hCtrl = (AHCI_CTRL_HANDLE)pvArg;                                /* ��ȡ���������               */
        ulRet = API_MsgQueueReceive(hCtrl->AHCICTRL_hMsgQueue,
                                    (PVOID)&tCtrlMsg,
                                    AHCI_MSG_SIZE,
                                    &stTemp,
                                    LW_OPTION_WAIT_INFINITE);           /* �ȴ���Ϣ                     */
        if (ulRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ahci msg queue recv error.", 0);
            continue;                                                   /* ���յ�������Ϣ������ȴ�     */
        }

        if (hCtrl != tCtrlMsg.AHCIMSG_hCtrl) {                          /* �����������Ч               */
            AHCI_LOG(AHCI_LOG_ERR,"msg handle error.", 0);
            continue;
        }
        hCtrl = tCtrlMsg.AHCIMSG_hCtrl;
        iDrive = tCtrlMsg.AHCIMSG_uiDrive;
        if ((iDrive < 0) ||
            (iDrive >= hCtrl->AHCICTRL_uiImpPortNum)) {                 /* ��������������               */
            AHCI_LOG(AHCI_LOG_ERR,"drive %d is out of range (0-%d).", iDrive, (AHCI_DRIVE_MAX - 1));
            continue;
        }
        iMsgId = tCtrlMsg.AHCIMSG_uiMsgId;
        hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];
        /*
         *  ����ָ����Ϣ
         */
        switch (iMsgId) {

        case AHCI_MSG_ATTACH:                                           /* �豸����                     */
            AHCI_LOG(AHCI_LOG_PRT,"recv attach msg ctrl %d drive %d.", hCtrl->AHCICTRL_uiIndex, iDrive);
            if (hDrive->AHCIDRIVE_ucState != AHCI_DEV_NONE) {
                AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d state none.", hCtrl->AHCICTRL_uiIndex, iDrive);
                continue;
            }

            hDrive->AHCIDRIVE_ucState = AHCI_DEV_INIT;
            AHCI_LOG(AHCI_LOG_PRT, "init ctrl %d drive %d.", hCtrl->AHCICTRL_uiIndex, iDrive);

            iRet = __ahciDriveNoBusyWait(hDrive);
            if (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR,
                         "wait ctrl %d drive %d no busy failed time %d ms.",
                         hCtrl->AHCICTRL_uiIndex, iDrive,
                         (hDrive->AHCIDRIVE_uiProbTimeUnit * hDrive->AHCIDRIVE_uiProbTimeCount));
                break;
            }

            iRetry = 0;
            iRet = __ahciDiskCtrlInit(hCtrl, iDrive);
            while (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "ctrl init err ctrl %d drive %d retry %d.",
                         hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                iRetry += 1;
                if (iRetry >= AHCI_RETRY_NUM) {
                    break;
                }
                iRet = __ahciDiskCtrlInit(hCtrl, iDrive);
            }

            iRetry = 0;
            iRet = __ahciDiskDriveInit(hCtrl, iDrive);
            while (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "drive init err ctrl %d drive %d retry %d.",
                         hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                iRetry += 1;
                if (iRetry >= AHCI_RETRY_NUM) {
                    break;
                }

                iRet = __ahciDiskDriveInit(hCtrl, iDrive);
            }

            hBlkDev = __ahciBlkDevCreate(hCtrl, iDrive, hDrive->AHCIDRIVE_ulStartSector, 0);
            if (!hBlkDev) {
                AHCI_LOG(AHCI_LOG_ERR, "create blk dev error %s", hDrive->AHCIDRIVE_cDevName);
                break;
            }
            API_AhciDevAdd(hCtrl, iDrive);
            break;

        case AHCI_MSG_REMOVE:                                           /* �豸�Ƴ�                     */
            AHCI_LOG(AHCI_LOG_PRT, "remove ctrl %d drive %d.", hCtrl->AHCICTRL_uiIndex, iDrive);
            if (hDrive->AHCIDRIVE_hDev != LW_NULL) {
                __ahciBlkDevRemove(hCtrl, iDrive);
            }
            break;

        case AHCI_MSG_ERROR:                                            /* �豸����                     */
            AHCI_LOG(AHCI_LOG_ERR, "error ctrl %d drive %d.", hCtrl->AHCICTRL_uiIndex, iDrive);
            __ahciDiskCtrlInit(hCtrl, iDrive);
            hDrive->AHCIDRIVE_bPortError = LW_FALSE;
            break;

        default:
            break;
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __ahciIsr
** ��������: �жϷ���
** �䡡��  : hCtrl    ���������
**           ulVector       �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t __ahciIsr (PVOID  pvArg, ULONG  ulVector)
{
    AHCI_CTRL_HANDLE    hCtrl = (AHCI_CTRL_HANDLE)pvArg;
    REGISTER INT        i;
    REGISTER INT        j;
    AHCI_DRIVE_HANDLE   hDrive;
    AHCI_MSG_CB         tCtrlMsg;
    UINT32              uiCtrlIntr;
    UINT32              uiSataIntr;
    UINT32              uiPortIntr;
    UINT32              uiTaskStatus;
    UINT32              uiSataStatus;
    UINT32              uiCmdActive;
    UINT32              uiSataActive;
    UINT32              uiActive;
    UINT32              uiReg;
    UINT32              uiSlotBit;

    tCtrlMsg.AHCIMSG_hCtrl = hCtrl;
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_IS);
    if (!uiReg) {
        return  (LW_IRQ_NONE);
    }

    while (uiReg) {
        for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum; i++) {
            hDrive = &hCtrl->AHCICTRL_hDrive[i];
            uiCtrlIntr = AHCI_CTRL_READ(hCtrl, AHCI_IS) & (_GuiAhciBitMask[hDrive->AHCIDRIVE_uiPort]);
            if (uiCtrlIntr & (_GuiAhciBitMask[hDrive->AHCIDRIVE_uiPort])) {
                hDrive->AHCIDRIVE_uiIntCount += 1;

                uiSataIntr = AHCI_PORT_READ(hDrive, AHCI_PxSERR);
                uiPortIntr = AHCI_PORT_READ(hDrive, AHCI_PxIS);
                AHCI_INT_LOG(AHCI_LOG_PRT, "AHCI_PxSERR 0x%08x.", uiSataIntr);
                AHCI_INT_LOG(AHCI_LOG_PRT, "AHCI_PxIS 0x%08x.", uiPortIntr);

                AHCI_PORT_WRITE(hDrive, AHCI_PxSERR, uiSataIntr);
                AHCI_PORT_WRITE(hDrive, AHCI_PxIS, uiPortIntr);
                AHCI_CTRL_WRITE(hCtrl, AHCI_IS, uiCtrlIntr);

                uiTaskStatus = AHCI_PORT_READ(hDrive, AHCI_PxTFD);
                uiSataStatus = AHCI_PORT_READ(hDrive, AHCI_PxSSTS);
                hDrive->AHCIDRIVE_uiIntStatus = uiTaskStatus & 0xFF;
                hDrive->AHCIDRIVE_uiIntError  = (uiTaskStatus & 0xFF00) >> 8;
                uiCmdActive = AHCI_PORT_READ(hDrive, AHCI_PxCI);
                uiSataActive = AHCI_PORT_READ(hDrive, AHCI_PxSACT);
                AHCI_INT_LOG(AHCI_LOG_PRT,
                             "AHCI_PxTFD 0x%08x AHCI_PxSSTS 0x%08x AHCI_PxCI 0x%08x AHCI_PxSACT 0x%08x.",
                             uiTaskStatus, uiSataStatus, uiCmdActive, uiSataActive);

                if ((uiPortIntr & AHCI_PIS_PRCS) &&
                    ((uiSataStatus & AHCI_PSSTS_IPM_MSK) == AHCI_PSSTS_IPM_ACTIVE)) {
                    if (hDrive->AHCIDRIVE_iInitActive == LW_FALSE) {
                        AHCI_INT_LOG(AHCI_LOG_PRT, "insert ctrl %d port %d.",
                                     hCtrl->AHCICTRL_uiIndex,hDrive->AHCIDRIVE_uiPort);

                        tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_ATTACH;
                        tCtrlMsg.AHCIMSG_uiDrive = i;
                        API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                        continue;
                    }
                }

                if ((uiPortIntr & AHCI_PIS_PRCS) &&
                    ((uiSataStatus & AHCI_PSSTS_IPM_MSK) == AHCI_PSSTS_IPM_DEVICE_NONE)) {
                    if (hDrive->AHCIDRIVE_iInitActive == LW_FALSE) {
                        AHCI_INT_LOG(AHCI_LOG_PRT, "remove ctrl %d port %d.",
                                     hCtrl->AHCICTRL_uiIndex,hDrive->AHCIDRIVE_uiPort);

                        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_NONE;
                        hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
                        hDrive->AHCIDRIVE_hDev->AHCIDEV_tBlkDev.BLKD_bDiskChange = LW_TRUE;
                        for (j = 0; j < hDrive->AHCIDRIVE_uiQueueDepth; j++) {
                            if (hDrive->AHCIDRIVE_uiCmdStarted & (_GuiAhciBitMask[j])) {
                                API_SemaphoreBPost(hDrive->AHCIDRIVE_hSyncBSem[j]);
                            }
                        }
                        tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_REMOVE;
                        tCtrlMsg.AHCIMSG_uiDrive = i;
                        API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                    }
                    continue;
                }

                if (uiPortIntr & AHCI_PIS_TFES) {
                    AHCI_INT_LOG(AHCI_LOG_PRT, "Task File Error ctrl %d port %d deep %d.",
                                 hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort,
                                 hDrive->AHCIDRIVE_uiQueueDepth);

                    hDrive->AHCIDRIVE_bPortError = LW_TRUE;
                    hDrive->AHCIDRIVE_uiTaskFileErrorCount++;
                    for (j = 0; j < hDrive->AHCIDRIVE_uiQueueDepth; j++) {
                        if (hDrive->AHCIDRIVE_uiCmdStarted & (_GuiAhciBitMask[j])) {
                            API_SemaphoreBPost(hDrive->AHCIDRIVE_hSyncBSem[j]);
                        }
                    }

                    hDrive->AHCIDRIVE_uiCmdStarted = 0;

                    tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_ERROR;
                    tCtrlMsg.AHCIMSG_uiDrive = i;
                    API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                    continue;
                }

                if (hDrive->AHCIDRIVE_bQueued == LW_TRUE) {
                    uiActive = uiSataActive;
                } else {
                    uiActive = uiCmdActive;
                }

                for (j = 0; j < hDrive->AHCIDRIVE_uiQueueDepth; j++) {
                    uiSlotBit = _GuiAhciBitMask[j];
                    if (hDrive->AHCIDRIVE_uiCmdStarted & uiSlotBit) {
                        if (!(uiActive & uiSlotBit)) {
                            API_SemaphoreBPost(hDrive->AHCIDRIVE_hSyncBSem[j]);
                            hDrive->AHCIDRIVE_uiCmdStarted &= ~uiSlotBit;
                        }
                    }
                }
            }
        }

        uiReg = AHCI_CTRL_READ(hCtrl, AHCI_IS);
    }

    return  (LW_IRQ_HANDLED);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
