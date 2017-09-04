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
** ��   ��   ��: ppcL2Cache750.c
**
** ��   ��   ��: Yang.HaiFeng (���)
**
** �ļ���������: 2016 �� 03 �� 08 ��
**
** ��        ��: MPC750 ��ϵ���� L2 CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0
#include "../ppcL2.h"
#include "ppcL2Cache750.h"
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID  ppc750L2CacheInit(UINT32  uiL2CR);
extern VOID  ppc750L2CacheEnable(VOID);
extern VOID  ppc750L2CacheDisable(VOID);
extern BOOL  ppc750L2CacheIsEnable(VOID);
extern VOID  ppc750L2CacheInvalidateAll(VOID);
extern VOID  ppc750L2CacheFlushAllSW(size_t  stSize, UINT8  *pucReadBuffer);
extern VOID  ppc750L2CacheFlushAllHW(VOID);

extern VOID  ppc745xL2CacheInit(UINT32  uiL2CR);
extern VOID  ppc745xL2CacheEnable(VOID);
extern VOID  ppc745xL2CacheDisable(VOID);
extern BOOL  ppc745xL2CacheIsEnable(VOID);
extern VOID  ppc745xL2CacheInvalidateAll(VOID);
extern VOID  ppc745xL2CacheFlushAllSW(size_t  stSize, UINT8  *pucReadBuffer);
extern VOID  ppc745xL2CacheFlushAllHW(VOID);
/*********************************************************************************************************
  Pointer of a page-aligned cacheable region to use as a flush buffer.
*********************************************************************************************************/
static UINT8                   *_G_pucL2CacheReadBuffer;
/*********************************************************************************************************
  L2 CACHE ����(Ĭ�ϲ����� L2 CACHE������ֵΪ����ʾ��)
*********************************************************************************************************/
static PPC750_L2CACHE_CONFIG    _G_l2Config = {
        .CFG_bPresent   = LW_FALSE,
        .CFG_stSize     = L2RAM_SIZE_1M,
        .CFG_uiL2CR     = L2CR_SIZE_1MB |
                          L2CR_RAM_PB2  |
                          L2CR_OH_0_5ns,
};
/*********************************************************************************************************
** ��������: ppcL2Cache750Config
** ��������: L2 CACHE ����
** �䡡��  : l2Config          L2 CACHE ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcL2Cache750Config (PPC750_L2CACHE_CONFIG  *pL2Config)
{
    if (pL2Config) {
        _G_l2Config = *pL2Config;
    }
}
/*********************************************************************************************************
** ��������: __ppc750L2CacheFlushAllSW
** ��������: L2 CACHE �����д����Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ppc750L2CacheFlushAllSW (VOID)
{
    ppc750L2CacheFlushAllSW((_G_l2Config.CFG_stSize * 2) / L1CACHE_ALIGN_SIZE,
                            _G_pucL2CacheReadBuffer);
}
/*********************************************************************************************************
** ��������: ppcL2Cache750Init
** ��������: ��ʼ�� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
**           uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcL2Cache750Init (L2C_DRVIER  *pl2cdrv,
                         CACHE_MODE   uiInstruction,
                         CACHE_MODE   uiData,
                         CPCHAR       pcMachineName)
{
    if (_G_l2Config.CFG_bPresent) {
        /*
         * Alloc a page-aligned cacheable region to use as a flush buffer.
         * Worst case PLRU flush needs 1.5 * cache size.
         */
        _G_pucL2CacheReadBuffer = __KHEAP_ALLOC_ALIGN(ROUND_UP((_G_l2Config.CFG_stSize * 3) >> 1,
                                                      LW_CFG_VMM_PAGE_SIZE), LW_CFG_VMM_PAGE_SIZE);
        if (!_G_pucL2CacheReadBuffer) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            return;
        }

        /*
         * See <<PowerPC 750 RISC Microprocessor User's Manual>>
         * The icbi instruction invalidates only L1 cache blocks and is never forwarded to the L2 cache.
         * ���� L2-Cache ֻ���� D-Cache only���������� DO λ
         */
        if (lib_strcmp(pcMachineName, PPC_MACHINE_750) == 0) {
            _G_l2Config.CFG_uiL2CR  |= L2CR_DO;
            _G_l2Config.CFG_uiL2CR <<= 16;

            pl2cdrv->L2CD_stSize             = _G_l2Config.CFG_stSize;
            pl2cdrv->L2CD_pfuncEnable        = ppc750L2CacheEnable;
            pl2cdrv->L2CD_pfuncDisable       = ppc750L2CacheDisable;
            pl2cdrv->L2CD_pfuncIsEnable      = ppc750L2CacheIsEnable;
            pl2cdrv->L2CD_pfuncSync          = LW_NULL;
            pl2cdrv->L2CD_pfuncFlush         = LW_NULL;
            pl2cdrv->L2CD_pfuncFlushAll      = __ppc750L2CacheFlushAllSW;
            pl2cdrv->L2CD_pfuncInvalidate    = LW_NULL;
            pl2cdrv->L2CD_pfuncInvalidateAll = ppc750L2CacheInvalidateAll;
            pl2cdrv->L2CD_pfuncClear         = LW_NULL;
            pl2cdrv->L2CD_pfuncClearAll      = pl2cdrv->L2CD_pfuncFlushAll;

            ppc750L2CacheInit(_G_l2Config.CFG_uiL2CR);

        } else {
            _G_l2Config.CFG_uiL2CR  |= PPC7450_L2CR_DO;
            _G_l2Config.CFG_uiL2CR <<= 16;

            pl2cdrv->L2CD_stSize             = _G_l2Config.CFG_stSize;
            pl2cdrv->L2CD_pfuncEnable        = ppc745xL2CacheEnable;
            pl2cdrv->L2CD_pfuncDisable       = ppc745xL2CacheDisable;
            pl2cdrv->L2CD_pfuncIsEnable      = ppc745xL2CacheIsEnable;
            pl2cdrv->L2CD_pfuncSync          = LW_NULL;
            pl2cdrv->L2CD_pfuncFlush         = LW_NULL;
            pl2cdrv->L2CD_pfuncFlushAll      = ppc745xL2CacheFlushAllHW;
            pl2cdrv->L2CD_pfuncInvalidate    = LW_NULL;
            pl2cdrv->L2CD_pfuncInvalidateAll = ppc745xL2CacheInvalidateAll;
            pl2cdrv->L2CD_pfuncClear         = LW_NULL;
            pl2cdrv->L2CD_pfuncClearAll      = pl2cdrv->L2CD_pfuncFlushAll;

            ppc745xL2CacheInit(_G_l2Config.CFG_uiL2CR);
        }
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
