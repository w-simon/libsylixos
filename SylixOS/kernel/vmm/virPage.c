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
** ��   ��   ��: virPage.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: �����ڴ����.

** BUG:
2013.05.24  ��������ռ���뿪��.
2014.08.08  ���� __vmmVirtualDesc() ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
  ��ַ�ռ��ͻ���
*********************************************************************************************************/
extern BOOL     __vmmLibVirtualOverlap(addr_t  ulAddr, size_t  stSize);
/*********************************************************************************************************
  ����ռ�����
*********************************************************************************************************/
static LW_MMU_VIRTUAL_DESC  _G_vmvirDescApp[LW_CFG_VMM_VIR_NUM];        /*  Ӧ�ó���                    */
static LW_MMU_VIRTUAL_DESC  _G_vmvirDescDev;                            /*  �����豸                    */
/*********************************************************************************************************
  �豸����ռ� zone ���ƿ�
*********************************************************************************************************/
static LW_VMM_ZONE          _G_vmzoneVirApp[LW_CFG_VMM_VIR_NUM];
static LW_VMM_ZONE          _G_vmzoneVirDev;
/*********************************************************************************************************
  �л�ͨ��
*********************************************************************************************************/
static addr_t               _G_ulVmmSwitchAddr = (addr_t)PX_ERROR;
/*********************************************************************************************************
** ��������: __vmmVirtualDesc
** ��������: �������ռ�����.
** �䡡��  : uiType        ����
**           ulZoneIndex   ���������±�
** �䡡��  : ����ռ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_MMU_VIRTUAL_DESC  __vmmVirtualDesc (UINT32  uiType, ULONG  ulZoneIndex)
{
    if (ulZoneIndex >= LW_CFG_VMM_VIR_NUM) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (uiType == LW_VIRTUAL_MEM_APP) {
        return  (&_G_vmvirDescApp[ulZoneIndex]);
    
    } else {
        return  (&_G_vmvirDescDev);
    }
}
/*********************************************************************************************************
** ��������: __vmmVirtualSwitch
** ��������: �������ռ����򽻻���.
** �䡡��  : NONE
** �䡡��  : ����ռ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
addr_t  __vmmVirtualSwitch (VOID)
{
    return  (_G_ulVmmSwitchAddr);
}
/*********************************************************************************************************
** ��������: __vmmVirtualGetZone
** ��������: ȷ������ zone �±�.
** �䡡��  : ulAddr        ��ַ
** �䡡��  : zone �±�.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __vmmVirtualGetZone (addr_t  ulAddr)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;

    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if ((ulAddr >= pvmzone->ZONE_ulAddr) &&
            (ulAddr <  pvmzone->ZONE_ulAddr + pvmzone->ZONE_stSize)) {
            return  ((ULONG)i);
        }
    }
    
    return  (LW_CFG_VMM_VIR_NUM);
}
/*********************************************************************************************************
** ��������: __vmmVirtualIsInside
** ��������: �жϵ�ַ�Ƿ�������ռ���.
** �䡡��  : ulAddr        ��ַ
** �䡡��  : �Ƿ��� VMM ����ռ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  __vmmVirtualIsInside (addr_t  ulAddr)
{
    ULONG   ulZoneIndex = __vmmVirtualGetZone(ulAddr);
    
    if (ulZoneIndex >= LW_CFG_VMM_VIR_NUM) {
        return  (LW_FALSE);
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __vmmVirtualCreate
** ��������: ��������ռ�����.
** �䡡��  : pvirdes       ����ռ�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmVirtualCreate (LW_MMU_VIRTUAL_DESC   pvirdes[])
{
    REGISTER ULONG  ulError = ERROR_NONE;
             ULONG  ulZone  = 0;
             INT    i;
    
    for (i = 0; ; i++) {
        if (pvirdes[i].VIRD_stSize == 0) {
            break;
        }
        
        _BugFormat(__vmmLibVirtualOverlap(pvirdes[i].VIRD_ulVirAddr, 
                                          pvirdes[i].VIRD_stSize), LW_TRUE,
                   "virtual zone vaddr 0x%08lx size : 0x%08zx overlap with virtual space.\r\n",
                   pvirdes[i].VIRD_ulVirAddr, pvirdes[i].VIRD_stSize);
        
        if (pvirdes[i].VIRD_uiType == LW_VIRTUAL_MEM_APP) {
            if (ulZone >= LW_CFG_VMM_ZONE_NUM) {
                continue;
            }
            
            _G_vmvirDescApp[ulZone] = pvirdes[i];
            
            if (_G_ulVmmSwitchAddr == (addr_t)PX_ERROR) {
                _G_ulVmmSwitchAddr =  pvirdes[i].VIRD_ulVirAddr;
                
                ulError = __pageZoneCreate(&_G_vmzoneVirApp[ulZone], 
                                           _G_ulVmmSwitchAddr + LW_CFG_VMM_PAGE_SIZE, 
                                           pvirdes[i].VIRD_stSize, 
                                           LW_ZONE_ATTR_NONE,
                                           __VMM_PAGE_TYPE_VIRTUAL);
            } else {
                ulError = __pageZoneCreate(&_G_vmzoneVirApp[ulZone], 
                                           pvirdes[i].VIRD_ulVirAddr, 
                                           pvirdes[i].VIRD_stSize, 
                                           LW_ZONE_ATTR_NONE,
                                           __VMM_PAGE_TYPE_VIRTUAL);
            }
            ulZone++;
            
        } else {
            _G_vmvirDescDev = pvirdes[i];
            
            ulError = __pageZoneCreate(&_G_vmzoneVirDev, 
                                       pvirdes[i].VIRD_ulVirAddr, 
                                       pvirdes[i].VIRD_stSize, 
                                       LW_ZONE_ATTR_NONE,
                                       __VMM_PAGE_TYPE_VIRTUAL);
        }
        
        if (ulError) {
            _ErrorHandle(ulError);
            return  (ulError);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmVirtualPageAlloc
** ��������: ����ָ������������ҳ��
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirtualPageAlloc (ULONG  ulPageNum)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
            pvmpage = __pageAllocate(pvmzone, ulPageNum, __VMM_PAGE_TYPE_VIRTUAL);
            if (pvmpage) {
                return  (pvmpage);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmVirDevPageAlloc
** ��������: ����ָ�������������豸ҳ��
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirDevPageAlloc (ULONG  ulPageNum)
{
    return  (__pageAllocate(&_G_vmzoneVirDev, ulPageNum, __VMM_PAGE_TYPE_VIRTUAL));
}
/*********************************************************************************************************
** ��������: __vmmVirtualPageAllocAlign
** ��������: ����ָ������������ҳ�� (ָ�������ϵ)
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
**           stAlign       �ڴ�����ϵ
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirtualPageAllocAlign (ULONG  ulPageNum, size_t  stAlign)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
            pvmpage = __pageAllocateAlign(pvmzone, ulPageNum, stAlign, __VMM_PAGE_TYPE_VIRTUAL);
            if (pvmpage) {
                return  (pvmpage);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmVirDevPageAllocAlign
** ��������: ����ָ�������������豸ҳ�� (ָ�������ϵ)
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
**           stAlign       �ڴ�����ϵ
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirDevPageAllocAlign (ULONG  ulPageNum, size_t  stAlign)
{
    return  (__pageAllocateAlign(&_G_vmzoneVirDev, ulPageNum, 
                                 stAlign, __VMM_PAGE_TYPE_VIRTUAL));
}
/*********************************************************************************************************
** ��������: __vmmVirtualPageFree
** ��������: ����ָ����������ҳ��
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmVirtualPageFree (PLW_VMM_PAGE  pvmpage)
{
    ULONG   ulZoneIndex = __vmmVirtualGetZone(pvmpage->PAGE_ulPageAddr);

    __pageFree(&_G_vmzoneVirApp[ulZoneIndex], pvmpage);
}
/*********************************************************************************************************
** ��������: __vmmVirDevPageFree
** ��������: ����ָ�����������豸ҳ��
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmVirDevPageFree (PLW_VMM_PAGE  pvmpage)
{
    __pageFree(&_G_vmzoneVirDev, pvmpage);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
