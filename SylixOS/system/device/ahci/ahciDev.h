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
** 文   件   名: ahciDev.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 29 日
**
** 描        述: AHCI 设备管理.
*********************************************************************************************************/

#ifndef __AHCI_DEV_H
#define __AHCI_DEV_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)

LW_API INT                  API_AhciDevBlkWrite(AHCI_DEV_HANDLE  hDev,
                                                PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount);
LW_API INT                  API_AhciDevBlkRead(AHCI_DEV_HANDLE  hDev,
                                               PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount);

LW_API INT                  API_AhciDevDelete(AHCI_DEV_HANDLE  hDev);
LW_API AHCI_DEV_HANDLE      API_AhciDevAdd(AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive);
LW_API AHCI_DEV_HANDLE      API_AhciDevHandleGet(UINT  uiCtrl, UINT  uiDrive);
LW_API UINT32               API_AhciDevCountGet(VOID);
LW_API INT                  API_AhciDevInit(VOID);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_DEV_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
