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
** ��   ��   ��: ppcCache603.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 03 �� 30 ��
**
** ��        ��: PowerPC 603 ��ϵ���� CACHE ����.
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
extern VOID     ppc603DCacheDisable(VOID);
extern VOID     ppc603DCacheEnable(VOID);
extern VOID     ppc603ICacheDisable(VOID);
extern VOID     ppc603ICacheEnable(VOID);

extern VOID     ppc603DCacheClearAll(VOID);
extern VOID     ppc603DCacheFlushAll(VOID);
extern VOID     ppc603ICacheInvalidateAll(VOID);

extern VOID     ppc603DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc603DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc603DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc603ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc603BranchPredictionDisable(VOID);
extern VOID     ppc603BranchPredictionEnable(VOID);
extern VOID     ppc603BranchPredictorInvalidate(VOID);
/*********************************************************************************************************
  603 CACHE ����
*********************************************************************************************************/
PPC60X_L1C_DRIVER  G_ppc603CacheDriver = {
    ppc603DCacheDisable,
    ppc603DCacheEnable,
    ppc603ICacheDisable,
    ppc603ICacheEnable,

    ppc603DCacheClearAll,
    ppc603DCacheFlushAll,
    ppc603ICacheInvalidateAll,

    ppc603DCacheClear,
    ppc603DCacheFlush,
    ppc603DCacheInvalidate,
    ppc603ICacheInvalidate,

    ppc603BranchPredictionDisable,
    ppc603BranchPredictionEnable,
    ppc603BranchPredictorInvalidate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
