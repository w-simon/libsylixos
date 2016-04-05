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
** ��   ��   ��: ppcCache83xx.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 03 �� 30 ��
**
** ��        ��: PowerPC MPC83XX ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "ppcCache60x.h"
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID     ppc83xxDCacheDisable(VOID);
extern VOID     ppc83xxDCacheEnable(VOID);
extern VOID     ppc83xxICacheDisable(VOID);
extern VOID     ppc83xxICacheEnable(VOID);

extern VOID     ppc83xxDCacheClearAll(VOID);
extern VOID     ppc83xxDCacheFlushAll(VOID);
extern VOID     ppc83xxICacheInvalidateAll(VOID);

extern VOID     ppc83xxDCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxDCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxDCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc83xxBranchPredictionDisable(VOID);
extern VOID     ppc83xxBranchPredictionEnable(VOID);
extern VOID     ppc83xxBranchPredictorInvalidate(VOID);
/*********************************************************************************************************
  MPC83XX CACHE ����
*********************************************************************************************************/
PPC60X_L1C_DRIVER  G_ppc83xxCacheDriver = {
    ppc83xxDCacheDisable,
    ppc83xxDCacheEnable,
    ppc83xxICacheDisable,
    ppc83xxICacheEnable,

    ppc83xxDCacheClearAll,
    ppc83xxDCacheFlushAll,
    ppc83xxICacheInvalidateAll,

    ppc83xxDCacheClear,
    ppc83xxDCacheFlush,
    ppc83xxDCacheInvalidate,
    ppc83xxICacheInvalidate,

    ppc83xxBranchPredictionDisable,
    ppc83xxBranchPredictionEnable,
    ppc83xxBranchPredictorInvalidate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
