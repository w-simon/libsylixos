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
** ��   ��   ��: pinCtrlDev.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 02 ��
**
** ��        ��: pinCtrl �������ӿ�ʵ��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "pinCtrl.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER               _G_plinePinCtrlDevs;           /*  pinctrl ����������          */
/*********************************************************************************************************
  ͬ����
*********************************************************************************************************/
static LW_OBJECT_HANDLE                  _G_hPctlDevLock = LW_OBJECT_HANDLE_INVALID;
static LW_OBJECT_HANDLE                  _G_hPctlPinLock = LW_OBJECT_HANDLE_INVALID;

#define __PIN_LOCK()                     API_SemaphoreMPend(_G_hPctlPinLock, LW_OPTION_WAIT_INFINITE)
#define __PIN_UNLOCK()                   API_SemaphoreMPost(_G_hPctlPinLock)
#define __PIN_LOCK_RETURN_NULL()         do { \
                                             if (_G_hPctlPinLock == LW_OBJECT_HANDLE_INVALID) { \
                                                 return  (LW_NULL);                             \
                                             }                                                  \
                                             __PIN_LOCK();                                      \
                                         } while (0)
#define __PIN_UNLOCK_RETURN_NULL()       __PIN_UNLOCK()
#define __PIN_LOCK_RETURN_VOID()         do { \
                                             if (_G_hPctlPinLock == LW_OBJECT_HANDLE_INVALID) { \
                                                 return;                                        \
                                             }                                                  \
                                             __PIN_LOCK();                                      \
                                         } while (0)
#define __PIN_UNLOCK_RETURN_VOID()       __PIN_UNLOCK()

#define __PCTLDEV_LOCK()                 API_SemaphoreMPend(_G_hPctlDevLock, LW_OPTION_WAIT_INFINITE)
#define __PCTLDEV_UNLOCK()               API_SemaphoreMPost(_G_hPctlDevLock)
#define __PCTLDEV_LOCK_RETURN_NULL()     do { \
                                             if (_G_hPctlPinLock == LW_OBJECT_HANDLE_INVALID) { \
                                                 return  (LW_NULL);                             \
                                             }                                                  \
                                             __PCTLDEV_LOCK();                                  \
                                         } while (0)
#define __PCTLDEV_UNLOCK_RETURN_NULL()   __PCTLDEV_UNLOCK()
/*********************************************************************************************************
** ��������: __pinCtrlPinDescRegister
** ��������: ע��һ�����ŵ�����
** �䡡��  : ppinctldev         ���ſ�����
**           ppctlpindesc       ���ſ�������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pinCtrlPinDescRegister (PLW_PINCTRL_DEV       ppinctldev,
                                      PLW_PINCTRL_PIN_DESC  ppctlpindesc)
{
    PLW_PIN_DESC  pindesc;

    pindesc = API_PinCtrlPinDescGet(ppinctldev,
                                    ppctlpindesc->PCTLPD_uiIndex);      /*  �鿴�����Ƿ��Ѿ���ע��      */
    if (pindesc) {
        PCTL_LOG(PCTL_LOG_BUG, "pin %d already registered\r\n",
                 ppctlpindesc->PCTLPD_uiIndex);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pindesc = __SHEAP_ZALLOC(sizeof(LW_PIN_DESC));
    if (!pindesc) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    pindesc->PIN_ppinctldev = ppinctldev;
    pindesc->PIN_pcName     = ppctlpindesc->PCTLPD_pcName;
    pindesc->PIN_pvData     = ppctlpindesc->PCTLPD_pvData;
    pindesc->PIN_uiPin      = ppctlpindesc->PCTLPD_uiIndex;

    __PIN_LOCK();
    _List_Line_Add_Ahead(&pindesc->PIN_lineManage, &ppinctldev->PCTLD_plineDescs);
    __PIN_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pinCtrlPinDescsAdd
** ��������: ע�����ż��ϵ�����
** �䡡��  : ppinctldev          ���ſ�����
**           ppctlpindesc        ���ſ�����������
**           uiDescsNum          ���ſ�����������Ԫ�ظ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pinCtrlPinDescsAdd (PLW_PINCTRL_DEV       ppinctldev,
                                  PLW_PINCTRL_PIN_DESC  ppctlpindescs,
                                  UINT                  uiDescsNum)
{
    INT     iRet = 0;
    UINT    i;

    for (i = 0; i < uiDescsNum; i++) {
        iRet = __pinCtrlPinDescRegister(ppinctldev, &ppctlpindescs[i]);
        if (iRet) {
            return  (iRet);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pinCtrlPinDescsFree
** ��������: �ͷ����ż��ϵ�����
** �䡡��  : ppinctldev         ���ſ�����
**           ppctlpindescs      ���ſ�����������
**           uiDescsNum         ���ſ�����������Ԫ�ظ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __pinCtrlPinDescsFree (PLW_PINCTRL_DEV       ppinctldev,
                                    PLW_PINCTRL_PIN_DESC  ppctlpindescs,
                                    UINT                  uiDescsNum)
{
    PLW_PIN_DESC     pindesc;
    INT              i;

    for (i = 0; i < uiDescsNum; i++) {
        pindesc = API_PinCtrlPinDescGet(ppinctldev,
                                        ppctlpindescs[i].PCTLPD_uiIndex);
        if (pindesc) {
            API_PinCtrlPinDescDel(ppinctldev, pindesc);
            __SHEAP_FREE(pindesc);
        }
    }
}
/*********************************************************************************************************
** ��������: API_PinCtrlPinDescGet
** ��������: ��ȡһ����������
** �䡡��  : ppinctldev      ���ſ�����
**           uiPin           �������
** �䡡��  : ��õ���������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PLW_PIN_DESC  API_PinCtrlPinDescGet (PLW_PINCTRL_DEV  ppinctldev, UINT  uiPin)
{
    PLW_PIN_DESC     pindesc;
    PLW_LIST_LINE    plineTemp;

    if (!ppinctldev) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    __PIN_LOCK_RETURN_NULL();
    for (plineTemp  = ppinctldev->PCTLD_plineDescs;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pindesc = _LIST_ENTRY(plineTemp, LW_PIN_DESC, PIN_lineManage);
        if (pindesc->PIN_uiPin == uiPin) {
            __PIN_UNLOCK_RETURN_NULL();
            return  (pindesc);
        }
    }
    __PIN_UNLOCK_RETURN_NULL();

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PinCtrlPinDescDel
** ��������: ɾ��һ����������
** �䡡��  : ppinctldev      ���ſ�����
**           pindesc         ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_PinCtrlPinDescDel (PLW_PINCTRL_DEV  ppinctldev, PLW_PIN_DESC  pindesc)
{
    if (!ppinctldev || !pindesc) {
        return;
    }

    __PIN_LOCK_RETURN_VOID();
    _List_Line_Del(&pindesc->PIN_lineManage, &ppinctldev->PCTLD_plineDescs);
    __PIN_UNLOCK_RETURN_VOID();
}
/*********************************************************************************************************
** ��������: API_PinCtrlDevGetByDevtreeNode
** ��������: ���豸���ڵ��ȡ���ſ�����
** �䡡��  : pdtnDev      �豸���ڵ�
** �䡡��  : ���ҵ������ſ�����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_PINCTRL_DEV  API_PinCtrlDevGetByDevtreeNode (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL_DEV  ppinctldev;
    PLW_LIST_LINE    plineTemp;

    __PCTLDEV_LOCK_RETURN_NULL();
    for (plineTemp  = _G_plinePinCtrlDevs;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppinctldev = _LIST_ENTRY(plineTemp, LW_PINCTRL_DEV, PCTLD_lineManage);
        if (ppinctldev->PCTLD_pdtnDev == pdtnDev) {
            __PCTLDEV_UNLOCK_RETURN_NULL();
            return  (ppinctldev);
        }
    }
    __PCTLDEV_UNLOCK_RETURN_NULL();

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_PinCtrlDescBuild
** ��������: �������ſ������������ṹ
** �䡡��  : pcName          ���ſ�����������
**           ppinctlops      ���ſ��Ʋ�����
**           ppinmuxops      ���Ÿ��ò�����
**           ppinconfops     �������ò�����
**           ppctlpindescs   ���ŵ���������
**           uiPinsNum       ���ŵ�����
** �䡡��  : ���������ſ����������ṹ
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_PINCTRL_DESC  API_PinCtrlDescBuild (PCHAR                 pcName,
                                        PLW_PINCTRL_OPS       ppinctlops,
                                        PLW_PINMUX_OPS        ppinmuxops,
                                        PLW_PINCONF_OPS       ppinconfops,
                                        PLW_PINCTRL_PIN_DESC  ppctlpindescs,
                                        INT                   uiPinsNum)
{
    PLW_PINCTRL_DESC  ppinctrldesc;

    if (!ppinctlops || !ppinmuxops || !ppinconfops || !ppctlpindescs) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    ppinctrldesc = (PLW_PINCTRL_DESC)__SHEAP_ALLOC(sizeof(LW_PINCTRL_DESC));
    if (ppinctrldesc == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    ppinctrldesc->PCTLD_ppinctlops      = ppinctlops;
    ppinctrldesc->PCTLD_ppinmuxops      = ppinmuxops;
    ppinctrldesc->PCTLD_ppinconfops     = ppinconfops;
    ppinctrldesc->PCTLD_ppinctrlpindesc = ppctlpindescs;
    ppinctrldesc->PCTLD_uiPinsNum       = uiPinsNum;
    ppinctrldesc->PCTLD_pcName          = pcName;

    return  (ppinctrldesc);
}
/*********************************************************************************************************
** ��������: API_PinCtrlDevCreate
** ��������: ����һ�����ſ�����
** �䡡��  : ppinctldesc     API_PinCtrlDescBuild ���ص���������
**           pvData          ���ſ�����˽�в���
**           pdtnDev         ���������豸���ڵ�
** �䡡��  : ���������ſ�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PLW_PINCTRL_DEV  API_PinCtrlDevCreate (PLW_PINCTRL_DESC  ppinctldesc,
                                       PVOID             pvData,
                                       PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL_DEV  ppinctldev;
    INT              iRet;

    if (!ppinctldesc) {                                                 /*  �������                    */
        PCTL_LOG(PCTL_LOG_ERR, "pinctrl desc not found.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    ppinctldev = (PLW_PINCTRL_DEV)__SHEAP_ZALLOC(sizeof(LW_PINCTRL_DEV));
    if (ppinctldev == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    if (_G_hPctlDevLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hPctlDevLock = API_SemaphoreMCreate("ppinctldev_lock", LW_PRIO_DEF_CEILING,
                                               LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_INHERIT_PRIORITY |
                                               LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    if (_G_hPctlPinLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hPctlPinLock = API_SemaphoreMCreate("pctlpin_lock", LW_PRIO_DEF_CEILING,
                                               LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_INHERIT_PRIORITY |
                                               LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    ppinctldev->PCTLD_ppinctldesc = ppinctldesc;
    ppinctldev->PCTLD_pvData   = pvData;
    ppinctldev->PCTLD_pdtnDev  = pdtnDev;

    iRet = __pinCtrlPinDescsAdd(ppinctldev,
                                ppinctldesc->PCTLD_ppinctrlpindesc,
                                ppinctldesc->PCTLD_uiPinsNum);          /*  ע���������ص�����        */
    if (iRet) {
        __pinCtrlPinDescsFree(ppinctldev,
                              ppinctldesc->PCTLD_ppinctrlpindesc,
                              ppinctldesc->PCTLD_uiPinsNum);
    }

    __PCTLDEV_LOCK();
    _List_Line_Add_Ahead(&ppinctldev->PCTLD_lineManage, &_G_plinePinCtrlDevs);
    __PCTLDEV_UNLOCK();

    return  (ppinctldev);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
