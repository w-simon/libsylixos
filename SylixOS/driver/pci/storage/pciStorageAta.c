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
** ��   ��   ��: pciStorageAta.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2016 �� 08 �� 03 ��
**
** ��        ��: ATA/IDE ����.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_ATA_EN > 0) && (LW_CFG_DRV_ATA_IDE > 0)
#include "pci_ids.h"
#include "pciStorageAta.h"
/*********************************************************************************************************
  ��������.
*********************************************************************************************************/
enum {
    board_std = 0,
    board_res = 1
};
/*********************************************************************************************************
  ����֧�ֵ��豸 ID ��.
*********************************************************************************************************/
static const PCI_DEV_ID_CB  pciStorageAtaIdTbl[] = {
    {
        PCI_VDEVICE(INTEL, 0x27c0), 
        board_std
    },
    {
        PCI_VDEVICE(INTEL, 0x7010), 
        board_std
    },
    {
        PCI_VDEVICE(INTEL, 0x2850),                                 /* ICH8 Mobile PATA Controller      */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x2828),                                 /* Mobile SATA Controller IDE       */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8c00),                                 /* SATA Controller IDE(Lynx Point)  */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8c01),                                 /* SATA Controller IDE(Lynx Point)  */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8c80),                                 /* SATA Controller IDE(9 Series)    */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8c81),                                 /* SATA Controller IDE(9 Series)    */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8d00),                                 /* SATA Controller IDE(Wellsburg)   */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8d60),                                 /* SATA Controller IDE(Wellsburg)   */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x9c00),                                 /* SATA Controller IDE(Lynx PLP)    */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x9c01),                                 /* SATA Controller IDE(Lynx PLP)    */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x1f20),                                 /* SATA Controller IDE(Avoton)      */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x1f21),                                 /* SATA Controller IDE(Avoton)      */
        board_res
    },
    
    {   
        PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, 
        PCI_CLASS_STORAGE_IDE << 8, 0xffffff00ul, board_std
    },

    { }                                                                 /* terminate list               */
};
/*********************************************************************************************************
  EXAR оƬ�����Ϣ
*********************************************************************************************************/
typedef struct {
    UINT    ATAPORT_uiPort;
    UINT    ATAPORT_uiFlag;
#define ATAPORT_IO_PORT     1
} PCI_ATA_PORT;
/*********************************************************************************************************
  �豸��Ϣ
*********************************************************************************************************/
static PCI_ATA_PORT  pciAtaPort[] = {
    {
        2, ATAPORT_IO_PORT
    },
    {
        2, 0
    }
};
/*********************************************************************************************************
  IDE ͨ�����Ͷ���
*********************************************************************************************************/
typedef struct {
    ATA_DRV_FUNCS      *ATACHAN_pDrvFuncs;
    ATA_CALLBACK        ATACHAN_pfuncAtaDevChk;
    PVOID               ATACHAN_pvAtaDevChkArg;
    ATA_CALLBACK        ATACHAN_pfuncAtaWrite;
    PVOID               ATACHAN_pvAtaWriteArg;
} PCI_ATA_CHAN;
/*********************************************************************************************************
** ��������: __pciAtaIoctl
** ��������: ���� ATA ͨ��
** �䡡��  : patachan      ATA ͨ��
**           iCmd          ����
**           pvArg         ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciAtaIoctl (ATA_CHAN  *patachan, INT  iCmd, PVOID  pvArg)
{
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __pciAtaIoOutByte
** ��������: ���һ���ֽ�
** �䡡��  : patachan      ATA ͨ��
**           ulIoAddr      IO ��ַ
**           uiData        ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciAtaIoOutByte (ATA_CHAN  *patachan, ULONG  ulIoAddr, UINT  uiData)
{
    out8((UINT8)uiData, ulIoAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pciAtaIoInByte
** ��������: ����һ���ֽ�
** �䡡��  : patachan      ATA ͨ��
**           ulIoAddr      IO ��ַ
** �䡡��  : ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciAtaIoInByte (ATA_CHAN  *patachan,  ULONG  ulIoAddr)
{
    return  (in8(ulIoAddr));
}
/*********************************************************************************************************
** ��������: __pciAtaIoOutWordString
** ��������: ���һ���ִ�
** �䡡��  : patachan      ATA ͨ��
**           ulIoAddr      IO ��ַ
**           psBuff        ���ݻ���
**           iWord         ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciAtaIoOutWordString (ATA_CHAN  *patachan, ULONG  ulIoAddr, INT16  *psBuff, INT  iWord)
{
    outs16(ulIoAddr, psBuff, iWord);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pciAtaIoInWordString
** ��������: ����һ���ִ�
** �䡡��  : patachan      ATA ͨ��
**           ulIoAddr      IO ��ַ
**           psBuff        ���ݻ���
**           iWord         ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciAtaIoInWordString (ATA_CHAN  *patachan,ULONG  ulIoAddr, INT16  *psBuff, INT  iWord)
{
    ins16(ulIoAddr, psBuff, iWord);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ideSysReset
** ��������: ��λ ATA ͨ��
** �䡡��  : patachan      ATA ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciAtaSysReset (ATA_CHAN  *patachan, INT  iDrive)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ideCallbackInstall
** ��������: ��װ�ص�����
** �䡡��  : patachan      ATA ͨ��
**           iCallbackType �ص�����
**           callback      �ص�����
**           pvCallbackArg �ص�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciAtaCallbackInstall (ATA_CHAN     *patachan,
                                     INT           iCallbackType,
                                     ATA_CALLBACK  callback,
                                     PVOID         pvCallbackArg)
{
    PCI_ATA_CHAN  *pciatachan = (PCI_ATA_CHAN *)patachan;

    switch (iCallbackType) {
    
    case ATA_CALLBACK_CHECK_DEV:
        pciatachan->ATACHAN_pfuncAtaDevChk = callback;
        pciatachan->ATACHAN_pvAtaDevChkArg = pvCallbackArg;
        break;


    case ATA_CALLBACK_WRITE_DATA:
        pciatachan->ATACHAN_pfuncAtaWrite = callback;
        pciatachan->ATACHAN_pvAtaWriteArg = pvCallbackArg;
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  ATA ��������
*********************************************************************************************************/
static ATA_DRV_FUNCS  pciStorageAtaDrvFuncs = {
    __pciAtaIoctl,
    __pciAtaIoOutByte,
    __pciAtaIoInByte,
    __pciAtaIoOutWordString,
    __pciAtaIoInWordString,
    __pciAtaSysReset,
    __pciAtaCallbackInstall
};
/*********************************************************************************************************
** ��������: pciStorageAtaCreateChan
** ��������: IDE Ӳ�̴���һ�� ATA ͨ��
** �䡡��  : NONE
** �䡡��  : ATA ͨ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ATA_CHAN  *pciStorageAtaCreateChan (VOID)
{
    PCI_ATA_CHAN  *pciatachan = (PCI_ATA_CHAN *)__SHEAP_ALLOC(sizeof(PCI_ATA_CHAN));

    if (pciatachan) {
        pciatachan->ATACHAN_pDrvFuncs = &pciStorageAtaDrvFuncs;
    }
    
    return  ((ATA_CHAN *)pciatachan);
}
/*********************************************************************************************************
  ATAPI registers ����
*********************************************************************************************************/
#define ATA_DATA(base0)         (base0 + 0)                             /* (RW) data register (16 bits) */
#define ATA_ERROR(base0)        (base0 + 1)                             /* (R)  error register          */
#define ATA_FEATURE(base0)      (base0 + 1)                             /* (W)  feature/precompensation */
#define ATA_SECCNT(base0)       (base0 + 2)                             /* (RW) sector count for ATA.   */
                                                                        /* R-Interrupt reason W-unused  */
#define ATA_SECTOR(base0)       (base0 + 3)                             /* (RW) first sector number.    */
                                                                        /* ATAPI- Reserved for SAMTAG   */
#define ATA_CYL_LO(base0)       (base0 + 4)                             /* (RW) cylinder low byte       */
                                                                        /* ATAPI - Byte count Low       */
#define ATA_CYL_HI(base0)       (base0 + 5)                             /* (RW) cylinder high byte      */
                                                                        /* ATAPI - Byte count High      */
#define ATA_SDH(base0)          (base0 + 6)                             /* (RW) sector size/drive/head  */
                                                                        /* ATAPI - drive select         */
#define ATA_COMMAND(base0)      (base0 + 7)                             /* (W)  command register        */
#define ATA_STATUS(base0)       (base0 + 7)                             /* (R)  immediate status        */

#define ATA_A_STATUS(ctl0)      (ctl0 + 2)                              /* (R)  alternate status        */
#define ATA_D_CONTROL(ctl0)     (ctl0 + 2)                              /* (W)  disk controller control */
#define ATA_D_ADDRESS(ctl0)     (ctl0 + 3)                              /* (R)  disk controller address */
/*********************************************************************************************************
** ��������: pciStorageAtaCreateDrv
** ��������: IDE Ӳ�̴���һ�� ATA �豸
** �䡡��  : ulIoBase      IO ����ַ
**           ulCtrlBase    ���ƻ���ַ
** �䡡��  : �Ƿ񴴽��ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaCreateDrv (addr_t  ulIoBase, addr_t  ulCtrlBase)
{
    ATA_CHAN       *patachan = pciStorageAtaCreateChan();
    ATA_CHAN_PARAM  param;
    INT             iError;
    static INT      iCtrlNum = 0;
    
    if (!patachan) {
        return  (PX_ERROR);
    }
    
    param.ATACP_iCtrlNum         = iCtrlNum;                            /*  ��������                    */
    param.ATACP_iDrives          = 2;                                   /*  �豸���� MASTER & SLAVE     */
    param.ATACP_iBytesPerSector  = 512;                                 /*  ÿ�����ֽ���                */
    param.ATACP_iConfigType      = ATA_PIO_MULTI | ATA_BITS_16;         /*  ���ñ�־                    */
    param.ATACP_bIntEnable       = LW_FALSE;                            /*  ϵͳ�ж�ʹ�ܱ�־            */
    param.ATACP_ulSyncSemTimeout = 2 * LW_OPTION_WAIT_A_SECOND;         /*  ͬ���ȴ���ʱʱ��(ϵͳʱ��)  */
    param.ATACP_bPreadBeSwap     = LW_FALSE;                            /*  ��� CPU Pread ����Ҫ��ת   */
    
    param.ATACP_atareg.ATAREG_ulData     = ATA_DATA(ulIoBase);
    param.ATACP_atareg.ATAREG_ulError    = ATA_ERROR(ulIoBase);
    param.ATACP_atareg.ATAREG_ulFeature  = ATA_FEATURE(ulIoBase);
    param.ATACP_atareg.ATAREG_ulSeccnt   = ATA_SECCNT(ulIoBase);
    param.ATACP_atareg.ATAREG_ulSector   = ATA_SECTOR(ulIoBase);
    param.ATACP_atareg.ATAREG_ulCylLo    = ATA_CYL_LO(ulIoBase);
    param.ATACP_atareg.ATAREG_ulCylHi    = ATA_CYL_HI(ulIoBase);
    param.ATACP_atareg.ATAREG_ulSdh      = ATA_SDH(ulIoBase);
    param.ATACP_atareg.ATAREG_ulCommand  = ATA_COMMAND(ulIoBase);
    param.ATACP_atareg.ATAREG_ulStatus   = ATA_STATUS(ulIoBase);
    param.ATACP_atareg.ATAREG_ulAStatus  = ATA_A_STATUS(ulCtrlBase);
    param.ATACP_atareg.ATAREG_ulDControl = ATA_D_CONTROL(ulCtrlBase);
    
    iError = API_AtaDrv(patachan, &param);
    if (iError < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    iCtrlNum++;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  ATA Ctrl ����
*********************************************************************************************************/
#define ATA_PRIMARY     0
#define ATA_SECONDARY   1
/*********************************************************************************************************
  ATA Drive ����
*********************************************************************************************************/
#define ATA_MASTER      0
#define ATA_SLAVE       1
/*********************************************************************************************************
** ��������: pciStorageAtaCreateDev
** ��������: IDE Ӳ�̴���һ�� ATA �豸
** �䡡��  : iCtrl             ������
**           iDrive            Drive
** �䡡��  : �Ƿ񴴽��ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaCreateDev (INT  iCtrl, INT  iDrive)
{
    PLW_BLK_DEV     pblkdev;
    
    pblkdev = API_AtaDevCreate(iCtrl, iDrive, 0, 0);
    if (!pblkdev) {
        return  (PX_ERROR);
    }
    
    API_OemDiskMount("/media/hdd", pblkdev, LW_NULL, 512 * LW_CFG_KB_SIZE, 32);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDevIdTblGet
** ��������: ��ȡ�豸�б�
** �䡡��  : phPciDevId     �豸 ID �б���������
**           puiSzie        �豸�б��С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDevIdTblGet (PCI_DEV_ID_HANDLE *phPciDevId, UINT32 *puiSzie)
{
    if ((!phPciDevId) ||
        (!puiSzie)) {
        return  (PX_ERROR);
    }

    *phPciDevId = (PCI_DEV_ID_HANDLE)pciStorageAtaIdTbl;
    *puiSzie    = sizeof(pciStorageAtaIdTbl) / sizeof(PCI_DEV_ID_CB);

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDevRemove
** ��������: �Ƴ� ATA �豸
** �䡡��  : hDevHandle         PCI �豸���ƿ���
**           hIdTable           �豸 ID �б�
** �䡡��  : ���������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
static VOID  pciStorageAtaDevRemove (PCI_DEV_HANDLE hHandle)
{
}
/*********************************************************************************************************
** ��������: pciStorageAtaDevProbe
** ��������: ATA ����̽�⵽�豸
** �䡡��  : hDevHandle         PCI �豸���ƿ���
**           hIdEntry           ƥ��ɹ����豸 ID ��Ŀ(�������豸 ID ��)
** �䡡��  : ���������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
static INT  pciStorageAtaDevProbe (PCI_DEV_HANDLE hPciDevHandle, const PCI_DEV_ID_HANDLE hIdEntry)
{
    PCI_ATA_PORT           *pataport;
    UINT                    uiChannel;
    addr_t                  ulBase, ulCtrl;                             /*  ��ʼ��ַ                    */
    PCI_RESOURCE_HANDLE     hResource;

    if ((!hPciDevHandle) || (!hIdEntry)) {                              /*  �豸������Ч                */
        _ErrorHandle(EINVAL);                                           /*  ��Ǵ���                    */
        return  (PX_ERROR);                                             /*  ���󷵻�                    */
    }
    
    if ((!hPciDevHandle) || (!hIdEntry)) {                              /*  �豸������Ч                */
        _ErrorHandle(EINVAL);                                           /*  ��Ǵ���                    */
        return  (PX_ERROR);                                             /*  ���󷵻�                    */
    }
    
    pataport = &pciAtaPort[hIdEntry->PCIDEVID_ulData];

    /*
     *  �����豸����, �豸�����汾��Ϣ����ǰ�����豸��������
     */
    hPciDevHandle->PCIDEV_uiDevVersion = ATA_PCI_DRV_VER_NUM;           /*  ��ǰ�豸�����汾��          */
    hPciDevHandle->PCIDEV_uiUnitNumber = 0;                             /*  �����豸����                */
    
    for (uiChannel = 0; uiChannel < pataport->ATAPORT_uiPort; uiChannel++) {
        if (pataport->ATAPORT_uiFlag & ATAPORT_IO_PORT) {
            if (uiChannel) {
                pciStorageAtaCreateDrv(0x170, 0x374);
            
            } else {
                pciStorageAtaCreateDrv(0x1f0, 0x3f4);
            }
        
        } else {
            hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IO, 2 * uiChannel);
            ulBase = (addr_t)(PCI_RESOURCE_START(hResource));

            hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IO, 2 * uiChannel + 1);
            ulCtrl = (addr_t)(PCI_RESOURCE_START(hResource));

            if (pciStorageAtaCreateDrv(ulBase, ulCtrl) < 0) {
                continue;
            }
        }
        
        if (uiChannel) {
            pciStorageAtaCreateDev(ATA_SECONDARY, ATA_MASTER);
            pciStorageAtaCreateDev(ATA_SECONDARY, ATA_SLAVE);
        
        } else {
            pciStorageAtaCreateDev(ATA_PRIMARY, ATA_MASTER);
            pciStorageAtaCreateDev(ATA_PRIMARY, ATA_SLAVE);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaInit
** ��������: �豸������ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  pciStorageAtaInit (VOID)
{
    INT                 iRet;                                           /*  �������                    */
    PCI_DRV_CB          tPciDrv;                                        /*  ��������������ע������      */
    PCI_DRV_HANDLE      hPciDrv = &tPciDrv;                             /*  �������ƿ���              */

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));                             /*  ��λ�������ƿ����          */
    iRet = pciStorageAtaDevIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {                                           /*  ��ȡ�豸 ID ��ʧ��          */
        return  (PX_ERROR);
    }
                                                                        /*  ������������                */
    lib_strlcpy(& hPciDrv->PCIDRV_cDrvName[0], ATA_PCI_DRV_NAME, PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = LW_NULL;                           /*  �豸������˽������          */
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;                           /*  ����������                */
    hPciDrv->PCIDRV_pfuncDevProbe  = pciStorageAtaDevProbe;             /*  �豸̽�⺯��, ����Ϊ��      */
    hPciDrv->PCIDRV_pfuncDevRemove = pciStorageAtaDevRemove;            /*  �����Ƴ�����, ����Ϊ��      */

    iRet = API_PciDrvRegister(hPciDrv);                                 /*  ע�� PCI �豸����           */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
                                                                        /*  (LW_CFG_DRV_ATA_IDE > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
