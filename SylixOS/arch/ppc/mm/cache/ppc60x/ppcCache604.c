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
** ��   ��   ��: ppcCache604.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 03 �� 30 ��
**
** ��        ��: PowerPC 604 ��ϵ���� CACHE ����.
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
extern VOID     ppc604DCacheDisable(VOID);
extern VOID     ppc604DCacheEnable(VOID);
extern VOID     ppc604ICacheDisable(VOID);
extern VOID     ppc604ICacheEnable(VOID);

extern VOID     ppc604DCacheClearAll(VOID);
extern VOID     ppc604DCacheFlushAll(VOID);
extern VOID     ppc604ICacheInvalidateAll(VOID);

extern VOID     ppc604DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc604BranchPredictionDisable(VOID);
extern VOID     ppc604BranchPredictionEnable(VOID);
extern VOID     ppc604BranchPredictorInvalidate(VOID);
/*********************************************************************************************************
  604 CACHE ����
*********************************************************************************************************/
PPC60X_L1C_DRIVER  G_ppc604CacheDriver = {
    ppc604DCacheDisable,
    ppc604DCacheEnable,
    ppc604ICacheDisable,
    ppc604ICacheEnable,

    ppc604DCacheClearAll,
    ppc604DCacheFlushAll,
    ppc604ICacheInvalidateAll,

    ppc604DCacheClear,
    ppc604DCacheFlush,
    ppc604DCacheInvalidate,
    ppc604ICacheInvalidate,

    ppc604BranchPredictionDisable,
    ppc604BranchPredictionEnable,
    ppc604BranchPredictorInvalidate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
