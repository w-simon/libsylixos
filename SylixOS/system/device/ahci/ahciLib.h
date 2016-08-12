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
** ��   ��   ��: ahciLib.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 01 �� 14 ��
**
** ��        ��: AHCI ������.
*********************************************************************************************************/

#ifndef __AHCI_LIB_H
#define __AHCI_LIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)

#include "ahci.h"
/*********************************************************************************************************
  ����ģʽ
*********************************************************************************************************/
#define AHCI_LOG_RUN                        0x01                        /* ����״̬��Ϣ                 */
#define AHCI_LOG_ERR                        0x02                        /* ������Ϣ                     */
#define AHCI_LOG_BUG                        0x04                        /* BUG ��Ϣ                     */
#define AHCI_LOG_PRT                        0x08                        /* ֱ�Ӵ�ӡ�����Ϣ             */
#define AHCI_LOG_ALL                        0x0F                        /* ��������                     */
#define AHCI_LOG_NONE                       0x00                        /* ������κ���Ϣ               */
/*********************************************************************************************************
  ������Ϣ
*********************************************************************************************************/
#if AHCI_LOG_EN > 0                                                     /* ʹ�ܵ���                     */
#define AHCI_LOG(level, fmt, ...)           do {                                                        \
                                                if ((level) & AHCI_LOG_LEVEL) {                         \
                                                    if ((level) & AHCI_LOG_ERR) {                       \
                                                        _DebugFormat((__PRINTMESSAGE_LEVEL),            \
                                                                     "file %s %s() line %d ",           \
                                                                     __FILE__, __func__, __LINE__);     \
                                                    }                                                   \
                                                    _DebugFormat((__PRINTMESSAGE_LEVEL),                \
                                                                 (fmt), ##__VA_ARGS__);                 \
                                                    _DebugFormat((__PRINTMESSAGE_LEVEL), "%s", "\r\n"); \
                                                }                                                       \
                                            } while (0)
#else                                                                   /* AHCI_LOG_EN                  */
#define AHCI_LOG(level, fmt, ...)
#endif                                                                  /* AHCI_LOG_EN                  */
/*********************************************************************************************************
  �жϵ�����Ϣ
*********************************************************************************************************/
#if (AHCI_LOG_EN > 0) && (AHCI_INT_LOG_EN > 0)                          /* ʹ�ܵ���                     */
#define AHCI_INT_LOG(level, fmt, ...)       do {                                                        \
                                                if ((level) & AHCI_LOG_ERR) {                           \
                                                    _DebugFormat((__PRINTMESSAGE_LEVEL),                \
                                                                 "file %s %s() line %d ",               \
                                                                 __FILE__, __func__, __LINE__);         \
                                                }                                                       \
                                                _DebugFormat((__PRINTMESSAGE_LEVEL),                    \
                                                             (fmt), ##__VA_ARGS__);                     \
                                                _DebugFormat((__PRINTMESSAGE_LEVEL), "%s", "\r\n");     \
                                            } while (0)
#else                                                                   /* AHCI_LOG_EN                  */
#define AHCI_INT_LOG(level, fmt, ...)
#endif                                                                  /* AHCI_LOG_EN                  */
/*********************************************************************************************************
  ���������Ϣ
*********************************************************************************************************/
#if AHCI_LOG_CMD_EN > 0                                                 /* ʹ���������                 */
#define AHCI_CMD_LOG                        AHCI_LOG
#else                                                                   /* AHCI_LOG_CMD_EN              */
#define AHCI_CMD_LOG(level, fmt, ...)
#endif                                                                  /* AHCI_LOG_CMD_EN              */
/*********************************************************************************************************
  �Ĵ�����д
*********************************************************************************************************/
#define AHCI_PCI_READ(dev, pos, len, data)  API_PciDevConfigRead(dev, pos, (UINT8 *)&data, len)
#define AHCI_PCI_WRITE(dev, pos, len, data) do {                                                        \
                                                UINT32  uiData;                                         \
                                                uiData = data;                                          \
                                                API_PciDevConfigWrite(dev, pos, (UINT8 *)&uiData, len); \
                                            } while (0)
/*********************************************************************************************************
  �����Ϣ
*********************************************************************************************************/
#define AHCI_FLAG(x, y)                     ((x & y) ? '+' : '-')
/*********************************************************************************************************
  �Ĵ�����Ϣ
*********************************************************************************************************/
#if AHCI_LOG_EN > 0                                                     /* ʹ�ܵ���                     */
#define AHCI_CTRL_REG_MSG(ctrl, reg)        AHCI_LOG(AHCI_LOG_PRT, "ctrl %d %s: 0x%08x",    \
                                            ctrl->AHCICTRL_uiIndex,                         \
                                            API_AhciCtrlRegNameGet(ctrl, reg), AHCI_CTRL_READ(ctrl, reg))
#define AHCI_PORT_REG_MSG(port, reg)        AHCI_LOG(AHCI_LOG_PRT, "port %d %s: 0x%08x",    \
                                            port->AHCIDRIVE_uiPort,                         \
                                            API_AhciDriveRegNameGet(port, reg), AHCI_PORT_READ(port, reg))
#else                                                                   /* AHCI_LOG_EN                  */
#define AHCI_CTRL_REG_MSG(ctrl, reg)
#define AHCI_PORT_REG_MSG(port, reg)
#endif                                                                  /* AHCI_LOG_EN                  */
/*********************************************************************************************************
  ��ַת��
*********************************************************************************************************/
#define AHCI_ADDR_LOW32(x)                  ((UINT32)(x))
#define AHCI_ADDR_HIGH32(x)                 (0)
/*********************************************************************************************************
  �汾�ַ�����ʽ
*********************************************************************************************************/
#define AHCI_DRV_VER_FORMAT(ver)            "%d.%d.%d-rc%d", (ver >> 24) & 0xff, (ver >> 16) & 0xff,    \
                                             (ver >> 8) & 0xff, ver & 0xff
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
LW_API UINT64               API_AhciDriveSectorCountGet(AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive);
LW_API PCHAR                API_AhciDriveWorkModeNameGet(UINT  uiIndex);
LW_API INT                  API_AhciDriveInfoShow(AHCI_CTRL_HANDLE   hCtrl,
                                                  UINT               uiDrive,
                                                  AHCI_PARAM_HANDLE  hParam);

LW_API INT                  API_AhciDriveRegWait(AHCI_DRIVE_HANDLE  hDrive,
                                                 UINT32             uiRegAddr,
                                                 UINT32             uiMask,
                                                 INT                iFlag,
                                                 UINT32             uiValue,
                                                 ULONG              ulInterTime,
                                                 ULONG              ulTimeout,
                                                 UINT32            *puiReg);
LW_API INT                  API_AhciDriveRecvFisStop(AHCI_DRIVE_HANDLE  hDrive);
LW_API INT                  API_AhciDriveEngineStop(AHCI_DRIVE_HANDLE  hDrive);
LW_API INT                  API_AhciDrivePowerUp(AHCI_DRIVE_HANDLE  hDrive);
LW_API PCHAR                API_AhciDriveRegNameGet(AHCI_DRIVE_HANDLE  hDrive, UINT  uiOffset);

LW_API INT                  API_AhciCtrlIntConnect(AHCI_CTRL_HANDLE  hCtrl,
                                                   PINT_SVR_ROUTINE  pfuncIsr, CPCHAR cpcName);
LW_API INT                  API_AhciCtrlReset(AHCI_CTRL_HANDLE  hCtrl);
LW_API INT                  API_AhciCtrlAhciModeEnable(AHCI_CTRL_HANDLE  hCtrl);
LW_API INT                  API_AhciCtrlSssSet(AHCI_CTRL_HANDLE  hCtrl, INT  iSet);

LW_API INT                  API_AhciCtrlInfoShow(AHCI_CTRL_HANDLE  hCtrl);
LW_API INT                  API_AhciCtrlImpPortGet(AHCI_CTRL_HANDLE  hCtrl);
LW_API INT                  API_AhciCtrlCapGet(AHCI_CTRL_HANDLE  hCtrl);
LW_API PCHAR                API_AhciCtrlRegNameGet(AHCI_CTRL_HANDLE  hCtrl, UINT  uiOffset);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */

#endif                                                                  /*  __AHCI_PORT_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
