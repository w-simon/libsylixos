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
** 文   件   名: ahciDev.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 29 日
**
** 描        述: AHCI 设备管理.
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
static UINT32                   _GuiAhciDevTotalNum  = 0;
static LW_OBJECT_HANDLE         _GulAhciDevLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplineAhciDevHeader = LW_NULL;

#define __AHCI_DEV_LOCK()       API_SemaphoreMPend(_GulAhciDevLock, LW_OPTION_WAIT_INFINITE)
#define __AHCI_DEV_UNLOCK()     API_SemaphoreMPost(_GulAhciDevLock)
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static VOID     __tshellAhciDevCmdShow(VOID);
static INT      __tshellAhciDevCmd(INT  iArgC, PCHAR  ppcArgV[]);
/*********************************************************************************************************
** 函数名称: API_AhciDevDelete
** 功能描述: 删除一个 AHCI 设备
** 输　入  : hDev      设备控制句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciDevDelete (AHCI_DEV_HANDLE  hDev)
{
    AHCI_DEV_HANDLE     hDevAhci = LW_NULL;

    hDevAhci = API_AhciDevHandleGet(hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive);
    if ((!hDevAhci) ||
        (hDevAhci != hDev)) {
        return  (PX_ERROR);
    }

    __AHCI_DEV_LOCK();
    _List_Line_Del(&hDev->AHCIDEV_lineDevNode, &_GplineAhciDevHeader);
    _GuiAhciDevTotalNum -= 1;
    __AHCI_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciDevAdd
** 功能描述: 增加一个设备
** 输　入  : hCtrl      控制器句柄
**           iDrive     驱动器编号
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
AHCI_DEV_HANDLE  API_AhciDevAdd (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive)
{
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;
    AHCI_DEV_HANDLE     hDev   = LW_NULL;

    if (iDrive >= hCtrl->AHCICTRL_uiImpPortNum) {
        return  (LW_NULL);
    }

    hDev = API_AhciDevHandleGet(hCtrl->AHCICTRL_uiIndex, iDrive);
    if (hDev != LW_NULL) {
        return  (hDev);
    }

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];
    hDev = hDrive->AHCIDRIVE_hDev;
    __AHCI_DEV_LOCK();
    _List_Line_Add_Ahead(&hDev->AHCIDEV_lineDevNode, &_GplineAhciDevHeader);
    _GuiAhciDevTotalNum += 1;
    __AHCI_DEV_UNLOCK();

    return  (hDev);
}
/*********************************************************************************************************
** 函数名称: API_AhciDevHandleGet
** 功能描述: 获取一个设备的句柄
** 输　入  : iCtrl      控制器索引
**           iDrive     驱动器索引
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
AHCI_DEV_HANDLE  API_AhciDevHandleGet (INT  iCtrl, INT  iDrive)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    AHCI_DEV_HANDLE     hDev      = LW_NULL;

    hDev = LW_NULL;
    __AHCI_DEV_LOCK();
    for (plineTemp  = _GplineAhciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDev = _LIST_ENTRY(plineTemp, AHCI_DEV_CB, AHCIDEV_lineDevNode);
        if ((hDev->AHCIDEV_uiCtrl  == iCtrl) &&
            (hDev->AHCIDEV_uiDrive == iDrive)) {
            break;
        }
    }
    __AHCI_DEV_UNLOCK();

    if (plineTemp) {
        return  (hDev);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_AhciDevCountGet
** 功能描述: 获取 AHCI 设备总数
** 输　入  : NONE
** 输　出  : AHCI 设备总数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
UINT32  API_AhciDevCountGet (VOID)
{
    UINT32      uiCount = 0;

    __AHCI_DEV_LOCK();
    uiCount = _GuiAhciDevTotalNum;
    __AHCI_DEV_UNLOCK();

    return  (uiCount);
}
/*********************************************************************************************************
** 函数名称: API_AhciDevInit
** 功能描述: AHCI 设备管理初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciDevInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiAhciDevTotalNum  = 0;
    _GulAhciDevLock      = LW_OBJECT_HANDLE_INVALID;
    _GplineAhciDevHeader = LW_NULL;

    _GulAhciDevLock = API_SemaphoreMCreate("ahci_devlock",
                                           LW_PRIO_DEF_CEILING,
                                           (LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL),
                                           LW_NULL);
    if (_GulAhciDevLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("ahcidev", __tshellAhciDevCmd);
    API_TShellFormatAdd("ahcidev", " [add | del] [0:1]");
    API_TShellHelpAdd("ahcidev", "show, add, del ahci device\n"
                                "eg. ahcidev\n"
                                "    ahcidev add 0 1\n"
                                "    ahcidev del 0 1\n");
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellAhciDevCmdShow
** 功能描述: 打印 AHCI 设备列表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tshellAhciDevCmdShow (VOID)
{
    static PCHAR        pcAhciDevShowHdr = \
    "INDEX CTRL DRIVER  START       COUNT           DRVNAME         DEVNAME         BLKNAME\n"
    "----- ---- ------ ------- ---------------- --------------- --------------- ---------------\n";

    REGISTER INT        i;
    AHCI_DEV_HANDLE     hDev = LW_NULL;
    PLW_BLK_DEV         hBlkDev = LW_NULL;
    PLW_LIST_LINE       plineTemp = LW_NULL;

    printf("\nahci dev number total : %d\n", _GuiAhciDevTotalNum);
    printf(pcAhciDevShowHdr);

    __AHCI_DEV_LOCK();
    i = 0;
    for (plineTemp  = _GplineAhciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDev = _LIST_ENTRY(plineTemp, AHCI_DEV_CB, AHCIDEV_lineDevNode);
        hBlkDev = &hDev->AHCIDEV_tBlkDev;
        printf("%5d %4d %6d %7d %16d %-15s %-15s %-15s\n",
               i,
               hDev->AHCIDEV_uiCtrl,
               hDev->AHCIDEV_uiDrive,
               (UINT32)hDev->AHCIDEV_ulBlkOffset,
               (UINT32)hDev->AHCIDEV_ulBlkCount,
               hDev->AHCIDEV_hCtrl->AHCICTRL_hDrv->AHCIDRV_cDrvName,
               hDev->AHCIDEV_hDrive->AHCIDRIVE_cDevName,
               hBlkDev->BLKD_pcName);
        i += 1;
    }
    __AHCI_DEV_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __tshellAhciDevCmd
** 功能描述: AHCI 命令 "ahcidev"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellAhciDevCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 1) {
        __tshellAhciDevCmdShow();
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
