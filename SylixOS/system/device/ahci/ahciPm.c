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
** ��   ��   ��: ahciPm.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 03 �� 31 ��
**
** ��        ��: AHCI �豸��Դ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "ahci.h"
#include "ahciLib.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciPm.h"
/*********************************************************************************************************
** ��������: API_AhciApmDisable
** ��������: �����豸�߼���Դ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciApmDisable (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                 iRet;
    AHCI_DRIVE_HANDLE   hDrive;
    AHCI_PARAM_HANDLE   hParam;

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((uiDrive >= hCtrl->AHCICTRL_uiImpPortNum     ) ||
        (hCtrl->AHCICTRL_bDrvInstalled != LW_TRUE    ) ||
        (hCtrl->AHCICTRL_bInstalled    != LW_TRUE    ) ||
        (hDrive->AHCIDRIVE_ucState     != AHCI_DEV_OK)) {
        return  (PX_ERROR);
    }

    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                         AHCI_CMD_SET_FEATURE, AHCI_SUB_DISABLE_APM, 0, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciApmEnable
** ��������: ʹ���豸�߼���Դ����
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
**           iApm       ��Դ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciApmEnable (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, INT  iApm)
{
    INT                 iRet;
    AHCI_DRIVE_HANDLE   hDrive;
    AHCI_PARAM_HANDLE   hParam;

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((uiDrive >= hCtrl->AHCICTRL_uiImpPortNum     ) ||
        (hCtrl->AHCICTRL_bDrvInstalled != LW_TRUE    ) ||
        (hCtrl->AHCICTRL_bInstalled    != LW_TRUE    ) ||
        (hDrive->AHCIDRIVE_ucState     != AHCI_DEV_OK)) {
        return  (PX_ERROR);
    }

    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                         AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_APM, iApm, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_AhciPmActive
** ��������: �豸��Դ�Ƿ�ʹ��
** �䡡��  : hCtrl      ���������
**           uiDrive    ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_AhciPmActive (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
