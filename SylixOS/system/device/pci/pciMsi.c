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
** ��   ��   ��: pciMsi.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2015 �� 09 �� 10 ��
**
** ��        ��: PCI ���� MSI(Message Signaled Interrupts) ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "linux/bitops.h"
#include "linux/log2.h"
#include "pciMsi.h"
/*********************************************************************************************************
** ��������: API_PciMsixClearSet
** ��������: MSIx ���������������
** �䡡��  : iBus               ���ߺ�
**           iSlot              ���
**           iFunc              ����
**           iMsixCapOft        ƫ�Ƶ�ַ
**           usClear            �����־
**           usSet              ���ñ�־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsixClearSet (INT     iBus,
                          INT     iSlot,
                          INT     iFunc,
                          INT     iMsixCapOft,
                          UINT16  usClear,
                          UINT16  usSet)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    UINT16      usNew     = 0;

    if (iMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsixCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usNew  = usControl & (~usClear);
    usNew |= usSet;
    if (usNew != usControl) {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, iMsixCapOft + PCI_MSIX_FLAGS, usNew);
    } else {
        iRet = ERROR_NONE;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PciMsiMsgWrite
** ��������: д MSI ��Ϣ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           ucMultiple     ����
**           ppmmMsg        ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMsgWrite (INT          iBus,
                         INT          iSlot,
                         INT          iFunc,
                         INT          iMsiCapOft,
                         UINT8        ucMultiple,
                         PCI_MSI_MSG *ppmmMsg)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    usControl &= ~PCI_MSI_FLAGS_QSIZE;
    usControl |= ucMultiple << 4;
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc,
                                 iMsiCapOft + PCI_MSI_ADDRESS_LO, ppmmMsg->uiAddressLo);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iRet = API_PciConfigOutDword(iBus, iSlot, iFunc,
                                     iMsiCapOft + PCI_MSI_ADDRESS_HI, ppmmMsg->uiAddressHi);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc,
                                    iMsiCapOft + PCI_MSI_DATA_64, (UINT16)ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc,
                                    iMsiCapOft + PCI_MSI_DATA_32, (UINT16)ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMsgRead
** ��������: �� MSI ��Ϣ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           ucMultiple     ����
**           ppmmMsg        ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMsgRead (INT          iBus,
                        INT          iSlot,
                        INT          iFunc,
                        INT          iMsiCapOft,
                        UINT8        ucMultiple,
                        PCI_MSI_MSG *ppmmMsg)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInDword(iBus, iSlot, iFunc,
                                iMsiCapOft + PCI_MSI_ADDRESS_LO, &ppmmMsg->uiAddressLo);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iRet = API_PciConfigInDword(iBus, iSlot, iFunc,
                                    iMsiCapOft + PCI_MSI_ADDRESS_HI, &ppmmMsg->uiAddressHi);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc,
                                   iMsiCapOft + PCI_MSI_DATA_64, (UINT16 *)&ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        ppmmMsg->uiAddressHi = 0;
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc,
                                   iMsiCapOft + PCI_MSI_DATA_32, (UINT16 *)&ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiPendingSet
** ��������: MSI Pending ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           uiPending      ָ��λ
**           uiFlag         �±�־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingSet (INT     iBus,
                           INT     iSlot,
                           INT     iFunc,
                           INT     iMsiCapOft,
                           UINT32  uiPending,
                           UINT32  uiFlag)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;
    INT         iPendingPos = 0;
    UINT32      uiPendinged = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iPendingPos = iMsiCapOft + PCI_MSI_PENDING_64;
    } else {
        iPendingPos = iMsiCapOft + PCI_MSI_PENDING_32;
    }
    uiPendinged &= ~uiPending;
    uiPendinged |=  uiFlag;
    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc, iPendingPos, uiPendinged);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiPendingGet
** ��������: ��ȡ MSI Pending ��ȡ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           puiPending     ��ȡ����״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, UINT32 *puiPending)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iEnable     = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;
    INT         iPendingPos = 0;
    UINT32      uiPendinged = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iPendingPos = iMsiCapOft + PCI_MSI_PENDING_64;
    } else {
        iPendingPos = iMsiCapOft + PCI_MSI_PENDING_32;
    }
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iPendingPos, &uiPendinged);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiPending) {
        *puiPending = uiPendinged;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiPendingPosGet
** ��������: ��ȡ MSI Pending λ��
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           piPendingPos   ���λ��
**                          0  ��������Ч
**                          1  ��������Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingPosGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piPendingPos)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iEnable     = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    if (piPendingPos) {
        iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
        if (iIs64) {
            *piPendingPos = iMsiCapOft + PCI_MSI_PENDING_64;
        } else {
            *piPendingPos = iMsiCapOft + PCI_MSI_PENDING_32;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskSet
** ��������: MSI ��������
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           uiMask         ����
**           uiFlag         �±�־
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskSet (INT     iBus,
                        INT     iSlot,
                        INT     iFunc,
                        INT     iMsiCapOft,
                        UINT32  uiMask,
                        UINT32  uiFlag)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;
    INT         iMaskPos  = 0;
    UINT32      uiMasked  = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iMaskPos = iMsiCapOft + PCI_MSI_MASK_BIT_64;
    } else {
        iMaskPos = iMsiCapOft + PCI_MSI_MASK_BIT_32;
    }
    uiMasked &= ~uiMask;
    uiMasked |= uiFlag;
    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc, iMaskPos, uiMasked);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskGet
** ��������: ��ȡ MSI ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           puiMask        ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, UINT32 *puiMask)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;
    INT         iMaskPos  = 0;
    UINT32      uiMasked  = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iMaskPos = iMsiCapOft + PCI_MSI_MASK_BIT_64;
    } else {
        iMaskPos = iMsiCapOft + PCI_MSI_MASK_BIT_32;
    }
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iMaskPos, &uiMasked);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiMask) {
        *puiMask = uiMasked;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskPosGet
** ��������: ��ȡ MSI ����λ��
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           piMaskPos      ���λ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskPosGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piMaskPos)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    if (piMaskPos) {
        iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
        if (iIs64) {
            *piMaskPos = iMsiCapOft + PCI_MSI_MASK_BIT_64;
        } else {
            *piMaskPos = iMsiCapOft + PCI_MSI_MASK_BIT_32;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskConvert
** ��������: MSI ����ת��
** �䡡��  : uiMask     ����
** �䡡��  : ת���������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
UINT32  API_PciMsiMaskConvert (UINT32  uiMask)
{
    if (uiMask >= 5) {
        return  (0xffffffff);
    }

    return  ((1 << (1 << uiMask)) - 1);
}
/*********************************************************************************************************
** ��������: API_PciMsiMultipleGet
** ��������: ��ȡ MSI ��Ϣ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           piMultiCap     ��Ϣ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMultipleGet (INT  iBus,
                            INT  iSlot,
                            INT  iFunc,
                            INT  iMsiCapOft,
                            INT  iNvec,
                            INT *piMultiple)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMultiple) {
        *piMultiple = ilog2(__roundup_pow_of_two(iNvec));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiVecCountGet
** ��������: ��ȡ MSI ������
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           piVecCount     ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiVecCountGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piVecCount)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piVecCount) {
        *piVecCount = 1 << ((usControl & PCI_MSI_FLAGS_QMASK) >> 1);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMultiCapGet
** ��������: ��ȡ MSI ��Ϣ����
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           piMultiCap     ��Ϣ��Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMultiCapGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piMultiCap)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMultiCap) {
        *piMultiCap = (usControl & PCI_MSI_FLAGS_QMASK) >> 1;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsi64BitGet
** ��������: ��ȡ MSI 64 λ��ַ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           pi64Bit        64 λ��ַ��־
**                          0  64 λ��ַ�����Ч
**                          1  64 λ��ַ�����Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsi64BitGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *pi64Bit)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (pi64Bit) {
        *pi64Bit = !!(usControl & PCI_MSI_FLAGS_64BIT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiMaskBitGet
** ��������: ��ȡ MSI ����λ
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           piMaskBit      �����־
**                          0  ��������Ч
**                          1  ��������Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskBitGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piMaskBit)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMaskBit) {
        *piMaskBit = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciMsiEnableSet
** ��������: MSI ʹ�ܿ���
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           iEnable        ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiEnableSet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT  iEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;
    UINT16      usControlNew = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usControlNew = usControl & (~(PCI_MSI_FLAGS_ENABLE));
    if (iEnable) {
        usControlNew |= PCI_MSI_FLAGS_ENABLE;
    }
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, usControlNew);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PciMsiEnableGet
** ��������: ��ȡ MSI ʹ�ܿ���״̬
** �䡡��  : iBus           ���ߺ�
**           iSlot          ���
**           iFunc          ����
**           iMsiCapOft     ƫ�Ƶ�ַ
**           piEnable       ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciMsiEnableGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (piEnable) {
        *piEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: API_PciDevMsiEnableGet
** ��������: ��ȡ MSI ʹ�ܿ���״̬
** �䡡��  : hHandle        �豸���ƾ��
**           piEnable       ʹ������ܱ�־
**                          0  ����
**                          1  ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PciDevMsiEnableGet (PCI_DEV_HANDLE  hHandle, INT *piEnable)
{
    INT     iRet = PX_ERROR;
    UINT8   ucMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PDT_iDevBus,
                          hHandle->PDT_iDevDevice,
                          hHandle->PDT_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &ucMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsiEnableGet(hHandle->PDT_iDevBus,
                               hHandle->PDT_iDevDevice,
                               hHandle->PDT_iDevFunction,
                               ucMsiCapOft,
                               piEnable);
    return  (iRet);
}
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
