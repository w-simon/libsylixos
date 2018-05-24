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
** ��   ��   ��: sdmemory.c
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2010 �� 11 �� 25 ��
**
** ��        ��: sd ���俨�ͻ�Ӧ������Դ�ļ�

** BUG:
2010.12.08  �Ż��� __sdmemTestBusy() ����.
2010.12.08  SD �豸�ṹ�м����Ѱַ���, ��֧�� SDHC ��.
2011.01.12  ���Ӷ� SPI ��֧��.
2011.03.25  �޸� __sdMemIoctl() ����, �����豸״̬���.
2011.03.25  �޸� API_SdMemDevCreate(), ���ڵײ�������װ�ϲ�Ļص�.
2011.04.03  �� API_SdMemDevShowInfo() ��Ϊ API_SdMemDevShow() ͳһ SylxiOS Show ����.
2011.04.03  ���� block io ��ص������Ĳ���.
2015.03.11  ���ӿ�д��������.
2015.09.18  ���ӶԱ��������Ĵ���.
2015.09.22  ���Ӷ� MMC/eMMC ��չЭ���֧��.
2015.12.17  ���Ӷ� MMC/eMMC �����Դ���.
2016.01.26  ������ SPI ģʽ�µ�д�����������Э��ĵط�.
2018.05.22  ���Ӷ� SD ������ʹ��1λģʽ�Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdmemory.h"
#include "../core/sdcore.h"
#include "../core/sdcoreLib.h"
#include "../core/sdstd.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
  sd ���豸�ڲ��ṹ
*********************************************************************************************************/
typedef struct __sd_blk_dev {
    LW_BLK_DEV            SDBLKDEV_blkDev;
    PLW_SDCORE_DEVICE     SDBLKDEV_pcoreDev;
    BOOL                  SDBLKDEV_bIsBlockAddr;                        /*  �Ƿ��ǿ�Ѱַ                */
    BOOL                  SDBLKDEV_bNeedReSelect;                       /*  �Ƿ���Ҫ����ѡ���豸        */
    ULONG                 SDBLKDEV_ulSectorOff;                         /*  ��������ƫ��                */
    ULONG                 SDBLKDEV_ulDiskNSector;                       /*  ���ش��̵�ʵ����������      */
    LW_BLK_INFO           SDBLKDEV_blkinfo;
    
	/*
     * ���� SDM ��, Ϊ�˱��� API ����, �������³�Ա
     */
    BOOL                  SDBLKDEV_bCoreDevSelf;                        /*  coredev ���Լ�����(��SDM��) */
} __SD_BLK_DEV, *__PSD_BLK_DEV;
/*********************************************************************************************************
  �ڲ���
*********************************************************************************************************/
#define __SDMEM_BLKADDR(pdev)       (pdev->SDBLKDEV_bIsBlockAddr)
#define __SD_CID_PNAME(iN)          (sddevcid.DEVCID_pucProductName[iN])

#define __SD_DEV_RETRY              4
#define __SD_MILLION                1000000
/*********************************************************************************************************
  æ��⺯���ĵȴ�����
*********************************************************************************************************/
#define __SD_BUSY_TYPE_READ         0
#define __SD_BUSY_TYPE_RDYDATA      1
#define __SD_BUSY_TYPE_PROG         2
#define __SD_BUSY_TYPE_ERASE        3
#define __SD_BUSY_RETRY             0x3fffffff
#define __SD_TIMEOUT_SEC            2                                   /*  2�볬ʱΪһ������ֵ         */

#define __SD_CARD_STATUS_MSK        (0x0f << 9)
#define __SD_CARD_STATUS_PRG        (0x07 << 9)                         /*  �������ڱ��                */
#define __SD_CARD_STATUS_RDYDATA    (0x01 << 8)
/*********************************************************************************************************
  ˽�к�������
*********************************************************************************************************/
static INT __sdMemTestBusy(PLW_SDCORE_DEVICE psdcoredevice, INT iType);
static INT __sdMemInit(PLW_SDCORE_DEVICE psdcoredevice);
static INT __sdMemWrtSingleBlk(PLW_SDCORE_DEVICE  psdcoredevice,
                               UINT8             *pucBuf,
                               UINT32             uiStartBlk);
static INT __sdMemWrtMultiBlk(PLW_SDCORE_DEVICE  psdcoredevice,
                              UINT8             *pucBuf,
                              UINT32             uiStartBlk,
                              UINT32             uiNBlks);
static INT __sdMemRdSingleBlk(PLW_SDCORE_DEVICE  psdcoredevice,
                              UINT8             *pucBuf,
                              UINT32             uiStartBlk);
static INT __sdMemRdMultiBlk(PLW_SDCORE_DEVICE  psdcoredevice,
                             UINT8             *pucBuf,
                             UINT32             uiStartBlk,
                             UINT32             uiNBlks);

static INT __sdMemBlkWrt(__PSD_BLK_DEV   psdblkdevice,
                        VOID            *pvWrtBuffer,
                        ULONG            ulStartBlk,
                        ULONG            ulBlkCount);
static INT __sdMemBlkRd(__PSD_BLK_DEV   psdblkdevice,
                        VOID           *pvRdBuffer,
                        ULONG           ulStartBlk,
                        ULONG           ulBlkCount);
static INT __sdMemIoctl(__PSD_BLK_DEV    psdblkdevice,
                        INT              iCmd,
                        LONG             lArg);

static INT __sdMemBlkInfoFmt(__PSD_BLK_DEV psdblkdevice);
static INT __sdMemStatus(__PSD_BLK_DEV     psdblkdevice);
static INT __sdMemReset(__PSD_BLK_DEV      psdblkdevice);

static INT __sdMemSdSwCapGet(PLW_SDCORE_DEVICE  psdcoredevice,
                             LW_SDDEV_SCR      *psdscr,
                             LW_SDDEV_CSD      *psdcsd,
                             LW_SDDEV_SW_CAP   *psdswcap);
static INT __sdMemSdHsSwitch(PLW_SDCORE_DEVICE  psdcoredevice, LW_SDDEV_SW_CAP  *psdswcap);

static INT __sdMemMmcFreqChange(PLW_SDCORE_DEVICE  psdcoredevice,
                                LW_SDDEV_CSD      *psdcsd,
                                INT               *piCardCap);
static INT __sdMemMmcBusWidthChange(PLW_SDCORE_DEVICE psdcoredevice, INT iCardCap);
static CPCHAR __sdMemProtVsnStr(UINT8 ucType, UINT8 ucVsn);
/*********************************************************************************************************
** ��������: API_SdMemDevCreate
** ��������: ����һ��SD���俨�豸
** ��    ��: iAdapterType     �豸�ҽӵ����������� (SDADAPTER_TYPE_SPI �� SDADAPTER_TYPE_SD)
**           pcAdapterName    �ҽӵ�����������
**           pcDeviceName     �豸����
**           psdmemchan       ͨ��
** ��    ��: NONE
** ��    ��: �ɹ�,�����豸���豸ָ��,���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API PLW_BLK_DEV API_SdMemDevCreate (INT                       iAdapterType,
                                       CPCHAR                    pcAdapterName,
                                       CPCHAR                    pcDeviceName,
                                       PLW_SDMEM_CHAN            psdmemchan)
{
    PLW_SDCORE_DEVICE   psdcoredevice   = LW_NULL;
    __PSD_BLK_DEV       psdblkdevice    = LW_NULL;
    PLW_BLK_DEV         pblkdevice      = LW_NULL;
    PLW_SDCORE_CHAN     psdcorechan     = LW_NULL;
    BOOL                bCoreDevSelf    = LW_TRUE;

    LW_SDDEV_CSD        sddevcsd;
    BOOL                bBlkAddr;
    INT                 iBlkDevFlag;
    ULONG               ulSectorOff;
    ULONG               ulReSelect;
    INT                 iError;

    /*
     * ������ SDM ��, Լ���������������ƺ��豸����Ϊ��ʱ, ��ʾ coredev �� SDM ����
     * ��ʱ, psdmemchan ָ���Ӧ��coredev
     */
    if (!pcAdapterName && !pcDeviceName) {
        psdcoredevice = (PLW_SDCORE_DEVICE)psdmemchan;
        bCoreDevSelf  = LW_FALSE;

    } else {
        psdcorechan   = (PLW_SDCORE_CHAN)psdmemchan;
        psdcoredevice = API_SdCoreDevCreate(iAdapterType,
                                            pcAdapterName,
                                            pcDeviceName,
                                            psdcorechan);
    }

    if (!psdcoredevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create coredevice failed.\r\n");
        return  (LW_NULL);
    }

    /*
     * ��ʼ������豸(��Լ��俨)
     */
    iError = __sdMemInit(psdcoredevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "do memory initialize failed.\r\n");

        if (bCoreDevSelf) {
            API_SdCoreDevDelete(psdcoredevice);
        }

        return  (LW_NULL);
    }

    psdblkdevice  = (__PSD_BLK_DEV)__SHEAP_ALLOC(sizeof(__SD_BLK_DEV));
    if (!psdblkdevice) {
        if (bCoreDevSelf) {
            API_SdCoreDevDelete(psdcoredevice);
        }

        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);

    }

    iError = API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "view csd of device failed.\r\n");
        __SHEAP_FREE(psdblkdevice);

        if (bCoreDevSelf) {
            API_SdCoreDevDelete(psdcoredevice);
        }

        return  (LW_NULL);
    }

    iError = API_SdCoreDevIsBlockAddr(psdcoredevice, &bBlkAddr);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unkonwn address access way.\r\n");
        __SHEAP_FREE(psdblkdevice);

        if (bCoreDevSelf) {
            API_SdCoreDevDelete(psdcoredevice);
        }

        return  (LW_NULL);
    }

    if (API_SdmHostIsCardWp(psdcoredevice)) {
        iBlkDevFlag = O_RDONLY;
    } else {
        iBlkDevFlag = O_RDWR;
    }

    API_SdmHostExtOptGet(psdcoredevice,
                         SDHOST_EXTOPT_CONFIG_FLAG_GET,
                         (LONG)&ulReSelect);

    if (ulReSelect & SDHOST_EXTOPT_CONFIG_RESELECT_SDMEM) {
        psdblkdevice->SDBLKDEV_bNeedReSelect = LW_TRUE;
    } else {
        psdblkdevice->SDBLKDEV_bNeedReSelect = LW_FALSE;
        API_SdCoreDevSelect(psdcoredevice);                             /*  ֻ���������һ��            */
    }

    API_SdmHostExtOptGet(psdcoredevice,
                         SDHOST_EXTOPT_RESERVE_SECTOR_GET,
                         (LONG)&ulSectorOff);

    if (sddevcsd.DEVCSD_uiCapacity <= ulSectorOff) {
        ulSectorOff = 0;
    }

    psdblkdevice->SDBLKDEV_bIsBlockAddr  = bBlkAddr;                    /*  ����Ѱַ��ʽ                */
    psdblkdevice->SDBLKDEV_pcoreDev      = psdcoredevice;               /*  ���Ӻ����豸                */
    psdblkdevice->SDBLKDEV_bCoreDevSelf  = bCoreDevSelf;
    psdblkdevice->SDBLKDEV_ulSectorOff   = ulSectorOff;
    psdblkdevice->SDBLKDEV_ulDiskNSector = sddevcsd.DEVCSD_uiCapacity - ulSectorOff;

    pblkdevice = &psdblkdevice->SDBLKDEV_blkDev;

    pblkdevice->BLKD_pcName            = "SD-MemCard";
    pblkdevice->BLKD_pfuncBlkRd        = __sdMemBlkRd;
    pblkdevice->BLKD_pfuncBlkWrt       = __sdMemBlkWrt;
    pblkdevice->BLKD_pfuncBlkIoctl     = __sdMemIoctl;
    pblkdevice->BLKD_pfuncBlkReset     = __sdMemReset;
    pblkdevice->BLKD_pfuncBlkStatusChk = __sdMemStatus;

    pblkdevice->BLKD_ulNSector         = psdblkdevice->SDBLKDEV_ulDiskNSector;

    pblkdevice->BLKD_ulBytesPerSector  = SD_MEM_DEFAULT_BLKSIZE;
    pblkdevice->BLKD_ulBytesPerBlock   = SD_MEM_DEFAULT_BLKSIZE;

    pblkdevice->BLKD_bRemovable        = LW_TRUE;
    pblkdevice->BLKD_bDiskChange       = LW_FALSE;                      /*  ý��û�иı�                */
    pblkdevice->BLKD_iRetry            = __SD_DEV_RETRY;                /*  ���Դ���                    */
    pblkdevice->BLKD_iFlag             = iBlkDevFlag;
    pblkdevice->BLKD_iLogic            = 0;
    pblkdevice->BLKD_uiLinkCounter     = 0;
    pblkdevice->BLKD_pvLink            = LW_NULL;
    pblkdevice->BLKD_uiPowerCounter    = 0;
    pblkdevice->BLKD_uiInitCounter     = 0;

    __sdMemBlkInfoFmt(psdblkdevice);                                    /*  ��ʽ�����豸��Ϣ            */

    return  (pblkdevice);
}
/*********************************************************************************************************
** ��������: API_SdMemDevDelete
** ��������: ɾ��һ��SD���俨�豸
** ��    ��: pblkdevice ���豸�ṹָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdMemDevDelete (PLW_BLK_DEV pblkdevice)
{
    PLW_SDCORE_DEVICE   psdcoredevice   = LW_NULL;
    __PSD_BLK_DEV       psdblkdevice    = LW_NULL;
    INT                 iError;

    if (!pblkdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdblkdevice  = (__PSD_BLK_DEV)pblkdevice;
    psdcoredevice = psdblkdevice->SDBLKDEV_pcoreDev;

    if (psdcoredevice && psdblkdevice->SDBLKDEV_bCoreDevSelf) {
        iError = API_SdCoreDevDelete(psdcoredevice);                    /*  ��ɾ��core�豸              */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delet coredevice failed.\r\n");
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    __SHEAP_FREE(psdblkdevice);                                         /*  ���ͷ�sd blk                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdMemDevShow
** ��������: ��ӡSD�豸��Ϣ
** ��    ��: pblkdevice  ���豸�ṹָ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdMemDevShow (PLW_BLK_DEV pblkdevice)
{
    PLW_SDCORE_DEVICE   psdcoredevice   = LW_NULL;
    __PSD_BLK_DEV       psdblkdevice    = LW_NULL;
    LW_SDDEV_CSD        sddevcsd;
    LW_SDDEV_CID        sddevcid;
    LW_SDDEV_SW_CAP     sddevswcap;

    UINT32              uiCapMod;
    UINT64              ullCap;
    UINT32              uiMaxSpeed;
    UINT8               ucType;
    CPCHAR              pcTypeStr = "unknown";
    CPCHAR              pcVsnStr  = "unknown";

    if (!pblkdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT) < 0) {
        return  (ERROR_NONE);
    }

    psdblkdevice   = (__PSD_BLK_DEV)pblkdevice;
    psdcoredevice = psdblkdevice->SDBLKDEV_pcoreDev;

    if (!psdcoredevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core device.\r\n");
        return  (PX_ERROR);
    }

    API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);
    API_SdCoreDevCidView(psdcoredevice, &sddevcid);
    API_SdCoreDevSwCapView(psdcoredevice, &sddevswcap);
    API_SdCoreDevTypeView(psdcoredevice, &ucType);

    if (sddevswcap.DEVSWCAP_uiHsMaxDtr) {
        uiMaxSpeed = sddevswcap.DEVSWCAP_uiHsMaxDtr;
    } else {
        uiMaxSpeed = sddevcsd.DEVCSD_uiTranSpeed;
    }

    ullCap   = (UINT64)sddevcsd.DEVCSD_uiCapacity * ((UINT64)1 << sddevcsd.DEVCSD_ucReadBlkLenBits);
    uiCapMod = ullCap % LW_CFG_MB_SIZE;

    switch (ucType) {

    case SDDEV_TYPE_MMC:
        pcTypeStr = "MMC/eMMC";
        break;

    case SDDEV_TYPE_SDSC:
        pcTypeStr = "SDSC";
        break;

    case SDDEV_TYPE_SDHC:
        pcTypeStr = "SDHC";
        break;

    case SDDEV_TYPE_SDXC:
        pcTypeStr = "SDXC";
        break;

    default:
        break;
    }

    pcVsnStr = __sdMemProtVsnStr(ucType, sddevcsd.DEVCSD_ucStructure);

    printf("\nSD Memory Information >>\n");
    printf("Manufacturer : 0x%02X\n", sddevcid.DEVCID_ucMainFid);
    if (ucType == SDDEV_TYPE_MMC) {
        printf("OEM ID       : %08X\n", sddevcid.DEVCID_usOemId);
    } else {
        printf("OEM ID       : %c%c\n", sddevcid.DEVCID_usOemId >> 8,
                                        sddevcid.DEVCID_usOemId & 0xff);
    }

    printf("Media Type   : %s(%s)\n", pcTypeStr, pcVsnStr);
    printf("Product Name : %c%c%c%c%c\n",
                           __SD_CID_PNAME(0),
                           __SD_CID_PNAME(1),
                           __SD_CID_PNAME(2),
                           __SD_CID_PNAME(3),
                           __SD_CID_PNAME(4));
    printf("Product Vsn  : v%d.%d\n", sddevcid.DEVCID_ucProductVsn >> 4,
                                      sddevcid.DEVCID_ucProductVsn & 0xf);
    printf("Serial Num   : %X\n", sddevcid.DEVCID_uiSerialNum);
    printf("Date         : %d/%02d\n", sddevcid.DEVCID_uiYear, sddevcid.DEVCID_ucMonth);
    printf("Max Speed    : %dMB/s\n", uiMaxSpeed / __SD_MILLION);
    printf("Capacity     : %u.%03u MB\n", (UINT32)(ullCap / LW_CFG_MB_SIZE), uiCapMod / 1000);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemTestBusy
** ��������: æ��⺯��.��дʱʹ��
** ��    ��: psdcoredevice  �����豸
**           iType          �������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemTestBusy (PLW_SDCORE_DEVICE psdcoredevice, INT iType)
{
    INT     iError;
    UINT    uiSta;
    INT     iRetry = 0;

    struct timespec   tvOld;
    struct timespec   tvNow;

    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);

    while (iRetry++ < __SD_BUSY_RETRY) {
        iError = API_SdCoreDevGetStatus(psdcoredevice, &uiSta);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device status failed.\r\n");
            break;
        }

        if ((uiSta & __SD_CARD_STATUS_MSK) != __SD_CARD_STATUS_PRG) {
            return   (ERROR_NONE);
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_TIMEOUT_SEC) {        /*  ��ʱ�˳�                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "timeout.\r\n");
            break;
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdMemInit
** ��������: SD���俨��ʼ��
** ��    ��: psdcoredevice  �����豸
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemInit (PLW_SDCORE_DEVICE psdcoredevice)
{
    INT             iError;
    UINT8           ucType;
    UINT32          uiOCR;
    UINT32          uiRCA;
    LW_SDDEV_OCR    sddevocr;
    LW_SDDEV_CID    sddevcid;
    LW_SDDEV_CSD    sddevcsd;
    LW_SDDEV_SCR    sddevscr;
    LW_SDDEV_SW_CAP sddevswcap;
    INT             iCardCap = 0;
    INT             iHostCap = 0;

    lib_bzero(&sddevcid, sizeof(LW_SDDEV_CID));
    lib_bzero(&sddevcsd, sizeof(LW_SDDEV_CSD));
    lib_bzero(&sddevscr, sizeof(LW_SDDEV_SCR));
    lib_bzero(&sddevswcap, sizeof(LW_SDDEV_SW_CAP));

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_POWEROFF, 0);
        bspDelayUs(10);
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_POWERON, 0);

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_SETCLK,
                                  SDARG_SETCLK_LOW);                    /*  ��ʼ��ʱ ����ʱ��           */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set clock to low failed.\r\n");
            return  (PX_ERROR);
        }

        SD_DELAYMS(10);
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_1);

        iError = API_SdCoreDevReset(psdcoredevice);                     /*  cmd0 go idle                */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device reset failed.\r\n");
            return  (PX_ERROR);
        }

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_GETOCR,
                                  (LONG)&uiOCR);

        iError = API_SdCoreDevSendIfCond(psdcoredevice);                /*  cmd8 (v2.0���ϵĿ��������) */
                                                                        /*  v2.0���µĿ���Ӧ��,���Դ��� */
        if (iError == ERROR_NONE) {                                     /*  SDHC����ʼ��֧��            */
            uiOCR |= SD_OCR_HCS;
        }

        iError = API_SdCoreDevSendAppOpCond(psdcoredevice,
                                            uiOCR,
                                            &sddevocr,
                                            &ucType);                   /*  acmd41                      */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device don't support the ocr.\r\n");
            return  (PX_ERROR);
        }

        /*
         * ���ﱣ���豸 OCR ��Ϣ, ������ݴ���Ϣ�ж��豸�Ƿ������
         * �ֽ�Ѱַ���ǿ�Ѱַ
         */
        if (sddevocr & SD_OCR_HCS) {
            psdcoredevice->COREDEV_iDevSta |= COREDEV_STA_HIGHCAP_OCR;
        }

        API_SdCoreDevTypeSet(psdcoredevice, ucType);                    /*  ����type��                  */

        iError = API_SdCoreDevSendAllCID(psdcoredevice, &sddevcid);     /*  cmd2                        */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device cid failed.\r\n");
            return  (PX_ERROR);
        }
        if (ucType == SDDEV_TYPE_MMC) {
            uiRCA = 0x01;
            iError = API_SdCoreDevMmcSetRelativeAddr(psdcoredevice, uiRCA);
        } else {
            iError = API_SdCoreDevSendRelativeAddr(psdcoredevice, &uiRCA);
                                                                        /*  cmd3                        */
        } if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device rca failed.\r\n");
            return  (PX_ERROR);
        }

        API_SdCoreDevRcaSet(psdcoredevice, uiRCA);                      /*  ����RCA��                   */

        iError = API_SdCoreDevSendAllCSD(psdcoredevice, &sddevcsd);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device csd failed.\r\n");
            return  (PX_ERROR);
        }

        if (ucType == SDDEV_TYPE_MMC) {
            LW_SDDEV_EXT_CSD  sddevextcsd;
            lib_bzero(&sddevextcsd, sizeof(sddevextcsd));
            API_SdCoreDevSelect(psdcoredevice);
            API_SdCoreDecodeExtCSD(psdcoredevice, &sddevcsd, &sddevextcsd);
            API_SdCoreDevDeSelect(psdcoredevice);
        }

        sddevcsd.DEVCSD_uiCapacity       <<= sddevcsd.DEVCSD_ucReadBlkLenBits - \
                                             SD_MEM_DEFAULT_BLKSIZE_NBITS;
        sddevcsd.DEVCSD_ucReadBlkLenBits   = SD_MEM_DEFAULT_BLKSIZE_NBITS;
        sddevcsd.DEVCSD_ucWriteBlkLenBits  = SD_MEM_DEFAULT_BLKSIZE_NBITS;

        API_SdCoreDevCsdSet(psdcoredevice, &sddevcsd);                  /*  ����CSD��                   */
        API_SdCoreDevCidSet(psdcoredevice, &sddevcid);                  /*  ����CID��                   */

        if (ucType != SDDEV_TYPE_MMC) {
            iError = API_SdCoreDevCtl(psdcoredevice,
                                      SDBUS_CTRL_SETCLK,
                                      SDARG_SETCLK_NORMAL);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set clock to normal failed.\r\n");
                return  (PX_ERROR);
            }

            API_SdmHostCapGet(psdcoredevice, &iHostCap);

            API_SdCoreDevSelect(psdcoredevice);
            iError = API_SdCoreDevSendAllSCR(psdcoredevice, &sddevscr);
            if ((iError == ERROR_NONE) && (iHostCap & SDHOST_CAP_HIGHSPEED)) {
                API_SdCoreDevScrSet(psdcoredevice, &sddevscr);          /*  ����SCR��                   */
                iError = __sdMemSdSwCapGet(psdcoredevice,
                                           &sddevscr,
                                           &sddevcsd,
                                           &sddevswcap);
                if (iError == ERROR_NONE) {
                    API_SdCoreDevSwCapSet(psdcoredevice, &sddevswcap);  /*  ����SWCAP��                 */
                    iError = __sdMemSdHsSwitch(psdcoredevice, &sddevswcap);
                    if (iError == ERROR_NONE) {
                        API_SdCoreDevCtl(psdcoredevice,
                                         SDBUS_CTRL_SETCLK,
                                         SDARG_SETCLK_MAX);
                    }
                }
            }
            API_SdCoreDevDeSelect(psdcoredevice);

            iError = API_SdCoreDevSetBlkLen(psdcoredevice, SD_MEM_DEFAULT_BLKSIZE);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set blklen failed.\r\n");
                return  (PX_ERROR);
            }

            if ((iHostCap & SDHOST_CAP_DATA_4BIT) &&
                !(iHostCap & SDHOST_CAP_SD_FORCE_1BIT)) {
            	iError = API_SdCoreDevSetBusWidth(psdcoredevice, SDARG_SETBUSWIDTH_4);
                                                                        /*  acmd6 set bus width         */
            	if (iError != ERROR_NONE) {
            		SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set bus width error.\r\n");
            		return  (PX_ERROR);
            	}
            	API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_4);
            }

        } else {                                                        /*  mmc ������������            */
            iError = API_SdCoreDevSetBlkLen(psdcoredevice, SD_MEM_DEFAULT_BLKSIZE);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set blklen failed.\r\n");
                return  (PX_ERROR);
            }

            API_SdCoreDevSelect(psdcoredevice);
            iError = __sdMemMmcFreqChange(psdcoredevice, &sddevcsd, &iCardCap);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "mmc change frequency error.\r\n");
                return  (PX_ERROR);
            }

            iError = __sdMemMmcBusWidthChange(psdcoredevice, iCardCap);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "mmc change bus width error.\r\n");
                return  (PX_ERROR);
            }
            API_SdCoreDevDeSelect(psdcoredevice);
        }
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_POWERON, 0);
        SD_DELAYMS(3);

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_SETCLK,
                                  SDARG_SETCLK_LOW);                    /*  ��ʼ��ʱ ����ʱ��           */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set clock to low failed.\r\n");
            return  (PX_ERROR);
        }

        API_SdCoreDevSpiClkDely(psdcoredevice, 100);                    /*  ��ʱ����74��ʱ��            */

        iError = API_SdCoreDevReset(psdcoredevice);                     /*  cmd0 go idle                */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device reset failed.\r\n");
            return  (PX_ERROR);
        }

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_GETOCR,
                                  (LONG)&uiOCR);

        iError = API_SdCoreSpiSendIfCond(psdcoredevice);                /*  cmd8 (v2.0���ϵĿ��������) */
                                                                        /*  v2.0���µĿ���Ӧ��,���Դ��� */

        uiOCR |= SD_OCR_HCS;                                            /*  SDHC����ʼ��֧��            */

        iError = API_SdCoreDevSendAppOpCond(psdcoredevice,
                                            uiOCR,
                                            &sddevocr,
                                            &ucType);                   /*  acmd41   +  cmd58(spi)      */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device don't support the ocr.\r\n");
            return  (PX_ERROR);
        }

        /*
         * ���ﱣ���豸 OCR ��Ϣ, ������ݴ���Ϣ�ж��豸�Ƿ������
         * �ֽ�Ѱַ���ǿ�Ѱַ
         */
        if (sddevocr & SD_OCR_HCS) {
            psdcoredevice->COREDEV_iDevSta |= COREDEV_STA_HIGHCAP_OCR;
        }

        API_SdCoreDevTypeSet(psdcoredevice, ucType);                    /*  ����type��                  */

#if LW_CFG_SDCARD_CRC_EN > 0
        API_SdCoreDevSpiCrcEn(psdcoredevice, LW_TRUE);                  /*  ʹ��crc                     */
#endif

        iError = API_SdCoreDevSetBlkLen(psdcoredevice, SD_MEM_DEFAULT_BLKSIZE);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set blklen failed.\r\n");
            return  (PX_ERROR);
        }

        iError = API_SdCoreDevSendAllCID(psdcoredevice, &sddevcid);     /*  cmd2                        */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device cid failed.\r\n");
            return  (PX_ERROR);
        }

        iError = API_SdCoreDevSendAllCSD(psdcoredevice, &sddevcsd);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device csd failed.\r\n");
            return  (PX_ERROR);
        }

        if (ucType == SDDEV_TYPE_MMC) {
            LW_SDDEV_EXT_CSD  sddevextcsd;
            lib_bzero(&sddevextcsd, sizeof(sddevextcsd));
            API_SdCoreDecodeExtCSD(psdcoredevice, &sddevcsd, &sddevextcsd);
        }

        sddevcsd.DEVCSD_uiCapacity       <<= sddevcsd.DEVCSD_ucReadBlkLenBits - \
                                             SD_MEM_DEFAULT_BLKSIZE_NBITS;
        sddevcsd.DEVCSD_ucReadBlkLenBits   = SD_MEM_DEFAULT_BLKSIZE_NBITS;
        sddevcsd.DEVCSD_ucWriteBlkLenBits  = SD_MEM_DEFAULT_BLKSIZE_NBITS;

        API_SdCoreDevCsdSet(psdcoredevice, &sddevcsd);                  /*  ����CSD��                   */
        API_SdCoreDevCidSet(psdcoredevice, &sddevcid);                  /*  ����CID��                   */

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_SETCLK,
                                  SDARG_SETCLK_NORMAL);                 /*  ����Ϊȫ��ʱ��              */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set to normal clock mode failed.\r\n");
        }
        return  (ERROR_NONE);

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknown adapter type.\r\n");
        break;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __sdMemWrtSingleBlk
** ��������: SD���俨�豸д����
** ��    ��: psdcoredevice �����豸�ṹ
**           pucBuf        д����
**           uiStartBlk    ��ʼ��ַ
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemWrtSingleBlk (PLW_SDCORE_DEVICE  psdcoredevice,
                                UINT8             *pucBuf,
                                UINT32             uiStartBlk)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    INT             iError;
    INT             iDevSta;

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode   = SD_WRITE_BLOCK;
    sdcmd.SDCMD_uiArg      = uiStartBlk;
    sdcmd.SDCMD_uiFlag     = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;   /*  ����                        */

    sddat.SDDAT_uiBlkNum   = 1;
    sddat.SDDAT_uiBlkSize  = SD_MEM_DEFAULT_BLKSIZE;
    sddat.SDDAT_uiFlags    = SD_DAT_WRITE;                              /*  ����                        */

    sdmsg.SDMSG_pucWrtBuffer = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psdcmdStop   = LW_NULL;
    sdmsg.SDMSG_pucRdBuffer  = LW_NULL;                                 /*  ������Ϊ��                  */

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SD(psdcoredevice)) {
        iError = __sdMemTestBusy(psdcoredevice, __SD_BUSY_TYPE_PROG);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "check busy error.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemWrtMultiBlk
** ��������: SD���俨�豸д���
** ��    ��: psdcoredevice �����豸�ṹ
**           pucBuf      д����
**           uiStartBlk  ��ʼ��ַ
**           uiNBlks     ������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemWrtMultiBlk (PLW_SDCORE_DEVICE  psdcoredevice,
                               UINT8             *pucBuf,
                               UINT32             uiStartBlk,
                               UINT32             uiNBlks)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    LW_SD_COMMAND   sdcmdStop;
    INT             iError;
    INT             iDevSta;

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sdcmdStop, sizeof(sdcmdStop));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode  = SD_WRITE_MULTIPLE_BLOCK;
    sdcmd.SDCMD_uiArg     = uiStartBlk;
    sdcmd.SDCMD_uiFlag    = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;    /*  ����                        */

    sddat.SDDAT_uiBlkNum  = uiNBlks;
    sddat.SDDAT_uiBlkSize = SD_MEM_DEFAULT_BLKSIZE;
    sddat.SDDAT_uiFlags   = SD_DAT_WRITE;                               /*  ����                        */

    /*
     * �ڶ��д������,Ҫ����ֹͣ����.��������SD��SPIģʽ�²�ͬ
     */
    if (COREDEV_IS_SD(psdcoredevice)) {
        sdcmdStop.SDCMD_uiOpcode = SD_STOP_TRANSMISSION;
        sdcmdStop.SDCMD_uiFlag   = SD_RSP_SPI_R1B | SD_RSP_R1B | SD_CMD_AC;
        sdmsg.SDMSG_psdcmdStop   = &sdcmdStop;                          /*  ֹͣ����                    */
    } else {
        sdmsg.SDMSG_psdcmdStop   = LW_NULL;
    }

    sdmsg.SDMSG_pucWrtBuffer = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_pucRdBuffer  = LW_NULL;                                 /*  ������Ϊ��                  */

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * SPI���д,�����ֹͣ��������
     */
    if (COREDEV_IS_SPI(psdcoredevice)) {
        API_SdCoreSpiMulWrtStop(psdcoredevice);
    } else {
        iError = __sdMemTestBusy(psdcoredevice, __SD_BUSY_TYPE_PROG);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "check busy error.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemRdSingleBlk
** ��������: SD���俨�豸������
** ��    ��: psdcoredevice �����豸�ṹ
**           pucBuf      ������
**           uiStartBlk  ��ʼ��ַ
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemRdSingleBlk (PLW_SDCORE_DEVICE  psdcoredevice,
                               UINT8             *pucBuf,
                               UINT32             uiStartBlk)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    INT             iError;
    INT             iDevSta;

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode   = SD_READ_SINGLE_BLOCK;
    sdcmd.SDCMD_uiArg      = uiStartBlk;
    sdcmd.SDCMD_uiFlag     = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;   /*  ����                        */

    sddat.SDDAT_uiBlkNum   = 1;
    sddat.SDDAT_uiBlkSize  = SD_MEM_DEFAULT_BLKSIZE;
    sddat.SDDAT_uiFlags    = SD_DAT_READ;                               /*  ����                        */

    sdmsg.SDMSG_pucRdBuffer  = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psdcmdStop   = LW_NULL;
    sdmsg.SDMSG_pucWrtBuffer = LW_NULL;                                 /*  д����Ϊ��                  */

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemRdMultiBlk
** ��������: SD���俨�豸�����
** ��    ��: psdcoredevice �����豸�ṹ
**           pucBuf      ������
**           uiStartBlk  ��ʼ��ַ
**           uiNBlks     ������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemRdMultiBlk (PLW_SDCORE_DEVICE   psdcoredevice,
                              UINT8              *pucBuf,
                              UINT32              uiStartBlk,
                              UINT32              uiNBlks)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    LW_SD_COMMAND   sdcmdStop;
    INT             iError;
    INT             iDevSta;

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sdcmdStop, sizeof(sdcmdStop));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode  = SD_READ_MULTIPLE_BLOCK;
    sdcmd.SDCMD_uiArg     = uiStartBlk;
    sdcmd.SDCMD_uiFlag    = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;    /*  ����                        */

    sddat.SDDAT_uiBlkNum  = uiNBlks;
    sddat.SDDAT_uiBlkSize = SD_MEM_DEFAULT_BLKSIZE;
    sddat.SDDAT_uiFlags   = SD_DAT_READ;                                /*  ����                        */

    /*
     * �ڶ���������, SPI��SDģʽ��ֹͣ��������ͬ��
     */
    sdcmdStop.SDCMD_uiOpcode = SD_STOP_TRANSMISSION;
    sdcmdStop.SDCMD_uiFlag   = SD_RSP_SPI_R1B | SD_RSP_R1B | SD_CMD_AC; /*  ֹͣ����                    */

    sdmsg.SDMSG_pucRdBuffer  = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psdcmdStop   = &sdcmdStop;
    sdmsg.SDMSG_pucWrtBuffer = LW_NULL;                                 /*  д����Ϊ��                  */

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemBlkWrt
** ��������: SD���俨���豸д
** ��    ��: psdblkdevice     ���豸�ṹ
**           pvWrtBuffer      д����
**           ulStartBlk       ��ʼ��ַ
**           ulBlkCount       ������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemBlkWrt(__PSD_BLK_DEV   psdblkdevice,
                         VOID           *pvWrtBuffer,
                         ULONG           ulStartBlk,
                         ULONG           ulBlkCount)
{
    INT                iError;
    INT                iDevSta;
    PLW_SDCORE_DEVICE  psdcoredevice;
    LW_SDDEV_CSD       sddevcsd;

    if (!psdblkdevice || !pvWrtBuffer) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdcoredevice = ((__PSD_BLK_DEV)psdblkdevice)->SDBLKDEV_pcoreDev;
    if (!psdcoredevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core device member.\r\n");
        return  (PX_ERROR);
    }

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SD(psdcoredevice) && psdblkdevice->SDBLKDEV_bNeedReSelect) {
        iError = API_SdCoreDevSelect(psdcoredevice);                        /*  ѡ���豸                */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
            return  (PX_ERROR);
        }
    }

    API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);

    ulStartBlk += psdblkdevice->SDBLKDEV_ulSectorOff;
    if ((ulStartBlk + ulBlkCount) > psdblkdevice->SDBLKDEV_ulDiskNSector) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "block number out of range.\r\n");
        iError = PX_ERROR;
        goto    __error_handle;
    }

    /*
     * ���ַת��
     */
    if (!__SDMEM_BLKADDR(psdblkdevice)) {
        ulStartBlk = ulStartBlk << sddevcsd.DEVCSD_ucWriteBlkLenBits;
    }

    if (ulBlkCount <= 1) {
        iError = __sdMemWrtSingleBlk(psdcoredevice, (UINT8 *)pvWrtBuffer, (UINT32)ulStartBlk);
    } else {
        iError = __sdMemWrtMultiBlk(psdcoredevice, (UINT8 *)pvWrtBuffer,
                                    (UINT32)ulStartBlk, (UINT32)ulBlkCount);
    }

__error_handle:
    if (COREDEV_IS_SD(psdcoredevice) && psdblkdevice->SDBLKDEV_bNeedReSelect) {
        API_SdCoreDevDeSelect(psdcoredevice);                           /*  ȡ���豸                    */
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: __sdMemBlkWrt
** ��������: SD���俨���豸��
** ��    ��: psdblkdevice   ���豸�ṹ
**           pvRdBuffer     ������
**           ulStartBlk     ��ʼ��ַ
**           ulBlkCount     ������
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemBlkRd (__PSD_BLK_DEV   psdblkdevice,
                         VOID           *pvRdBuffer,
                         ULONG           ulStartBlk,
                         ULONG           ulBlkCount)
{
    INT                iError;
    INT                iDevSta;
    PLW_SDCORE_DEVICE  psdcoredevice;
    LW_SDDEV_CSD       sddevcsd;

    if (!psdblkdevice || !pvRdBuffer) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdcoredevice = ((__PSD_BLK_DEV)psdblkdevice)->SDBLKDEV_pcoreDev;
    if (!psdcoredevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core device.\r\n");
        return  (PX_ERROR);
    }

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SD(psdcoredevice) && psdblkdevice->SDBLKDEV_bNeedReSelect) {
        iError = API_SdCoreDevSelect(psdcoredevice);                    /*  ѡ���豸                    */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
            return  (PX_ERROR);
        }
    }

    API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);

    ulStartBlk += psdblkdevice->SDBLKDEV_ulSectorOff;
    if ((ulStartBlk + ulBlkCount) > psdblkdevice->SDBLKDEV_ulDiskNSector) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "block number is out of range.\r\n");
        iError = PX_ERROR;
        goto    __error_handle;
    }

    /*
     * ���ַת��
     */
    if (!__SDMEM_BLKADDR(psdblkdevice)) {
        ulStartBlk = ulStartBlk << sddevcsd.DEVCSD_ucReadBlkLenBits;
    }

    if (ulBlkCount <= 1) {
        iError = __sdMemRdSingleBlk(psdcoredevice, (UINT8 *)pvRdBuffer, (UINT32)ulStartBlk);
    } else {
        iError = __sdMemRdMultiBlk(psdcoredevice, (UINT8 *)pvRdBuffer,
                                   (UINT32)ulStartBlk, (UINT32)ulBlkCount);
    }

__error_handle:
    if (COREDEV_IS_SD(psdcoredevice) && psdblkdevice->SDBLKDEV_bNeedReSelect) {
        API_SdCoreDevDeSelect(psdcoredevice);                           /*  ȡ���豸                    */
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: __sdMemIoctl
** ��������: SD���俨���豸IO����
** ��    ��: psdblkdevice   ���豸�ṹ
**           iCmd           ��������
**           lArg           ����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemIoctl (__PSD_BLK_DEV    psdblkdevice,
                         INT              iCmd,
                         LONG             lArg)
{
    INT           iDevSta;
    LW_SDDEV_CSD  sdcsd;
    LW_BLK_INFO  *pblkinfo;

    if (!psdblkdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iDevSta = API_SdCoreDevStaView(psdblkdevice->SDBLKDEV_pcoreDev);
    if (iDevSta != SD_DEVSTA_EXIST) {
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case FIOSYNC:
    case FIODATASYNC:
    case FIOFLUSH:                                                      /*  �������д������            */
    case FIOSYNCMETA:
    case FIOUNMOUNT:                                                    /*  ж�ؾ�                      */
    case FIODISKINIT:                                                   /*  ��ʼ���豸                  */
    case FIODISKCHANGE:                                                 /*  ����ý�ʷ����仯            */
        break;

    case LW_BLKD_GET_SECSIZE:
    case LW_BLKD_GET_BLKSIZE:
        *((ULONG *)lArg) = SD_MEM_DEFAULT_BLKSIZE;
        break;

    case LW_BLKD_GET_SECNUM:
        API_SdCoreDevCsdView(psdblkdevice->SDBLKDEV_pcoreDev, &sdcsd);
        *((UINT32 *)lArg) = sdcsd.DEVCSD_uiCapacity;
        break;

    case LW_BLKD_CTRL_INFO:
        pblkinfo = (LW_BLK_INFO *)lArg;
        lib_memcpy(pblkinfo, &psdblkdevice->SDBLKDEV_blkinfo, sizeof(LW_BLK_INFO));
        break;

    case FIOWTIMEOUT:
    case FIORTIMEOUT:
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemBlkInfoFmt
** ��������: ��ʽ�����豸��Ϣ
** ��    ��: psdblkdevice  ���豸�ṹ
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemBlkInfoFmt (__PSD_BLK_DEV psdblkdevice)
{
    PLW_SDCORE_DEVICE  psdcoredevice;
    LW_SDDEV_CSD       sddevcsd;
    LW_SDDEV_CID       sddevcid;
    LW_BLK_INFO       *pblkinfo;

    UINT8              ucType;
    CPCHAR             pcTypeStr = "unknown";
    CPCHAR             pcVsnStr  = "unknown";

    psdcoredevice = psdblkdevice->SDBLKDEV_pcoreDev;
    pblkinfo      = &psdblkdevice->SDBLKDEV_blkinfo;
    
    pblkinfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_SDMMC;

    API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);
    API_SdCoreDevCidView(psdcoredevice, &sddevcid);
    API_SdCoreDevTypeView(psdcoredevice, &ucType);

    switch (ucType) {

    case SDDEV_TYPE_MMC:
        pcTypeStr = "MMC/eMMC";
        break;

    case SDDEV_TYPE_SDSC:
        pcTypeStr = "SDSC";
        break;

    case SDDEV_TYPE_SDHC:
        pcTypeStr = "SDHC";
        break;

    case SDDEV_TYPE_SDXC:
        pcTypeStr = "SDXC";
        break;

    default:
        break;
    }

    pcVsnStr = __sdMemProtVsnStr(ucType, sddevcsd.DEVCSD_ucStructure);

    snprintf(pblkinfo->BLKI_cSerial, 32,
             "%08X",
             sddevcid.DEVCID_uiSerialNum);

    snprintf(pblkinfo->BLKI_cFirmware, 32,
             "%d.%02d, v%d.%d",
             sddevcid.DEVCID_uiYear,
             sddevcid.DEVCID_ucMonth,
             sddevcid.DEVCID_ucProductVsn >> 4,
             sddevcid.DEVCID_ucProductVsn & 0xf);

    snprintf(pblkinfo->BLKI_cProduct, 32,
             "%c%c%c%c%c %s memory card",
             __SD_CID_PNAME(0),
             __SD_CID_PNAME(1),
             __SD_CID_PNAME(2),
             __SD_CID_PNAME(3),
             __SD_CID_PNAME(4),
             pcTypeStr);

    snprintf(pblkinfo->BLKI_cMedia, 32,
             "%s(%s)",
             pcTypeStr,
             pcVsnStr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemStatus
** ��������: SD���俨���豸״̬����
** ��    ��: psdblkdevice  ���豸�ṹ
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemStatus (__PSD_BLK_DEV   psdblkdevice)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemStatus
** ��������: SD���俨���豸��λ
** ��    ��: psdblkdevice  ���豸�ṹ
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemReset (__PSD_BLK_DEV    psdblkdevice)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemProtVsnStr
** ��������: ��ȡ����Э��汾�ַ���
** ��    ��: ucType   ������(MMC �� SD)
**           ucVsn    CSD����Ľṹ��ʶ
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static CPCHAR __sdMemProtVsnStr (UINT8 ucType, UINT8 ucVsn)
{
    CPCHAR pcVsnStr = "v1.0";

    if (ucType != SDDEV_TYPE_MMC) {
        switch (ucVsn) {

        case CSD_STRUCT_VER_1_0:
            pcVsnStr = "v1.0";
            break;

        case CSD_STRUCT_VER_2_0:
            pcVsnStr = "v2.0";
            break;

        default:
            pcVsnStr = "v2.0";
            break;
        }

    } else {
        switch (ucVsn) {

        case MMC_VERSION_1_2:
            pcVsnStr = "v1.2";
            break;

        case MMC_VERSION_1_4:
            pcVsnStr = "v1.4";
            break;

        case MMC_VERSION_2_2:
            pcVsnStr = "v2.2";
            break;

        case MMC_VERSION_3:
            pcVsnStr = "v3.0";
            break;

        case MMC_VERSION_4:
            pcVsnStr = "v4.0";
            break;

        default:
            pcVsnStr = "v1.2";
            break;
        }
    }

    return  (pcVsnStr);
}
/*********************************************************************************************************
** ��������: __sdMemSdSwCapGet
** ��������: ���SD����SWITCH������Ϣ
** ��    ��: psdcoredevice   �����豸����
**           psdscr          SCR ��Ϣ
**           psdcsd          CSD ��Ϣ
**           psdswcap        �����õ�SWITCH������Ϣ
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemSdSwCapGet (PLW_SDCORE_DEVICE  psdcoredevice,
                              LW_SDDEV_SCR      *psdscr,
                              LW_SDDEV_CSD      *psdcsd,
                              LW_SDDEV_SW_CAP   *psdswcap)
{
    UINT8   ucSwCap[64];
    INT     iRet;

    lib_bzero(psdswcap, sizeof(LW_SDDEV_SW_CAP));

    if (psdscr->DEVSCR_ucSdaVsn < SD_SCR_SPEC_VER_1) {
        return  (PX_ERROR);
    }

    if (!(psdcsd->DEVCSD_usCmdclass & CCC_SWITCH)) {
        return  (PX_ERROR);
    }

    iRet = API_SdCoreDevSwitchEx(psdcoredevice, 0, 0, 0, ucSwCap);
    if (iRet != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get sd switch information error.\r\n");
        return  (PX_ERROR);
    }

    if (ucSwCap[13] & SD_SW_MODE_HIGH_SPEED) {
        psdswcap->DEVSWCAP_uiHsMaxDtr = 50000000;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemSdHsSwitch
** ��������: SD ������ģʽ�л�
** ��    ��: psdcoredevice   �����豸����
**           psdswcap        SWITCH������Ϣ
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemSdHsSwitch (PLW_SDCORE_DEVICE  psdcoredevice, LW_SDDEV_SW_CAP   *psdswcap)
{
    INT     iRet;
    UINT8   ucStatus[64];

    if (!psdswcap->DEVSWCAP_uiHsMaxDtr) {
        return  (PX_ERROR);
    }

    iRet = API_SdCoreDevSwitchEx(psdcoredevice, 1, 0, 1, ucStatus);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if ((ucStatus[16] & 0xF) != 1) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemMmcFreqChange
** ��������: ���� MMC ��������Ƶ��
** ��    ��: psdcoredevice   �����豸����
**           psdcsd          �Ѿ�������� CSD
**           piCardCap       ����CARD�Ĺ���(�ٶȺ�֧�ֵ�λ��ģʽ)
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemMmcFreqChange (PLW_SDCORE_DEVICE  psdcoredevice,
                                 LW_SDDEV_CSD      *psdcsd,
                                 INT               *piCardCap)
{
    UINT8   ucExtCSD[512];
    CHAR    cCardType;
    INT     iError;
    INT     iCapab  = 0;

    if (psdcsd->DEVCSD_ucStructure < MMC_VERSION_4) {
        return	(ERROR_NONE);
    }

    iError = API_SdCoreDevSendExtCSD(psdcoredevice, ucExtCSD);
    if (iError) {
        return  (iError);
    }

    if (ucExtCSD[212] || ucExtCSD[213] || ucExtCSD[214] || ucExtCSD[215]) {
        COREDEV_HIGHSPEED_SET(psdcoredevice);
    }

    cCardType = ucExtCSD[196] & 0xf;

    /*
     * ����Ĺ���λ��ʹ�� sddrvm.h����Ķ���,
     * �����ں������ֱ��ʹ��λ��ķ�ʽ�õ�ʵ����Ҫʹ�õĹ���
     */
    if (cCardType & MMC_HS_52MHZ_1_8V_3V_IO) {
        iCapab |= MMC_MODE_HS_52MHz_DDR_18_3V
               |  MMC_MODE_HS_52MHz
               |  MMC_MODE_HS
               |  SDHOST_CAP_DATA_8BIT_DDR
               |  SDHOST_CAP_DATA_4BIT_DDR
               |  SDHOST_CAP_DATA_4BIT
               |  SDHOST_CAP_DATA_8BIT;

    } else if (cCardType & MMC_HS_52MHZ) {
        iCapab |= MMC_MODE_HS_52MHz
               |  MMC_MODE_HS
               |  SDHOST_CAP_DATA_4BIT
               |  SDHOST_CAP_DATA_8BIT;
    }

    *piCardCap = iCapab;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdMemMmcBusWidthChange
** ��������: ���� MMC ��������λ��
** ��    ��: psdcoredevice   �����豸����
**           iCardCap        MMC��֧�ֵ�λ����
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdMemMmcBusWidthChange (PLW_SDCORE_DEVICE psdcoredevice, INT iCardCap)
{
    INT iHostCap = 0;
    INT iError   = ERROR_NONE;

    iError = API_SdmHostCapGet(psdcoredevice, &iHostCap);
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (!(iHostCap & SDHOST_CAP_DATA_4BIT) || (iHostCap & SDHOST_CAP_MMC_FORCE_1BIT)) {
        return  (ERROR_NONE);
    }

    iCardCap = iCardCap & iHostCap;

    if (iCardCap & MMC_MODE_HS) {
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_HS_TIMING,
                                     1);

    } else {
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_HS_TIMING,
                                     0);
    }

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "switch to %s mode failed.\r\n",
                          iCardCap & MMC_MODE_HS ? "high speed" : "full speed");
        return  (iError);
    }

    if (iCardCap & SDHOST_CAP_DATA_8BIT_DDR) {
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_8_DDR);
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     EXT_CSD_BUS_WIDTH_8_DDR);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "switch to 8Bit-DDR mode failed.\r\n");
            return  (iError);
        }

    } else if (iCardCap & SDHOST_CAP_DATA_4BIT_DDR) {
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_4_DDR);
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     EXT_CSD_BUS_WIDTH_4_DDR);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "switch to 4Bit-DDR mode failed.\r\n");
            return  (iError);
        }

    } else if (iCardCap & SDHOST_CAP_DATA_8BIT) {
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_8);
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     EXT_CSD_BUS_WIDTH_8);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "switch to 8Bit mode failed.\r\n");
            return  (iError);
        }

    } else if (iCardCap & SDHOST_CAP_DATA_4BIT) {
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_4);
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     EXT_CSD_BUS_WIDTH_4);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "switch to 4Bit mode failed.\r\n");
            return  (iError);
        }
    }

    if (iCardCap & MMC_MODE_HS) {
        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_SETCLK,
                                  SDARG_SETCLK_MAX);
    }

    return  (iError);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
