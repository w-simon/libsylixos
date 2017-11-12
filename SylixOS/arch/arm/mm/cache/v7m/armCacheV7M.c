/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: armCacheV7M.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 11 �� 08 ��
**
** ��        ��: ARMv7 Cortex-Mx ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../../mpu/v7m/armMpuV7M.h"
/*********************************************************************************************************
** ��������: armCacheV7MEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MEnable (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MDisable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MDisable (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MLock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV7MUnlock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV7MTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  armCacheV7MTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheV7MInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  armCacheV7MInit (LW_CACHE_OP *pcacheop,
                               CACHE_MODE   uiInstruction,
                               CACHE_MODE   uiData,
                               CPCHAR       pcMachineName)
{
    pcacheop->CACHEOP_ulOption = 0ul;

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;

    pcacheop->CACHEOP_iICacheLine = 32;
    pcacheop->CACHEOP_iDCacheLine = 32;

    pcacheop->CACHEOP_iICacheWaySize = ((16 * LW_CFG_KB_SIZE) / 64);
    pcacheop->CACHEOP_iDCacheWaySize = ((16 * LW_CFG_KB_SIZE) / 64);

    pcacheop->CACHEOP_pfuncEnable  = armCacheV7MEnable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV7MDisable;

    pcacheop->CACHEOP_pfuncLock    = armCacheV7MLock;
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV7MUnlock;

    pcacheop->CACHEOP_pfuncFlush          = armCacheV7MFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = LW_NULL;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV7MInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = LW_NULL;
    pcacheop->CACHEOP_pfuncClear          = armCacheV7MClear;
    pcacheop->CACHEOP_pfuncClearPage      = LW_NULL;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV7MTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = LW_NULL;

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
    pcacheop->CACHEOP_pfuncDmaMalloc      = armMpuV7MDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = armMpuV7MDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = armMpuV7MDmaFree;
#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
}
/*********************************************************************************************************
** ��������: archCacheV7MReset
** ��������: ��λ CACHE
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  armCacheV7MReset (CPCHAR  pcMachineName)
{
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

