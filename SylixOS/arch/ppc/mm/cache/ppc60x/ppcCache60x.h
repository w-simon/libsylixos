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
** ��   ��   ��: ppcCache60x.h
**
** ��   ��   ��: Yang.HaiFeng (���)
**
** �ļ���������: 2016 �� 01 �� 18 ��
**
** ��        ��: PowerPC 60X ��ϵ���� CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_PPCCACHE60X_H
#define __ARCH_PPCCACHE60X_H

/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/

typedef struct {
    VOID     (*L1CD_pfuncDCacheDisable)(VOID);
    VOID     (*L1CD_pfuncDCacheEnable)(VOID);
    VOID     (*L1CD_pfuncICacheDisable)(VOID);
    VOID     (*L1CD_pfuncICacheEnable)(VOID);

    VOID     (*L1CD_pfuncDCacheClearAll)(VOID);
    VOID     (*L1CD_pfuncDCacheFlushAll)(VOID);
    VOID     (*L1CD_pfuncICacheInvalidateAll)(VOID);

    VOID     (*L1CD_pfuncDCacheClear)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncDCacheFlush)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncDCacheInvalidate)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncICacheInvalidate)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

    VOID     (*L1CD_pfuncBranchPredictionDisable)(VOID);
    VOID     (*L1CD_pfuncBranchPredictionEnable)(VOID);
    VOID     (*L1CD_pfuncBranchPredictorInvalidate)(VOID);
} PPC60X_L1C_DRIVER;

VOID  ppc60xCacheInit(LW_CACHE_OP *pcacheop,
                      CACHE_MODE   uiInstruction,
                      CACHE_MODE   uiData,
                      CPCHAR       pcMachineName);

VOID  ppc60xCacheReset(CPCHAR  pcMachineName);

#endif                                                                  /*  __ARCH_PPCCACHE60X_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
