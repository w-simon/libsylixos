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
** ��   ��   ��: pciDev.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 09 �� 28 ��
**
** ��        ��: PCI ��������ģ��, �豸ͷ����.
*********************************************************************************************************/

#ifndef __PCI_DEV_H
#define __PCI_DEV_H

#include "pciBus.h"
#include "pciIds.h"
#include "pciDrv.h"
#include "pciPm.h"
#include "pciMsi.h"
#include "pciCap.h"
#include "pciError.h"
#include "pciCapExt.h"
#include "pciExpress.h"
#include "pciAutoCfg.h"

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

/*********************************************************************************************************
  PCI ����
*********************************************************************************************************/

#if LW_CFG_PCI_64 > 0
typedef UINT64          pci_addr_t;
typedef UINT64          pci_size_t;
#else
typedef UINT32          pci_addr_t;
typedef UINT32          pci_size_t;
#endif                                                                  /*  LW_CFG_PCI_64 > 0           */

/*********************************************************************************************************
   PCI Ӳ�����ӽṹ��ͼ
   
                      +-------+
                      |       |
                      |  CPU  |
                      |       |
                      +-------+
                          |
       Host bus           |
    ----------------------+-------------------------- ...
                          |
                 +------------------+
                 |     Bridge 0     |
                 |  (host adapter)  |
                 +------------------+
                          |
       PCI bus segment 0  |  (primary bus segment)
    ----------------------+-------------------------- ...
             |            |    |                  |
           dev 0          |   dev 1              dev 2
                          |
                 +------------------+
                 |     Bridge 1     |
                 |   (PCI to PCI)   |
                 +------------------+
                          |
       PCI bus segment 1  |  (secondary bus segment)
    ----------------------+-------------------------- ...
             |            |    |                  |
           dev 0          |   dev 1              dev 2
                          |
                 +------------------+
                 |     Bridge 2     |
                 |   (PCI to PCI)   |
                 +------------------+
                          |
       PCI bus segment 2  |  (tertiary bus segment)
    ----------------------+-------------------------- ...
             |                 |                  |
           dev 0              dev 1              dev 2
*********************************************************************************************************/
/*********************************************************************************************************
  PCI �豸ͷ
*********************************************************************************************************/

typedef struct {
    UINT16          PCID_usVendorId;                                    /* vendor ID                    */
    UINT16          PCID_usDeviceId;                                    /* device ID                    */
    UINT16          PCID_usCommand;                                     /* command register             */
    UINT16          PCID_usStatus;                                      /* status register              */
    
    UINT8           PCID_ucRevisionId;                                  /* revision ID                  */
    UINT8           PCID_ucProgIf;                                      /* programming interface        */
    UINT8           PCID_ucSubClass;                                    /* sub class code               */
    UINT8           PCID_ucClassCode;                                   /* class code                   */
    
    UINT8           PCID_ucCacheLine;                                   /* cache line                   */
    UINT8           PCID_ucLatency;                                     /* latency time                 */
    UINT8           PCID_ucHeaderType;                                  /* header type                  */
    UINT8           PCID_ucBist;                                        /* BIST                         */
    
    UINT32          PCID_uiBase0;                                       /* base address 0               */
    UINT32          PCID_uiBase1;                                       /* base address 1               */
    UINT32          PCID_uiBase2;                                       /* base address 2               */
    UINT32          PCID_uiBase3;                                       /* base address 3               */
    UINT32          PCID_uiBase4;                                       /* base address 4               */
    UINT32          PCID_uiBase5;                                       /* base address 5               */
    
    UINT32          PCID_uiCis;                                         /* cardBus CIS pointer          */
    UINT16          PCID_usSubVendorId;                                 /* sub system vendor ID         */
    UINT16          PCID_usSubSystemId;                                 /* sub system ID                */
    UINT32          PCID_uiRomBase;                                     /* expansion ROM base address   */

    UINT32          PCID_uiReserved0;                                   /* reserved                     */
    UINT32          PCID_uiReserved1;                                   /* reserved                     */

    UINT8           PCID_ucIntLine;                                     /* interrupt line               */
    UINT8           PCID_ucIntPin;                                      /* interrupt pin                */
    
    UINT8           PCID_ucMinGrant;                                    /* min Grant                    */
    UINT8           PCID_ucMaxLatency;                                  /* max Latency                  */
} PCI_DEV_HDR;

/*********************************************************************************************************
  PCI ���豸ͷ
*********************************************************************************************************/

typedef struct {
    UINT16          PCIB_usVendorId;                                    /* vendor ID                    */
    UINT16          PCIB_usDeviceId;                                    /* device ID                    */
    UINT16          PCIB_usCommand;                                     /* command register             */
    UINT16          PCIB_usStatus;                                      /* status register              */

    UINT8           PCIB_ucRevisionId;                                  /* revision ID                  */
    UINT8           PCIB_ucProgIf;                                      /* programming interface        */
    UINT8           PCIB_ucSubClass;                                    /* sub class code               */
    UINT8           PCIB_ucClassCode;                                   /* class code                   */
    
    UINT8           PCIB_ucCacheLine;                                   /* cache line                   */
    UINT8           PCIB_ucLatency;                                     /* latency time                 */
    UINT8           PCIB_ucHeaderType;                                  /* header type                  */
    UINT8           PCIB_ucBist;                                        /* BIST                         */

    UINT32          PCIB_uiBase0;                                       /* base address 0               */
    UINT32          PCIB_uiBase1;                                       /* base address 1               */

    UINT8           PCIB_ucPriBus;                                      /* primary bus number           */
    UINT8           PCIB_ucSecBus;                                      /* secondary bus number         */
    UINT8           PCIB_ucSubBus;                                      /* subordinate bus number       */
    UINT8           PCIB_ucSecLatency;                                  /* secondary latency timer      */
    UINT8           PCIB_ucIoBase;                                      /* IO base                      */
    UINT8           PCIB_ucIoLimit;                                     /* IO limit                     */

    UINT16          PCIB_usSecStatus;                                   /* secondary status             */

    UINT16          PCIB_usMemBase;                                     /* memory base                  */
    UINT16          PCIB_usMemLimit;                                    /* memory limit                 */
    UINT16          PCIB_usPreBase;                                     /* prefetchable memory base     */
    UINT16          PCIB_usPreLimit;                                    /* prefetchable memory limit    */

    UINT32          PCIB_uiPreBaseUpper;                                /* prefetchable memory base     */
                                                                        /* upper 32 bits                */
    UINT32          PCIB_uiPreLimitUpper;                               /* prefetchable memory limit    */
                                                                        /* upper 32 bits                */

    UINT16          PCIB_usIoBaseUpper;                                 /* IO base upper 16 bits        */
    UINT16          PCIB_usIoLimitUpper;                                /* IO limit upper 16 bits       */

    UINT32          PCIB_uiReserved;                                    /* reserved                     */
    UINT32          PCIB_uiRomBase;                                     /* expansion ROM base address   */

    UINT8           PCIB_ucIntLine;                                     /* interrupt line               */
    UINT8           PCIB_ucIntPin;                                      /* interrupt pin                */

    UINT16          PCIB_usControl;                                     /* bridge control               */
} PCI_BRG_HDR;

/*********************************************************************************************************
  PCI �����豸ͷ
*********************************************************************************************************/

typedef struct {
    UINT16          PCICB_usVendorId;                                   /* vendor ID                    */
    UINT16          PCICB_usDeviceId;                                   /* device ID                    */
    UINT16          PCICB_usCommand;                                    /* command register             */
    UINT16          PCICB_usStatus;                                     /* status register              */

    UINT8           PCICB_ucRevisionId;                                 /* revision ID                  */
    UINT8           PCICB_ucProgIf;                                     /* programming interface        */
    UINT8           PCICB_ucSubClass;                                   /* sub class code               */
    UINT8           PCICB_ucClassCode;                                  /* class code                   */
    
    UINT8           PCICB_ucCacheLine;                                  /* cache line                   */
    UINT8           PCICB_ucLatency;                                    /* latency time                 */
    UINT8           PCICB_ucHeaderType;                                 /* header type                  */
    UINT8           PCICB_ucBist;                                       /* BIST                         */
    
    UINT32          PCICB_uiBase0;                                      /* base address 0               */

    UINT8           PCICB_ucCapPtr;                                     /* capabilities pointer         */
    UINT8           PCICB_ucReserved;                                   /* reserved                     */

    UINT16          PCICB_usSecStatus;                                  /* secondary status             */

    UINT8           PCICB_ucPriBus;                                     /* primary bus number           */
    UINT8           PCICB_ucSecBus;                                     /* secondary bus number         */
    UINT8           PCICB_ucSubBus;                                     /* subordinate bus number       */
    UINT8           PCICB_ucSecLatency;                                 /* secondary latency timer      */

    UINT32          PCICB_uiMemBase0;                                   /* memory base 0                */
    UINT32          PCICB_uiMemLimit0;                                  /* memory limit 0               */
    UINT32          PCICB_uiMemBase1;                                   /* memory base 1                */
    UINT32          PCICB_uiMemLimit1;                                  /* memory limit 1               */

    UINT32          PCICB_uiIoBase0;                                    /* IO base 0                    */
    UINT32          PCICB_uiIoLimit0;                                   /* IO limit 0                   */
    UINT32          PCICB_uiIoBase1;                                    /* IO base 1                    */
    UINT32          PCICB_uiIoLimit1;                                   /* IO limit 1                   */

    UINT8           PCICB_ucIntLine;                                    /* interrupt line               */
    UINT8           PCICB_ucIntPin;                                     /* interrupt pin                */

    UINT16          PCICB_usControl;                                    /* bridge control               */
    UINT16          PCICB_usSubVendorId;                                /* sub system vendor ID         */
    UINT16          PCICB_usSubSystemId;                                /* sub system ID                */

    UINT32          PCICB_uiLegacyBase;                                 /* pccard 16bit legacy mode base*/
} PCI_CBUS_HDR;

/*********************************************************************************************************
  PCI ��׼�豸ͷ
*********************************************************************************************************/

typedef struct {
    UINT8               PCIH_ucType;                                    /*  PCI ����                    */
    /*
     * PCI_HEADER_TYPE_NORMAL
     * PCI_HEADER_TYPE_BRIDGE
     * PCI_HEADER_TYPE_CARDBUS
     */
    union {
        PCI_DEV_HDR     PCIHH_pcidHdr;
        PCI_BRG_HDR     PCIHH_pcibHdr;
        PCI_CBUS_HDR    PCIHH_pcicbHdr;
    } hdr;
#define PCIH_pcidHdr    hdr.PCIHH_pcidHdr
#define PCIH_pcibHdr    hdr.PCIHH_pcibHdr
#define PCIH_pcicbHdr   hdr.PCIHH_pcicbHdr
} PCI_HDR;

/*********************************************************************************************************
  PCI I/O Drv
*********************************************************************************************************/

typedef struct pci_drv_funcs0 {
    INT     (*cfgRead)(INT iBus, INT iSlot, INT iFunc, INT iOft, INT iLen, PVOID pvRet);
    INT     (*cfgWrite)(INT iBus, INT iSlot, INT iFunc, INT iOft, INT iLen, UINT32 uiData);
    INT     (*vpdRead)(INT iBus, INT iSlot, INT iFunc, INT iPos, UINT8 *pucBuf, INT iLen);
    INT     (*irqGet)(INT iBus, INT iSlot, INT iFunc, INT iMsiEn, INT iLine, INT iPin, ULONG *pulVector);
    INT     (*cfgSpcl)(INT iBus, UINT32 uiMsg);
} PCI_DRV_FUNCS0;                                                       /*  PCI_MECHANISM_0             */

typedef struct pci_drv_funcs12 {
    UINT8   (*ioInByte)(addr_t ulAddr);
    UINT16  (*ioInWord)(addr_t ulAddr);
    UINT32  (*ioInDword)(addr_t ulAddr);
    VOID    (*ioOutByte)(UINT8 ucValue, addr_t ulAddr);
    VOID    (*ioOutWord)(UINT16 usValue, addr_t ulAddr);
    VOID    (*ioOutDword)(UINT32 uiValue, addr_t ulAddr);
} PCI_DRV_FUNCS12;                                                      /*  PCI_MECHANISM_1 , 2         */

/*********************************************************************************************************
  The PCI interface treats multi-function devices as independent
  devices. The slot/function address of each device is encoded
  in a single byte as follows:
  7:3 = slot
  2:0 = function
*********************************************************************************************************/

#define PCI_SLOTFN(slot, func)  ((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_DEVFN(slot, func)   PCI_SLOTFN(slot, func)
#define PCI_SLOT(slotfn)        (((slotfn) >> 3) & 0x1f)
#define PCI_FUNC(slotfn)        ((slotfn) & 0x07)

/*********************************************************************************************************
  pack parameters for the PCI Configuration Address Register
*********************************************************************************************************/
#define PCI_PACKET(bus, slot, func) ((((bus) << 16) & 0x00ff0000) | \
                                    (((slot) << 11) & 0x0000f800) | \
                                    (((func) << 8)  & 0x00000700))

/*********************************************************************************************************
  T. Straumann, 7/31/2001: increased to 32 - PMC slots are not
  scanned on mvme2306 otherwise
*********************************************************************************************************/

#define PCI_MAX_BUS             256
#define PCI_MAX_SLOTS           32
#define PCI_MAX_FUNCTIONS       8

/*********************************************************************************************************
  Configuration I/O addresses for mechanism 1
*********************************************************************************************************/

#define PCI_CONFIG_ADDR         0x0cf8                                  /* write 32 bits to set address */
#define PCI_CONFIG_DATA         0x0cfc                                  /* 8, 16, or 32 bit accesses    */

/*********************************************************************************************************
  Configuration I/O addresses for mechanism 2
*********************************************************************************************************/

#define PCI_CONFIG_CSE          0x0cf8                                  /* CSE register                 */
#define PCI_CONFIG_FORWARD      0x0cfa                                  /* forward register             */
#define PCI_CONFIG_BASE         0xc000                                  /* base register                */

/*********************************************************************************************************
  ���ò���
*********************************************************************************************************/
#define PCI_DEBUG_EN            0                                       /* �Ƿ�ʹ�ܵ�����Ϣ             */

#define PCI_BARS_MAX            6                                       /* BAR �������                 */
#define PCI_CONFIG_LEN_MAX      256                                     /* ���ÿռ��С                 */

#define PCI_DEV_NAME_MAX        (32 + 1)                                /* �豸�������ֵ               */
#define PCI_DEV_IRQ_NAME_MAX    (32 + 1)                                /* �豸�ж��������ֵ           */

/*********************************************************************************************************
  ID ����
*********************************************************************************************************/
#define PCI_ANY_ID              (~0)                                    /* �κ� ID                      */

#define PCI_DEVICE(vend,dev)                            (vend),                 \
                                                        (dev),                  \
                                                        PCI_ANY_ID,             \
                                                        PCI_ANY_ID

#define PCI_DEVICE_SUB(vend, dev, subvend, subdev)      (vend),                 \
                                                        (dev),                  \
                                                        (subvend),              \
                                                        (subdev)

#define PCI_DEVICE_CLASS(dev_class,dev_class_mask)      PCI_ANY_ID,             \
                                                        PCI_ANY_ID,             \
                                                        PCI_ANY_ID,             \
                                                        PCI_ANY_ID,             \
                                                        (dev_class),            \
                                                        (dev_class_mask)        \

#define PCI_VDEVICE(vend, dev)                          PCI_VENDOR_ID_##vend,   \
                                                        (dev),                  \
                                                        PCI_ANY_ID,             \
                                                        PCI_ANY_ID,             \
                                                        0,                      \
                                                        0

/*********************************************************************************************************
  ����ģʽ����
*********************************************************************************************************/
#if PCI_DEBUG_EN > 0                                                    /* �Ƿ�ʹ�� PCI ����ģʽ        */
#define PCI_DEBUG_MSG                       _DebugHandle
#define PCI_DEBUG_MSG_FMT                   _DebugFormat
#else                                                                   /* PCI_DEBUG_EN                 */
#define PCI_DEBUG_MSG(level, msg)
#define PCI_DEBUG_MSG_FMT(level, fmt, ...)
#endif                                                                  /* PCI_DEBUG_EN                 */

/*********************************************************************************************************
  PCI access config
*********************************************************************************************************/

typedef struct {
    union {
        PCI_DRV_FUNCS0         *PCICF_pDrvFuncs0;
        PCI_DRV_FUNCS12        *PCICF_pDrvFuncs12;
    } f;
#define PCIC_pDrvFuncs0         f.PCICF_pDrvFuncs0
#define PCIC_pDrvFuncs12        f.PCICF_pDrvFuncs12
    
#define PCI_MECHANISM_0         0
#define PCI_MECHANISM_1         1
#define PCI_MECHANISM_2         2
    UINT8                       PCIC_ucMechanism;
    
    /*
     * �����ַ�������� PCI_MECHANISM_1 �� PCI_MECHANISM_2
     */
    addr_t                      PCIC_ulConfigAddr;
    addr_t                      PCIC_ulConfigData;
    addr_t                      PCIC_ulConfigBase;                      /* only for PCI_MECHANISM_2     */
    
    LW_OBJECT_HANDLE            PCIC_ulLock;

    INT                         PCIC_iIndex;
    INT                         PCIC_iBusMax;
} PCI_CONFIG;

/*********************************************************************************************************
  PCI �豸���ƿ� (ֻ���� PCI_HEADER_TYPE_NORMAL �����豸)
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        PDT_lineDevNode;                                /* �豸����ڵ�                 */

    UINT32              PDT_uiDevVersion;                               /* �豸�汾                     */
    UINT32              PDT_uiUnitNumber;                               /* �豸���                     */
    CHAR                PDT_cDevName[PCI_DEV_NAME_MAX];                 /* �豸����                     */
    INT                 PDT_iDevBus;                                    /* ���ߺ�                       */
    INT                 PDT_iDevDevice;                                 /* �豸��                       */
    INT                 PDT_iDevFunction;                               /* ���ܺ�                       */
    PCI_HDR             PDT_phDevHdr;                                   /* �豸ͷ                       */

    PVOID               PDT_pvDevBarBase[PCI_BARS_MAX];                 /* �� BAR ����ַ                */
    PVOID               PDT_pvDevBarBasePhys[PCI_BARS_MAX];             /* �� BAR �����ַ              */
    size_t              PDT_stDevBarBaseSize[PCI_BARS_MAX];             /* �� BAR ��С                  */
    UINT32              PDT_uiDevBarBaseFlags[PCI_BARS_MAX];            /* �� BAR ����                  */

    PVOID               PDT_pvDevDriver;                                /* �������                     */

    INT                 PDT_iDevIrqMsiEn;                               /* �Ƿ�ʹ�� MSI                 */
    ULONG               PDT_ulDevIrqVector;                             /* �ж�����                     */
    CHAR                PDT_cDevIrqName[PCI_DEV_IRQ_NAME_MAX];          /* �ж�����                     */
    PINT_SVR_ROUTINE    PDT_pfuncDevIrqHandle;                          /* �жϷ�����                 */
    PVOID               PDT_pvDevIrqArg;                                /* �жϷ������                 */

    LW_OBJECT_HANDLE    PDT_hDevLock;                                   /* �豸���������               */
} PCI_DEV_TCB;
typedef PCI_DEV_TCB    *PCI_DEV_HANDLE;

/*********************************************************************************************************
  ����֧���豸�б���ƿ�
*********************************************************************************************************/
typedef struct {
    UINT32      PDIT_uiVendor;                                          /* ���� ID                      */
    UINT32      PDIT_uiDevice;                                          /* �豸 ID                      */

    UINT32      PDIT_uiSubVendor;                                       /* �ӳ��� ID                    */
    UINT32      PDIT_uiSubDevice;                                       /* ���豸 ID                    */

    UINT32      PDIT_uiClass;                                           /* �豸��                       */
    UINT32      PDIT_uiClassMask;                                       /* �豸����                     */

    ULONG       PDIT_ulData;                                            /* �豸˽������                 */
} PCI_DEVICE_ID_TCB;
typedef PCI_DEVICE_ID_TCB      *PCI_DEVICE_ID_HANDLE;

/*********************************************************************************************************
  ����ע����ƿ�
*********************************************************************************************************/
typedef struct {
    CPCHAR                  PDRT_pcName;                                /* ��������                     */
    PVOID                   PDRT_pvHandle;                              /* �ض����                     */
    PCI_DEVICE_ID_HANDLE    PDRT_hIdTable;                              /* �豸֧���б�                 */
    UINT32                  PDRT_uiIdTableSize;                         /* �豸֧���б��С             */

    /*
     *  �������ú���, PDRT_pfuncProbe �� PDRT_pfuncRemove ����Ϊ LW_NULL, ������ѡ
     */
    INT   (*PDRT_pfuncProbe)(PCI_DEV_HANDLE hHandle, const PCI_DEVICE_ID_HANDLE hIdEntry);
    VOID  (*PDRT_pfuncRemove)(PCI_DEV_HANDLE hHandle);
    INT   (*PDRT_pfuncSuspend)(PCI_DEV_HANDLE hHandle, PCI_PM_MESSAGE_HANDLE hPmMsg);
    INT   (*PDRT_pfuncSuspendLate)(PCI_DEV_HANDLE hHandle, PCI_PM_MESSAGE_HANDLE hPmMsg);
    INT   (*PDRT_pfuncResumeEarly)(PCI_DEV_HANDLE hHandle);
    INT   (*PDRT_pfuncResume)(PCI_DEV_HANDLE hHandle);
    VOID  (*PDRT_pfuncShutdown)(PCI_DEV_HANDLE hHandle);

    PCI_ERROR_HANDLE        PDRT_hErrorHandler;                         /* ��������                 */

} PCI_DRV_REGISTER_TCB;
typedef PCI_DRV_REGISTER_TCB       *PCI_DRV_REGISTER_HANDLE;

/*********************************************************************************************************
  ���������豸���ƿ�
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            PDDT_lineDrvDevNode;                        /* �豸�ڵ����                 */

    PCI_DEV_HANDLE          PDDT_hDrvDevHandle;                         /* �豸���                     */
} PCI_DRV_DEV_TCB;
typedef PCI_DRV_DEV_TCB     *PCI_DRV_DEV_HANDLE;

/*********************************************************************************************************
  �������ƿ�
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            PDT_lineDrvNode;                            /* �����ڵ����                 */
    CHAR                    PDT_cDrvName[PCI_DRV_NAME_MAX];             /* ��������                     */
    PCI_DEVICE_ID_HANDLE    PDT_hDrvIdTable;                            /* �豸֧���б�                 */
    UINT32                  PDT_uiDrvIdTableSize;                       /* �豸֧���б��С             */

    /*
     *  �������ú���, PDRT_pfuncProbe �� PDRT_pfuncRemove ����Ϊ LW_NULL, ������ѡ
     */
    INT   (*PDT_pfuncDrvProbe)(PCI_DEV_HANDLE hHandle, const PCI_DEVICE_ID_HANDLE hIdEntry);
    VOID  (*PDT_pfuncDrvRemove)(PCI_DEV_HANDLE hHandle);
    INT   (*PDT_pfuncDrvSuspend)(PCI_DEV_HANDLE hHandle, PCI_PM_MESSAGE_HANDLE hPmMsg);
    INT   (*PDT_pfuncDrvSuspendLate)(PCI_DEV_HANDLE hHandle, PCI_PM_MESSAGE_HANDLE hPmMsg);
    INT   (*PDT_pfuncDrvResumeEarly)(PCI_DEV_HANDLE hHandle);
    INT   (*PDT_pfuncDrvResume)(PCI_DEV_HANDLE hHandle);
    VOID  (*PDT_pfuncDrvShutdown)(PCI_DEV_HANDLE hHandle);

    PCI_ERROR_HANDLE        PDT_hDrvErrHandler;                         /* ��������                 */

    INT                     PDT_iDrvFlag;                               /* ������־                     */
    UINT32                  PDT_uiDrvDevNum;                            /* �����豸��                   */
    LW_LIST_LINE_HEADER     PDT_plineDrvDevHeader;                      /* �豸��������ͷ               */
} PCI_DRV_TCB;
typedef PCI_DRV_TCB    *PCI_DRV_HANDLE;

/*********************************************************************************************************
  API
  API_PciConfigInit() ������ BSP ��ʼ������ϵͳʱ������, ���ұ��뱣֤�ǵ�һ������ȷ���õ� PCI ϵͳ����.
*********************************************************************************************************/

LW_API INT          API_PciConfigInit(PCI_CONFIG *ppcicfg);
LW_API VOID         API_PciConfigReset(INT  iRebootType);

LW_API INT          API_PciLock(VOID);
LW_API INT          API_PciUnlock(VOID);

LW_API INT          API_PciConfigInByte(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 *pucValue);
LW_API INT          API_PciConfigInWord(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 *pusValue);
LW_API INT          API_PciConfigInDword(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 *puiValue);

LW_API INT          API_PciConfigOutByte(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 ucValue);
LW_API INT          API_PciConfigOutWord(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 usValue);
LW_API INT          API_PciConfigOutDword(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 uiValue);

LW_API INT          API_PciConfigModifyByte(INT iBus, INT iSlot, INT iFunc, INT iOft,
                                            UINT8 ucMask, UINT8 ucValue);
LW_API INT          API_PciConfigModifyWord(INT iBus, INT iSlot, INT iFunc, INT iOft,
                                            UINT16 usMask, UINT16 usValue);
LW_API INT          API_PciConfigModifyDword(INT iBus, INT iSlot, INT iFunc, INT iOft,
                                             UINT32 uiMask, UINT32 uiValue);
                                         
LW_API INT          API_PciSpecialCycle(INT iBus, UINT32 uiMsg);

LW_API INT          API_PciFindDev(UINT16 usVendorId, UINT16 usDeviceId, INT  iInstance,
                                   INT *piBus, INT *piSlot, INT *piFunc);
LW_API INT          API_PciFindClass(UINT16  usClassCode, INT  iInstance,
                                     INT *piBus, INT *piSlot, INT *piFunc);
                                 
LW_API INT          API_PciTraversal(INT (*pfuncCall)(), PVOID pvArg, INT iMaxBusNum);
LW_API INT          API_PciTraversalDev(INT iBusStart, BOOL bSubBus, INT (*pfuncCall)(), PVOID pvArg);
                                 
LW_API INT          API_PciConfigDev(INT iBus, INT iSlot, INT iFunc,
                                     ULONG ulIoBase, pci_addr_t ulMemBase,
                                     UINT8 ucLatency, UINT32 uiCommand);

LW_API INT          API_PciFuncDisable(INT iBus, INT iSlot, INT iFunc);

LW_API INT          API_PciInterConnect(ULONG ulVector, PINT_SVR_ROUTINE pfuncIsr,
                                        PVOID pvArg, CPCHAR pcName);
LW_API INT          API_PciInterDisconnect(ULONG ulVector, PINT_SVR_ROUTINE pfuncIsr,
                                           PVOID pvArg);

LW_API INT          API_PciGetHeader(INT iBus, INT iSlot, INT iFunc, PCI_HDR *p_pcihdr);
LW_API INT          API_PciHeaderTypeGet(INT iBus, INT iSlot, INT iFunc, UINT8 *ucType);

LW_API INT          API_PciVpdRead(INT iBus, INT iSlot, INT iFunc, INT iPos, UINT8 *pucBuf, INT iLen);
LW_API INT          API_PciIrqGet(INT iBus, INT iSlot, INT iFunc,
                                  INT iMsiEn, INT iLine, INT iPin, ULONG *pulVector);

LW_API INT          API_PciConfigFetch(INT iBus, INT iSlot, INT iFunc, UINT uiPos, UINT uiLen);

LW_API PCI_CONFIG  *API_PciConfigHandleGet(INT iIndex);
LW_API INT          API_PciConfigIndexGet(PCI_CONFIG *ppcHandle);
LW_API INT          API_PciConfigBusMaxSet(INT iIndex, UINT32 uiBusMax);
LW_API INT          API_PciConfigBusMaxGet(INT iIndex);

LW_API INT          API_PciIntxEnableSet(INT iBus, INT iSlot, INT iFunc, INT iEnable);
LW_API INT          API_PciIntxMaskSupported(INT iBus, INT iSlot, INT iFunc, INT *piSupported);

LW_API VOID                 API_PciDrvBindEachDev(PCI_DRV_HANDLE hDrvHandle);
LW_API INT                  API_PciDrvLoad(PCI_DRV_HANDLE       hDrvHandle,
                                           PCI_DEV_HANDLE       hDevHandle,
                                           PCI_DEVICE_ID_HANDLE hIdEntry);
LW_API PCI_DRV_DEV_HANDLE   API_PciDrvDevFind(PCI_DRV_HANDLE hDrvHandle, PCI_DEV_HANDLE hDevHandle);
LW_API INT                  API_PciDrvDevDel(PCI_DRV_HANDLE hDrvHandle, PCI_DEV_HANDLE hDevHandle);
LW_API INT                  API_PciDrvDevAdd(PCI_DRV_HANDLE hDrvHandle, PCI_DEV_HANDLE hDevHandle);
LW_API PCI_DRV_HANDLE       API_PciDrvHandleGet(CPCHAR pcName);
LW_API INT                  API_PciDrvDelete(PCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_PciDrvRegister(PCI_DRV_REGISTER_HANDLE hHandle);
LW_API INT                  API_PciDrvInit(VOID);

LW_API PCI_DEVICE_ID_HANDLE API_PciDevMatchDrv(PCI_DEV_HANDLE hDevHandle, PCI_DRV_HANDLE hDrvHandle);
LW_API VOID                 API_PciDevBindEachDrv(PCI_DEV_HANDLE hDevHandle);
LW_API INT                  API_PciDevInterDisable(PCI_DEV_HANDLE   hHandle,
                                                   ULONG            ulVector,
                                                   PINT_SVR_ROUTINE pfuncIsr,
                                                   PVOID            pvArg);
LW_API INT                  API_PciDevInterEnable(PCI_DEV_HANDLE   hHandle,
                                                  ULONG            ulVector,
                                                  PINT_SVR_ROUTINE pfuncIsr,
                                                  PVOID            pvArg);
LW_API INT                  API_PciDevInterDisonnect(PCI_DEV_HANDLE   hHandle,
                                                     ULONG            ulVector,
                                                     PINT_SVR_ROUTINE pfuncIsr,
                                                     PVOID            pvArg);
LW_API INT                  API_PciDevInterConnect(PCI_DEV_HANDLE   hHandle,
                                                   ULONG            ulVector,
                                                   PINT_SVR_ROUTINE pfuncIsr,
                                                   PVOID            pvArg,
                                                   CPCHAR           pcName);
LW_API INT                  API_PciDevInterVectorGet(PCI_DEV_HANDLE  hHandle, ULONG *pulVector);

LW_API INT                  API_PciDevConfigRead(PCI_DEV_HANDLE hHandle,
                                                 INT iPos, UINT8 *pucBuf, INT iLen);
LW_API INT                  API_PciDevConfigWrite(PCI_DEV_HANDLE hHandle,
                                                  INT iPos, UINT8 *pucBuf, INT iLen);
LW_API PCI_DEV_HANDLE       API_PciDevHandleGet(INT iBus, INT iDevice, INT iFunction);
LW_API PCI_DEV_HANDLE       API_PciDevAdd(INT iBus, INT iDevice, INT iFunction);
LW_API INT                  API_PciDevDelete(PCI_DEV_HANDLE hHandle);
LW_API INT                  API_PciDevDrvDel(PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_PciDevDrvUpdate(PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_PciDevListCreate(VOID);
LW_API INT                  API_PciDevInit(VOID);

LW_API INT                  API_PciDevMsiEnableGet(PCI_DEV_HANDLE  hHandle, INT *piEnable);

#define pciConfigInit           API_PciConfigInit
#define pciConfigReset          API_PciConfigReset

#define pciLock                 API_PciLock
#define pciUnlock               API_PciUnlock

#define pciConfigInByte         API_PciConfigInByte
#define pciConfigInWord         API_PciConfigInWord
#define pciConfigInDword        API_PciConfigInDword

#define pciConfigOutByte        API_PciConfigOutByte
#define pciConfigOutWord        API_PciConfigOutWord
#define pciConfigOutDword       API_PciConfigOutDword

#define pciConfigModifyByte     API_PciConfigModifyByte
#define pciConfigModifyWord     API_PciConfigModifyWord
#define pciConfigModifyDword    API_PciConfigModifyDword

#define pciSpecialCycle         API_PciSpecialCycle

#define pciFindDev              API_PciFindDev
#define pciFindClass            API_PciFindClass

#define pciTraversal            API_PciTraversal
#define pciTraversalDev         API_PciTraversalDev

#define pciConfigDev            API_PciConfigDev
#define pciFuncDisable          API_PciFuncDisable

#define pciInterDisable         API_PciInterDisable
#define pciInterEnable          API_PciInterEnable
#define pciInterConnect         API_PciInterConnect
#define pciInterDisconnect      API_PciInterDisconnect

#define pciGetHeader            API_PciGetHeader
#define pciHeaderTypeGet        API_PciHeaderTypeGet

#define pciVpdRead              API_PciVpdRead
#define pciIrqGet               API_PciIrqGet

#define pciConfigFetch          API_PciConfigFetch

#define pciConfigHandleGet      API_PciConfigHandleGet
#define pciConfigIndexGet       API_PciConfigIndexGet
#define pciConfigBusMaxSet      API_PciConfigBusMaxSet
#define pciConfigBusMaxGet      API_PciConfigBusMaxGet

#define pciIntxEnableSet        API_PciIntxEnableSet
#define pciIntxMaskSupported    API_PciIntxMaskSupported

#define pciDrvLoad              API_PciDrvLoad
#define pciDrvDevFind           API_PciDrvDevFind
#define pciDrvDevDel            API_PciDrvDevDel
#define pciDrvDevAdd            API_PciDrvDevAdd
#define pciDrvHandleGet         API_PciDrvHandleGet
#define pciDrvDelete            API_PciDrvDelete
#define pciDrvRegister          API_PciDrvRegister
#define pciDrvInit              API_PciDrvInit

#define pciDevMatchDrv          API_PciDevMatchDrv
#define pciDevBindEachDrv       API_PciDevBindEachDrv
#define pciDevInterDisable      API_PciDevInterDisable
#define pciDevInterEnable       API_PciDevInterEnable
#define pciDevInterDisonnect    API_PciDevInterDisonnect
#define pciDevInterConnect      API_PciDevInterConnect
#define pciDevInterVectorGet    API_PciDevInterVectorGet
#define pciDevConfigRead        API_PciDevConfigRead
#define pciDevConfigWrite       API_PciDevConfigWrite
#define pciHandleGet            API_PciDevHandleGet
#define pciDevAdd               API_PciDevAdd
#define pciDevDelete            API_PciDevDelete
#define pciDevDrvDel            API_PciDevDrvDel
#define pciDevDrvUpdate         API_PciDevDrvUpdate
#define pciDevListCreate        API_PciDevListCreate
#define pciDevInit              API_PciDevInit

#define pciDevMsiEnableGet      API_PciDevMsiEnableGet

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCI_DEV_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
