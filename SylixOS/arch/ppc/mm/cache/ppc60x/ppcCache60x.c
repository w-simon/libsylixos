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
** ��   ��   ��: ppcCache60x.c
**
** ��   ��   ��: Yang.HaiFeng (���)
**
** �ļ���������: 2016 �� 01 �� 18 ��
**
** ��        ��: PowerPC 60X ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "ppcCache60x.h"
/*********************************************************************************************************
  L2 CACHE ֧��
*********************************************************************************************************/
#if LW_CFG_PPC_CACHE_L2 > 0
#include "../l2/ppcL2.h"
/*********************************************************************************************************
  L1 CACHE ״̬
*********************************************************************************************************/
static INT              iCacheStatus = 0;
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  CACHE ѭ������ʱ���������С, ���ڸô�Сʱ��ʹ�� All ����
*********************************************************************************************************/
#define PPC_CACHE_LOOP_OP_MAX_SIZE     (8 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/
#define PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)               \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
typedef struct {
    UINT32              CACHE_uiSize;                                   /*  Cache ��С                  */
    UINT32              CACHE_uiLineSize;                               /*  Cache �д�С                */
    UINT32              CACHE_uiSetNr;                                  /*  ����                        */
    UINT32              CACHE_uiWayNr;                                  /*  ·��                        */
    UINT32              CACHE_uiWayStep;                                /*  ·����                      */
} PPC_CACHE;

static PPC_CACHE        _G_ICache, _G_DCache;                           /*  I-Cache �� D-Cache ��Ϣ     */
/*********************************************************************************************************
  Pointer of a page-aligned cacheable region to use as a flush buffer.
*********************************************************************************************************/
UINT8                  *G_pucPpc60xCacheReadBuffer;
/*********************************************************************************************************
  ���� CACHE ����
*********************************************************************************************************/
extern PPC60X_L1C_DRIVER    G_ppc603CacheDriver;
extern PPC60X_L1C_DRIVER    G_ppc604CacheDriver;
extern PPC60X_L1C_DRIVER    G_ppc745xCacheDriver;
extern PPC60X_L1C_DRIVER    G_ppc83xxCacheDriver;
extern PPC60X_L1C_DRIVER    G_ppcEC603CacheDriver;

static PPC60X_L1C_DRIVER   *_G_pCacheDriver = LW_NULL;
/*********************************************************************************************************
  ���� CACHE ������
*********************************************************************************************************/
#define ppc60xDCacheDisable                 _G_pCacheDriver->L1CD_pfuncDCacheDisable
#define ppc60xDCacheEnable                  _G_pCacheDriver->L1CD_pfuncDCacheEnable

#define ppc60xICacheDisable                 _G_pCacheDriver->L1CD_pfuncICacheDisable
#define ppc60xICacheEnable                  _G_pCacheDriver->L1CD_pfuncICacheEnable

#define ppc60xDCacheClearAll                _G_pCacheDriver->L1CD_pfuncDCacheClearAll
#define ppc60xDCacheFlushAll                _G_pCacheDriver->L1CD_pfuncDCacheFlushAll
#define ppc60xICacheInvalidateAll           _G_pCacheDriver->L1CD_pfuncICacheInvalidateAll

#define ppc60xDCacheClear                   _G_pCacheDriver->L1CD_pfuncDCacheClear
#define ppc60xDCacheFlush                   _G_pCacheDriver->L1CD_pfuncDCacheFlush
#define ppc60xDCacheInvalidate              _G_pCacheDriver->L1CD_pfuncDCacheInvalidate
#define ppc60xICacheInvalidate              _G_pCacheDriver->L1CD_pfuncICacheInvalidate

#define ppc60xBranchPredictionDisable       _G_pCacheDriver->L1CD_pfuncBranchPredictionDisable
#define ppc60xBranchPredictionEnable        _G_pCacheDriver->L1CD_pfuncBranchPredictionEnable
#define ppc60xBranchPredictorInvalidate     _G_pCacheDriver->L1CD_pfuncBranchPredictorInvalidate
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: ppc60xCacheEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppc60xCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        ppc60xICacheEnable();
#if LW_CFG_PPC_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        ppc60xBranchPredictionEnable();

    } else {
        ppc60xDCacheEnable();
#if LW_CFG_PPC_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_EN)) {
        ppcL2Enable();
    }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheDisable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppc60xCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        ppc60xICacheDisable();
#if LW_CFG_PPC_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        ppc60xBranchPredictionDisable();

    } else {
        ppc60xDCacheDisable();
#if LW_CFG_PPC_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_DIS)) {
        ppcL2Disable();
    }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����дָ��.
*********************************************************************************************************/
static INT  ppc60xCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheFlushAll();                                     /*  ȫ����д                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheFlush(pvAdrs, (PVOID)ulEnd,                     /*  ���ֻ�д                    */
                              _G_DCache.CACHE_uiLineSize);
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2FlushAll();
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppc60xCacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheFlushAll();                                     /*  ȫ����д                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheFlush(pvAdrs, (PVOID)ulEnd,                     /*  ���ֻ�д                    */
                              _G_DCache.CACHE_uiLineSize);
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ (pvAdrs ������������ַ)
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺���������� DCACHE pvAdrs �����ַ�������ַ������ͬ.
*********************************************************************************************************/
static INT  ppc60xCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xICacheInvalidateAll();                                /*  ICACHE ȫ����Ч             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppc60xICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppc60xDCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }

            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppc60xDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  ����Ч���벿��              */
            ppc60xDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);

#if LW_CFG_PPC_CACHE_L2 > 0
            ppcL2Invalidate(pvAdrs, stBytes);                           /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppc60xCacheInvalidatePage (LW_CACHE_TYPE    cachetype,
                                       PVOID            pvAdrs,
                                       PVOID            pvPdrs,
                                       size_t           stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xICacheInvalidateAll();                                /*  ICACHE ȫ����Ч             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppc60xICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppc60xDCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }

            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppc60xDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  ����Ч���벿��              */
            ppc60xDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);

#if LW_CFG_PPC_CACHE_L2 > 0
            ppcL2Invalidate(pvPdrs, stBytes);                           /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����д����Чָ��.
*********************************************************************************************************/
static INT  ppc60xCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xICacheInvalidateAll();                                /*  ICACHE ȫ����Ч             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppc60xICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheClearAll();                                     /*  ȫ����д����Ч              */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  ���ֻ�д����Ч              */
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2ClearAll();
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppc60xCacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xICacheInvalidateAll();                                /*  ICACHE ȫ����Ч             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppc60xICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheClearAll();                                     /*  ȫ����д����Ч              */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  ���ֻ�д����Ч              */
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheLock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppc60xCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ppc60xCacheUnlock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppc60xCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ppc60xCacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  ppc60xCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
        ppc60xDCacheFlushAll();                                         /*  DCACHE ȫ����д             */
        ppc60xICacheInvalidateAll();                                    /*  ICACHE ȫ����Ч             */

    } else {
        PVOID   pvAdrsBak = pvAdrs;

        PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        ppc60xDCacheFlush(pvAdrs, (PVOID)ulEnd,
                          _G_DCache.CACHE_uiLineSize);                  /*  ���ֻ�д                    */

        PPC_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
        ppc60xICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
INT  ppc60xCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (bInv == LW_FALSE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheFlushAll();                                     /*  ȫ����д                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheFlush(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  ���ֻ�д                    */
        }

    } else {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheClearAll();                                     /*  ȫ����д                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  ���ֻ�д                    */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppc60xCacheProbe
** ��������: CACHE ̽��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppc60xCacheProbe (CPCHAR       pcMachineName)
{
    static BOOL  bProbed = LW_FALSE;

    if (bProbed) {
        return  (ERROR_NONE);
    }

    if (lib_strcmp(pcMachineName, PPC_MACHINE_750) == 0) {
        _G_pCacheDriver = &G_ppc604CacheDriver;

        _G_ICache.CACHE_uiLineSize  = 32;
        _G_ICache.CACHE_uiWayNr     = 8;
        _G_ICache.CACHE_uiSetNr     = 128;
        _G_ICache.CACHE_uiSize      = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiWayNr * \
                                      _G_ICache.CACHE_uiLineSize;
        _G_ICache.CACHE_uiWayStep   = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiLineSize;

        _G_DCache.CACHE_uiLineSize  = 32;
        _G_DCache.CACHE_uiWayNr     = 8;
        _G_DCache.CACHE_uiSetNr     = 128;
        _G_DCache.CACHE_uiSize      = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiWayNr * \
                                      _G_DCache.CACHE_uiLineSize;
        _G_DCache.CACHE_uiWayStep   = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiLineSize;

    } else if (lib_strcmp(pcMachineName, PPC_MACHINE_MPC83XX) == 0) {
        _G_pCacheDriver = &G_ppc83xxCacheDriver;

        _G_ICache.CACHE_uiLineSize  = 32;
        _G_ICache.CACHE_uiWayNr     = 8;
        _G_ICache.CACHE_uiSetNr     = 128;
        _G_ICache.CACHE_uiSize      = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiWayNr * \
                                      _G_ICache.CACHE_uiLineSize;
        _G_ICache.CACHE_uiWayStep   = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiLineSize;

        _G_DCache.CACHE_uiLineSize  = 32;
        _G_DCache.CACHE_uiWayNr     = 8;
        _G_DCache.CACHE_uiSetNr     = 128;
        _G_DCache.CACHE_uiSize      = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiWayNr * \
                                      _G_DCache.CACHE_uiLineSize;
        _G_DCache.CACHE_uiWayStep   = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiLineSize;

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
        return  (PX_ERROR);
    }

    /*
     * Alloc a page-aligned cacheable region to use as a flush buffer.
     * Worst case PLRU flush needs 1.5 * cache size.
     */
    G_pucPpc60xCacheReadBuffer = __KHEAP_ALLOC_ALIGN(_G_DCache.CACHE_uiSize * 3 / 2,
                                                     LW_CFG_VMM_PAGE_SIZE);
    if (!G_pucPpc60xCacheReadBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (PX_ERROR);
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "L1 I-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                 _G_ICache.CACHE_uiSize / 1024,
                 _G_ICache.CACHE_uiLineSize,
                 _G_ICache.CACHE_uiWayNr,
                 _G_ICache.CACHE_uiSetNr);

    _DebugFormat(__LOGMESSAGE_LEVEL, "L1 D-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                 _G_DCache.CACHE_uiSize / 1024,
                 _G_DCache.CACHE_uiLineSize,
                 _G_DCache.CACHE_uiWayNr,
                 _G_DCache.CACHE_uiSetNr);

    bProbed = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppc60xCacheInit (LW_CACHE_OP *pcacheop,
                       CACHE_MODE   uiInstruction,
                       CACHE_MODE   uiData,
                       CPCHAR       pcMachineName)
{
    INT     iError;

    iError = ppc60xCacheProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        return;
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    ppcL2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWayStep;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWayStep;

    _DebugFormat(__LOGMESSAGE_LEVEL, "PowerPC I-Cache line size = %d bytes Way size = %d bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "PowerPC D-Cache line size = %d bytes Way size = %d bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    pcacheop->CACHEOP_pfuncEnable  = ppc60xCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = ppc60xCacheDisable;

    pcacheop->CACHEOP_pfuncLock   = ppc60xCacheLock;                    /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock = ppc60xCacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = ppc60xCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = ppc60xCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = ppc60xCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = ppc60xCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = ppc60xCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = ppc60xCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = ppc60xCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = ppc60xCacheDataUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: archCacheReset
** ��������: ��λ CACHE
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ����� lockdown �������� unlock & invalidate ��������
*********************************************************************************************************/
VOID  ppc60xCacheReset (CPCHAR  pcMachineName)
{
    INT     iError;

    iError = ppc60xCacheProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        return;
    }

    ppc60xICacheInvalidateAll();
    ppc60xDCacheDisable();
    ppc60xICacheDisable();
    ppc60xBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
