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
** ��   ��   ��: pciDrv.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2015 �� 12 �� 31 ��
**
** ��        ��: PCI �����豸��������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

#include "pciDev.h"
#include "pciDrv.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT                     _GuiPciDrvTotalNum  = 0;
static UINT                     _GuiPciDrvActiveNum = 0;
static LW_LIST_LINE_HEADER      _GplinePciDrvHeader = LW_NULL;
static LW_OBJECT_HANDLE         _GulPciDrvLock      = LW_OBJECT_HANDLE_INVALID;

#define __PCI_DRV_LOCK()        API_SemaphoreMPend(_GulPciDrvLock, LW_OPTION_WAIT_INFINITE)
#define __PCI_DRV_UNLOCK()      API_SemaphoreMPost(_GulPciDrvLock)
/*********************************************************************************************************
** ��������: __tshellPciDrvDevDel
** ��������: ɾ�������������豸
** �䡡��  : cpcName        ��������
**           iAll           �Ƿ�Ϊ����ȫ���豸
**           iBus           ���ߺ�
**           iDevice        �豸��
**           iFunction      ���ܺ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellPciDrvDevDel (CPCHAR  cpcName, INT iAll, INT  iBus, INT  iDevice, INT  iFunction)
{
    INT                     iRet = PX_ERROR;
    PCI_DRV_HANDLE          hDrvHandle;
    PCI_DEV_HANDLE          hDevHandle;
    PCI_DRV_DEV_HANDLE      hDrvDevHandle;

    hDrvHandle = API_PciDrvHandleGet(cpcName);
    if (hDrvHandle == LW_NULL) {
        goto  __error_handle;
    }

    if (iAll == LW_FALSE) {
        hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);
        if (hDevHandle == LW_NULL) {
            goto  __error_handle;
        }
        hDrvDevHandle = API_PciDrvDevFind(hDrvHandle, hDevHandle);
        if (hDrvDevHandle == LW_NULL) {
            goto  __error_handle;
        }
    } else {
        hDevHandle = LW_NULL;
    }

    iRet = API_PciDrvDevDel(hDrvHandle, hDevHandle);
    if (iRet != ERROR_NONE) {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "delete pci device error.\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __tshellPciDrvCmdShow
** ��������: ��ӡ PCI �豸�����б�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tshellPciDrvCmdShow (VOID)
{
    static PCHAR        pcPciDrvShowHdr = \
    " INDEX          DRVNAME           INDEX   BUS   DEV   FUNC  VENDOR DEVICE  SUBV   SUBD   TOTAL\n"
    "------- ------------------------ ------- ----- ----- ------ ------ ------ ------ ------ -------\n";

    PLW_LIST_LINE       plineDrvTemp  = LW_NULL;
    PLW_LIST_LINE       plineDevTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle    = LW_NULL;
    PCI_DRV_HANDLE      hDrvHandle    = LW_NULL;
    PCI_DRV_DEV_HANDLE  hDrvDevHandle = LW_NULL;
    REGISTER INT        i;
    REGISTER INT        j;

    printf("pci device driver total : %d active : %d.\n", _GuiPciDrvTotalNum, _GuiPciDrvActiveNum);

    printf(pcPciDrvShowHdr);

    __PCI_DRV_LOCK();
    i = 0;
    for (plineDrvTemp  = _GplinePciDrvHeader;
         plineDrvTemp != LW_NULL;
         plineDrvTemp  = _list_line_get_next(plineDrvTemp)) {
        hDrvHandle = _LIST_ENTRY(plineDrvTemp, PCI_DRV_TCB, PDT_lineDrvNode);
        printf("%7d %-24s %-7s %-5s %-5s %-6s %-6s %-6s %-6s %-6s %7d\n",
               i,
               hDrvHandle->PDT_cDrvName,
               "", "", "", "", "", "", "", "",
               hDrvHandle->PDT_uiDrvDevNum);

        j = 0;
        for (plineDevTemp  = hDrvHandle->PDT_plineDrvDevHeader;
             plineDevTemp != LW_NULL;
             plineDevTemp  = _list_line_get_next(plineDevTemp)) {
            hDrvDevHandle = _LIST_ENTRY(plineDevTemp, PCI_DRV_DEV_TCB, PDDT_lineDrvDevNode);
            hDevHandle = hDrvDevHandle->PDDT_hDrvDevHandle;
            printf("%-7s %-24s %7d %5d %5d %6d %6x %6x %6x %6x\n",
                   "", "",
                   j,
                   hDevHandle->PDT_iDevBus,
                   hDevHandle->PDT_iDevDevice,
                   hDevHandle->PDT_iDevFunction,
                   hDevHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_usVendorId,
                   hDevHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_usDeviceId,
                   hDevHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_usSubVendorId,
                   hDevHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_usSubSystemId);
            j += 1;
        }

        i += 1;
    }
    __PCI_DRV_UNLOCK();

    fflush(stdout);                                                     /*  ������                    */
}
/*********************************************************************************************************
** ��������: __tshellPciDrvCmd
** ��������: PCI ���� "pcidrv"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellPciDrvCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iRet;
    INT     iBus;
    INT     iDevice;
    INT     iFunction;
    INT     iAllFlag = LW_FALSE;

    if (iArgC == 1) {
        __tshellPciDrvCmdShow();

        return  (ERROR_NONE);
    }

    if ((lib_strcmp(ppcArgV[1], "add") == 0) && (iArgC == 4)) {
        /*
         *  TODO:
         */
        goto  __error_handle;
    } else if ((lib_strcmp(ppcArgV[1], "del") == 0) && (iArgC == 4)) {
        if (lib_strcmp(ppcArgV[3], "all") == 0) {
            iAllFlag = LW_TRUE;
        } else {
            iAllFlag = LW_FALSE;
            iRet = sscanf(ppcArgV[3], "%d:%d.%d", &iBus, &iDevice, &iFunction);
            if (iRet != 3) {
                fprintf(stderr, "pci device address format error.\n");
                goto  __error_handle;
            }
        }

        iRet = __tshellPciDrvDevDel(ppcArgV[2], iAllFlag, iBus, iDevice, iFunction);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    } else {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "argments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** ��������: API_PciDrvLoad
** ��������: ���� PCI ����
** �䡡��  : hDrvHandle     �������
**           hDevHandle     �豸���
**           hIdEntry       ��ǰƥ��ɹ���һ�� PCI ID ��Ŀ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDrvLoad (PCI_DRV_HANDLE       hDrvHandle,
                     PCI_DEV_HANDLE       hDevHandle,
                     PCI_DEVICE_ID_HANDLE hIdEntry)
{
    INT     iRet;

    if ((hDrvHandle == LW_NULL) ||
        (hDevHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    if (!hIdEntry) {
        hIdEntry = hDrvHandle->PDT_hDrvIdTable;
    }

    iRet = hDrvHandle->PDT_pfuncDrvProbe(hDevHandle, hIdEntry);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciDevDrvUpdate(hDevHandle, hDrvHandle);
    if (iRet != ERROR_NONE) {
        hDrvHandle->PDT_pfuncDrvRemove(hDevHandle);
        return  (PX_ERROR);
    }

    iRet = API_PciDrvDevAdd(hDrvHandle, hDevHandle);
    if (iRet != ERROR_NONE) {
        API_PciDevDrvDel( hDevHandle, hDrvHandle);
        hDrvHandle->PDT_pfuncDrvRemove(hDevHandle);
        return  (PX_ERROR);
    }

    if (!(hDrvHandle->PDT_iDrvFlag & PCI_DRV_FLAG_ACTIVE)) {
        hDrvHandle->PDT_iDrvFlag |= PCI_DRV_FLAG_ACTIVE;
        __PCI_DRV_LOCK();
        _GuiPciDrvActiveNum += 1;
        __PCI_DRV_UNLOCK();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDrvDevFind
** ��������: ��ѯһ�������ϵ�ָ���豸
** �䡡��  : hDrvHandle     �������
**           hDevHandle     �豸���
** �䡡��  : �����豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCI_DRV_DEV_HANDLE  API_PciDrvDevFind (PCI_DRV_HANDLE  hDrvHandle, PCI_DEV_HANDLE  hDevHandle)
{
    PLW_LIST_LINE           plineTemp     = LW_NULL;
    PCI_DRV_DEV_HANDLE      hDrvDevHandle = LW_NULL;

    hDrvDevHandle = LW_NULL;
    __PCI_DRV_LOCK();
    for (plineTemp  = hDrvHandle->PDT_plineDrvDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrvDevHandle = _LIST_ENTRY(plineTemp, PCI_DRV_DEV_TCB, PDDT_lineDrvDevNode);
        if (hDrvDevHandle->PDDT_hDrvDevHandle == hDevHandle) {
            break;
        }
    }
    __PCI_DRV_UNLOCK();

    if (plineTemp) {
        return  (hDrvDevHandle);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_PciDrvDevDel
** ��������: ָ�� PCI ����ɾ��һ���豸
** �䡡��  : hDrvHandle     �������ƿ���
**           hDevHandle     �豸���ƿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDrvDevDel (PCI_DRV_HANDLE  hDrvHandle, PCI_DEV_HANDLE  hDevHandle)
{
    PLW_LIST_LINE           plineTemp = LW_NULL;
    PCI_DRV_DEV_HANDLE      hDrvDevHandle = LW_NULL;

    if (hDrvHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (hDevHandle == LW_NULL) {
        __PCI_DRV_UNLOCK();
        for (plineTemp  = hDrvHandle->PDT_plineDrvDevHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            hDrvDevHandle = _LIST_ENTRY(plineTemp, PCI_DRV_DEV_TCB, PDDT_lineDrvDevNode);
            _List_Line_Del(&hDrvDevHandle->PDDT_lineDrvDevNode, &hDrvHandle->PDT_plineDrvDevHeader);
            hDrvHandle->PDT_uiDrvDevNum -= 1;
            if (hDrvHandle->PDT_uiDrvDevNum < 1) {
                hDrvHandle->PDT_iDrvFlag &= ~(PCI_DRV_FLAG_ACTIVE);
                hDrvHandle->PDT_plineDrvDevHeader = LW_NULL;
            }
            API_PciDevDrvDel(hDrvDevHandle->PDDT_hDrvDevHandle, hDrvHandle);
            hDrvHandle->PDT_pfuncDrvRemove(hDrvDevHandle->PDDT_hDrvDevHandle);
            hDrvDevHandle->PDDT_hDrvDevHandle = LW_NULL;
            __SHEAP_FREE(hDrvDevHandle);
        }
        __PCI_DRV_UNLOCK();

        API_PciDrvDelete(hDrvHandle);

        return  (ERROR_NONE);
    }

    hDrvDevHandle = API_PciDrvDevFind(hDrvHandle, hDevHandle);
    if (hDrvDevHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    __PCI_DRV_LOCK();
    _List_Line_Del(&hDrvDevHandle->PDDT_lineDrvDevNode, &hDrvHandle->PDT_plineDrvDevHeader);
    hDrvHandle->PDT_uiDrvDevNum -= 1;
    if (hDrvHandle->PDT_uiDrvDevNum < 1) {
        hDrvHandle->PDT_iDrvFlag &= ~(PCI_DRV_FLAG_ACTIVE);
        hDrvHandle->PDT_plineDrvDevHeader = LW_NULL;
    }
    API_PciDevDrvDel(hDrvDevHandle->PDDT_hDrvDevHandle, hDrvHandle);
    hDrvHandle->PDT_pfuncDrvRemove(hDrvDevHandle->PDDT_hDrvDevHandle);
    hDrvDevHandle->PDDT_hDrvDevHandle = LW_NULL;
    __PCI_DRV_UNLOCK();

    __SHEAP_FREE(hDrvDevHandle);

    API_PciDrvDelete(hDrvHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDrvDevAdd
** ��������: ָ�� PCI ��������һ���豸
** �䡡��  : hDrvHandle     �������ƿ���
**           hDevHandle     �豸���ƿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDrvDevAdd (PCI_DRV_HANDLE  hDrvHandle, PCI_DEV_HANDLE  hDevHandle)
{
    PCI_DRV_DEV_HANDLE      hDrvDevHandle = LW_NULL;

    if ((hDrvHandle == LW_NULL) ||
        (hDevHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    hDrvDevHandle = API_PciDrvDevFind(hDrvHandle, hDevHandle);
    if (hDrvDevHandle != LW_NULL) {
        return  (ERROR_NONE);
    }

    hDrvDevHandle = (PCI_DRV_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(PCI_DRV_DEV_TCB));
    if (hDrvDevHandle == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    __PCI_DRV_LOCK();
    hDrvDevHandle->PDDT_hDrvDevHandle = hDevHandle;
    _List_Line_Add_Ahead(&hDrvDevHandle->PDDT_lineDrvDevNode, &hDrvHandle->PDT_plineDrvDevHeader);
    hDrvHandle->PDT_uiDrvDevNum += 1;
    __PCI_DRV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDrvHandleGet
** ��������: ��ȡһ�������ľ��
** �䡡��  : cpcName       ��������
** �䡡��  : �豸���ƾ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PCI_DRV_HANDLE  API_PciDrvHandleGet (CPCHAR  cpcName)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DRV_HANDLE      hDrvHandle = LW_NULL;

    hDrvHandle = LW_NULL;
    __PCI_DRV_LOCK();
    for (plineTemp  = _GplinePciDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrvHandle = _LIST_ENTRY(plineTemp, PCI_DRV_TCB, PDT_lineDrvNode);
        if (lib_strncmp(&hDrvHandle->PDT_cDrvName[0], cpcName, PCI_DRV_NAME_MAX) == 0) {
            break;
        }
    }
    __PCI_DRV_UNLOCK();

    if (plineTemp) {
        return  (hDrvHandle);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_PciDrvDelete
** ��������: ɾ��һ�� PCI ����
** �䡡��  : hDrvHandle     �������ƿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_PciDrvDelete (PCI_DRV_HANDLE  hDrvHandle)
{
    if (hDrvHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if ((hDrvHandle->PDT_iDrvFlag & PCI_DRV_FLAG_ACTIVE) ||
        (hDrvHandle->PDT_uiDrvDevNum > 0)) {
        return  (PX_ERROR);
    }

    __PCI_DRV_LOCK();
    if (!(hDrvHandle->PDT_iDrvFlag & PCI_DRV_FLAG_ACTIVE)) {
        _GuiPciDrvActiveNum -= 1;
    }
    _List_Line_Del(&hDrvHandle->PDT_lineDrvNode, &_GplinePciDrvHeader);
    _GuiPciDrvTotalNum -= 1;
    __PCI_DRV_UNLOCK();

    __SHEAP_FREE(hDrvHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDrvRegister
** ��������: ע�� PCI ����
** �䡡��  : hRegiser       ����ע����ƿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDrvRegister (PCI_DRV_REGISTER_HANDLE  hHandle)
{
    PCI_DRV_HANDLE      hDrvHandle = LW_NULL;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if ((hHandle->PDRT_pcName        == LW_NULL) ||
        (hHandle->PDRT_uiIdTableSize <  1      ) ||
        (hHandle->PDRT_hIdTable      == LW_NULL) ||
        (hHandle->PDRT_pfuncProbe    == LW_NULL) ||
        (hHandle->PDRT_pfuncRemove   == LW_NULL)) {
        return  (PX_ERROR);
    }

    hDrvHandle = API_PciDrvHandleGet(hHandle->PDRT_pcName);
    if (hDrvHandle != LW_NULL) {
        return  (PX_ERROR);
    }

    hDrvHandle = (PCI_DRV_HANDLE)__SHEAP_ZALLOC(sizeof(PCI_DRV_TCB));
    if (hDrvHandle == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    lib_strncpy(&hDrvHandle->PDT_cDrvName[0], hHandle->PDRT_pcName, PCI_DRV_NAME_MAX);
    hDrvHandle->PDT_hDrvIdTable = hHandle->PDRT_hIdTable;
    hDrvHandle->PDT_uiDrvIdTableSize = hHandle->PDRT_uiIdTableSize;

    hDrvHandle->PDT_pfuncDrvProbe       = hHandle->PDRT_pfuncProbe;
    hDrvHandle->PDT_pfuncDrvRemove      = hHandle->PDRT_pfuncRemove;
    hDrvHandle->PDT_pfuncDrvSuspend     = hHandle->PDRT_pfuncSuspend;
    hDrvHandle->PDT_pfuncDrvSuspendLate = hHandle->PDRT_pfuncSuspendLate;
    hDrvHandle->PDT_pfuncDrvResumeEarly = hHandle->PDRT_pfuncResumeEarly;
    hDrvHandle->PDT_pfuncDrvResume      = hHandle->PDRT_pfuncResume;
    hDrvHandle->PDT_pfuncDrvShutdown    = hHandle->PDRT_pfuncShutdown;

    hDrvHandle->PDT_hDrvErrHandler = hHandle->PDRT_hErrorHandler;

    hDrvHandle->PDT_iDrvFlag &= ~(PCI_DRV_FLAG_ACTIVE);
    hDrvHandle->PDT_uiDrvDevNum = 0;
    hDrvHandle->PDT_plineDrvDevHeader = LW_NULL;

    __PCI_DRV_LOCK();
    _List_Line_Add_Ahead(&hDrvHandle->PDT_lineDrvNode, &_GplinePciDrvHeader);
    _GuiPciDrvTotalNum += 1;
    if (hDrvHandle->PDT_iDrvFlag & PCI_DRV_FLAG_ACTIVE) {
        _GuiPciDrvActiveNum += 1;
    }

    API_PciDrvBindEachDev(hDrvHandle);                                  /*  ���԰��豸                */
    __PCI_DRV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciDevBindEachDrv
** ��������: ��һ���µ�PCI�豸, ������ϵͳ�е��������а�(�������Ȳ��֧��)
** �䡡��  : hDevHandle     �豸���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_PciDevBindEachDrv (PCI_DEV_HANDLE hDevHandle)
{
    PCI_DRV_HANDLE        hDrvCurr;
    PLW_LIST_LINE         plineTemp;
    PCI_DEVICE_ID_HANDLE  hId;
    INT                   iRet;

    if (hDevHandle->PDT_pvDevDriver) {
        return;
    }

    __PCI_DRV_LOCK();
    for (plineTemp  = _GplinePciDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        hDrvCurr = _LIST_ENTRY(plineTemp, PCI_DRV_TCB, PDT_lineDrvNode);

        hId = API_PciDevMatchDrv(hDevHandle, hDrvCurr);
        if (!hId) {
            continue;                                                   /*  ID ��ƥ��                   */
        }

        /*
         * ���̽��ʧ��, ��ֱ�ӷ���. ��Ϊϵͳ�п��ܻ����ں��ʵ�����
         * ����ȫ�� PCI �豸������д�߾���
         */
        iRet = API_PciDrvLoad(hDrvCurr, hDevHandle, hId);
        if (iRet == ERROR_NONE) {
            break;
        }
    }
    __PCI_DRV_UNLOCK();
}
/*********************************************************************************************************
** ��������: API_PciDrvInit
** ��������: PCI ����ע���ʼ��, ʹ��ע��ӿ�ǰӦ�ȳ�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDrvInit (VOID)
{
    _GuiPciDrvTotalNum  = 0;
    _GuiPciDrvActiveNum = 0;
    _GplinePciDrvHeader = LW_NULL;
    _GulPciDrvLock      = LW_OBJECT_HANDLE_INVALID;

    _GulPciDrvLock = API_SemaphoreMCreate("pci_drv_lock",
                                          LW_PRIO_DEF_CEILING,
                                          (LW_OPTION_WAIT_FIFO |
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_OBJECT_GLOBAL),
                                          LW_NULL);
    if (_GulPciDrvLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("pcidrv", __tshellPciDrvCmd);
    API_TShellFormatAdd("pcidrv", " [add | del] [driver] [[all] | name]");
    API_TShellHelpAdd("pcidrv", "show, add, del pci driver table\n"
                                "eg. pcidrv\n"
                                "    pcidrv add ahci all (Invalid)\n"
                                "    pcidrv add ahci 1:0.1 (Invalid)\n"
                                "    pcidrv del ahci 1:0.1\n");

    return  (ERROR_NONE);
}
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
