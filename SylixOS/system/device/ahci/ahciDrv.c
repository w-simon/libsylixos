/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ahciDrv.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 29 日
**
** 描        述: AHCI 驱动管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "ahci.h"
#include "ahciLib.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciCtrl.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT                     _GuiAhciDrvTotalNum  = 0;
static UINT                     _GuiAhciDrvActiveNum = 0;
static LW_LIST_LINE_HEADER      _GplineAhciDrvHeader = LW_NULL;
static LW_OBJECT_HANDLE         _GulAhciDrvLock      = LW_OBJECT_HANDLE_INVALID;

#define __AHCI_DRV_LOCK()       API_SemaphoreMPend(_GulAhciDrvLock, LW_OPTION_WAIT_INFINITE)
#define __AHCI_DRV_UNLOCK()     API_SemaphoreMPost(_GulAhciDrvLock)
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static VOID     __tshellAhciDrvCmdShow (VOID);
static INT      __tshellAhciDrvCmd(INT  iArgC, PCHAR  ppcArgV[]);
/*********************************************************************************************************
** 函数名称: API_AhciDrvHandleGet
** 功能描述: 获取一个驱动的句柄
** 输　入  : cpcName       驱动名称
** 输　出  : 驱动控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
AHCI_DRV_HANDLE  API_AhciDrvHandleGet (CPCHAR  cpcName)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    AHCI_DRV_HANDLE     hDrv = LW_NULL;

    hDrv = LW_NULL;
    __AHCI_DRV_LOCK();
    for (plineTemp  = _GplineAhciDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrv = _LIST_ENTRY(plineTemp, AHCI_DRV_CB, AHCIDRV_lineDrvNode);
        if (lib_strncmp(&hDrv->AHCIDRV_cDrvName[0], cpcName, AHCI_DRV_NAME_MAX) == 0) {
            break;
        }
    }
    __AHCI_DRV_UNLOCK();

    if (plineTemp) {
        return  (hDrv);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_AhciDrvRegister
** 功能描述: 注册指定类型类型 AHCI 驱动
** 输　入  : hDrvReg       注册控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciDrvRegister (AHCI_DRV_HANDLE  hDrvReg)
{
    INT                 iRet;
    AHCI_DRV_HANDLE     hDrv;

    if ((!hDrvReg) ||
        (_GulAhciDrvLock == LW_OBJECT_HANDLE_INVALID)) {
        return  (PX_ERROR);
    }

    hDrv = API_AhciDrvHandleGet(hDrvReg->AHCIDRV_cDrvName);
    if (hDrv != LW_NULL) {
        return  (PX_ERROR);
    }

    if (!hDrvReg->AHCIDRV_pfuncOptCtrl) {
        return  (PX_ERROR);
    }

    hDrv = (AHCI_DRV_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_DRV_CB));
    if (!hDrv) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    lib_bzero(hDrv, sizeof(AHCI_DRV_CB));
    lib_strlcpy(&hDrv->AHCIDRV_cDrvName[0], &hDrvReg->AHCIDRV_cDrvName[0], AHCI_DRV_NAME_MAX);

    __AHCI_DRV_LOCK();
    _List_Line_Add_Ahead(&hDrv->AHCIDRV_lineDrvNode, &_GplineAhciDrvHeader);
    _GuiAhciDrvTotalNum += 1;
    __AHCI_DRV_UNLOCK();

    hDrv->AHCIDRV_uiDrvVer                   = hDrvReg->AHCIDRV_uiDrvVer;
    hDrv->AHCIDRV_pfuncOptCtrl               = hDrvReg->AHCIDRV_pfuncOptCtrl;
    hDrv->AHCIDRV_pfuncVendorDriveInfoShow   = hDrvReg->AHCIDRV_pfuncVendorDriveInfoShow;
    hDrv->AHCIDRV_pfuncVendorDriveRegNameGet = hDrvReg->AHCIDRV_pfuncVendorDriveRegNameGet;
    hDrv->AHCIDRV_pfuncVendorDriveInit       = hDrvReg->AHCIDRV_pfuncVendorDriveInit;
    hDrv->AHCIDRV_pfuncVendorCtrlInfoShow    = hDrvReg->AHCIDRV_pfuncVendorCtrlInfoShow;
    hDrv->AHCIDRV_pfuncVendorCtrlRegNameGet  = hDrvReg->AHCIDRV_pfuncVendorCtrlRegNameGet;
    hDrv->AHCIDRV_pfuncVendorCtrlTypeNameGet = hDrvReg->AHCIDRV_pfuncVendorCtrlTypeNameGet;
    hDrv->AHCIDRV_pfuncVendorCtrlIntEnable   = hDrvReg->AHCIDRV_pfuncVendorCtrlIntEnable;
    hDrv->AHCIDRV_pfuncVendorCtrlIntConnect  = hDrvReg->AHCIDRV_pfuncVendorCtrlIntConnect;
    hDrv->AHCIDRV_pfuncVendorCtrlInit        = hDrvReg->AHCIDRV_pfuncVendorCtrlInit;
    hDrv->AHCIDRV_pfuncVendorCtrlReadyWork   = hDrvReg->AHCIDRV_pfuncVendorCtrlReadyWork;
    hDrv->AHCIDRV_pfuncVendorPlatformInit    = hDrvReg->AHCIDRV_pfuncVendorPlatformInit;
    hDrv->AHCIDRV_pfuncVendorDrvReadyWork    = hDrvReg->AHCIDRV_pfuncVendorDrvReadyWork;

    if (hDrv->AHCIDRV_pfuncVendorDrvReadyWork) {
        iRet = hDrv->AHCIDRV_pfuncVendorDrvReadyWork(hDrv);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:

    if (hDrv) {
        __AHCI_DRV_LOCK();
        _List_Line_Del(&hDrv->AHCIDRV_lineDrvNode, &_GplineAhciDrvHeader);
        _GuiAhciDrvTotalNum -= 1;
        __AHCI_DRV_UNLOCK();

        __SHEAP_FREE(hDrv);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_AhciDrvInit
** 功能描述: AHCI 驱动注册初始化, 使用注册接口前应先初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciDrvInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiAhciDrvTotalNum  = 0;
    _GuiAhciDrvActiveNum = 0;
    _GplineAhciDrvHeader = LW_NULL;
    _GulAhciDrvLock      = LW_OBJECT_HANDLE_INVALID;

    _GulAhciDrvLock = API_SemaphoreMCreate("ahci_drvlock",
                                           LW_PRIO_DEF_CEILING,
                                           (LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL),
                                           LW_NULL);
    if (_GulAhciDrvLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("ahcidrv", __tshellAhciDrvCmd);
    API_TShellFormatAdd("ahcidrv", " [add | del] [driver] [name]");
    API_TShellHelpAdd("ahcidrv", "show, add, del ahci driver\n"
                                "eg. ahcidrv\n"
                                "    ahcidrv add imx6dq_ahci\n"
                                "    ahcidrv del imx6dq_ahci\n");

    API_AhciDevInit();
    API_AhciCtrlInit();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellAhciDrvCmdShow
** 功能描述: 打印 AHCI 设备驱动列表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tshellAhciDrvCmdShow (VOID)
{
    static PCHAR        pcAhciDrvShowHdr = \
    "INDEX     DRVNAME\n"
    "----- ---------------\n";
    REGISTER INT        i;
    PLW_LIST_LINE       plineTemp = LW_NULL;
    AHCI_DRV_HANDLE     hDrv      = LW_NULL;

    printf("\nahci drv number total : %d\n", _GuiAhciDrvTotalNum);
    printf(pcAhciDrvShowHdr);

    __AHCI_DRV_LOCK();
    i = 0;
    for (plineTemp  = _GplineAhciDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrv = _LIST_ENTRY(plineTemp, AHCI_DRV_CB, AHCIDRV_lineDrvNode);
        printf("%5d %-15s\n", i,hDrv->AHCIDRV_cDrvName);

        i += 1;
    }
    __AHCI_DRV_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __tshellAhciDrvCmd
** 功能描述: AHCI 命令 "ahcidrv"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellAhciDrvCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 1) {
        __tshellAhciDrvCmdShow();
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
