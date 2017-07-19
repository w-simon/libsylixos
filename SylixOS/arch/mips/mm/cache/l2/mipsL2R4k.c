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
** ��   ��   ��: mipsL2R4k.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 07 �� 18 ��
**
** ��        ��: MIPS R4K ��ϵ���� L2-CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_MIPS_CACHE_L2 > 0
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
#include "arch/mips/mm/cache/mipsCacheCommon.h"
#include "arch/mips/mm/cache/l2/mipsL2R4k.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID     mipsL2R4kLineFlush(PCHAR  pcAddr);
extern VOID     mipsL2R4kLineClear(PCHAR  pcAddr);
extern VOID     mipsL2R4kLineInvalidate(PCHAR  pcAddr);
extern VOID     mipsL2R4kIndexClear(PCHAR  pcAddr);
extern VOID     mipsL2R4kIndexStoreTag(PCHAR  pcAddr);
/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
static MIPS_CACHE   _G_SCache;                                          /*  SCACHE ��Ϣ                 */
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
static BOOL         _G_bHaveHitWritebackS = LW_FALSE;                   /*  �Ƿ��� HitWritebackS ����   */
/*********************************************************************************************************
  CACHE ѭ������ʱ���������С, ���ڸô�Сʱ��ʹ�� All ����
*********************************************************************************************************/
#define MIPS_L2_CACHE_LOOP_OP_MAX_SIZE  (64 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** ��������: __mipsL2R4kClear
** ��������: SCACHE ��������Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __mipsL2R4kClear (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsL2R4kLineClear(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: __mipsL2R4kFlush
** ��������: SCACHE �����ݻ�д
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __mipsL2R4kFlush (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsL2R4kLineFlush(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsL2R4kClearAll
** ��������: SCACHE �������ݻ�д����Ч
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kClearAll (VOID)
{
    if (_G_SCache.CACHE_bPresent) {
        REGISTER INT     iWay;
        REGISTER PCHAR   pcLineAddr;
        REGISTER PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
        REGISTER PCHAR   pcEndAddr  = (PCHAR)(A_K0BASE + _G_SCache.CACHE_uiWayStep);

        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay++) {
                mipsL2R4kIndexClear(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }
        MIPS_PIPE_FLUSH();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsL2R4kFlushAll
** ��������: SCACHE �������ݻ�д
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kFlushAll (VOID)
{
    return  (mipsL2R4kClearAll());
}
/*********************************************************************************************************
** ��������: mipsL2R4kClear
** ��������: SCACHE ��������Ч
** �䡡��  : pvStart        ��ʼ�����ַ
**           stBytes        �ֽ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kClear (PVOID  pvAdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= MIPS_L2_CACHE_LOOP_OP_MAX_SIZE) {
        mipsL2R4kClearAll();                                            /*  ȫ����д����Ч              */

    } else {
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_SCache.CACHE_uiLineSize);
        __mipsL2R4kClear(pvAdrs, (PVOID)ulEnd,
                         _G_SCache.CACHE_uiLineSize);                   /*  ���ֻ�д����Ч              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsL2R4kFlush
** ��������: SCACHE �����ݻ�д
** �䡡��  : pvStart        ��ʼ�����ַ
**           stBytes        �ֽ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kFlush (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= MIPS_L2_CACHE_LOOP_OP_MAX_SIZE) {
        mipsL2R4kFlushAll();                                            /*  ȫ����д                    */

    } else {
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_SCache.CACHE_uiLineSize);
        if (_G_bHaveHitWritebackS) {
            __mipsL2R4kFlush(pvAdrs, (PVOID)ulEnd,                      /*  ���ֻ�д                    */
                             _G_SCache.CACHE_uiLineSize);
        } else {
            __mipsL2R4kClear(pvAdrs, (PVOID)ulEnd,
                             _G_SCache.CACHE_uiLineSize);               /*  ���ֻ�д����Ч              */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsL2R4kInvalidate
** ��������: SCACHE ��������Ч
** �䡡��  : pvStart        ��ʼ�����ַ
**           stBytes        �ֽ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kInvalidate (PVOID  pvStart, size_t stBytes)
{
    REGISTER PCHAR   pcAddr;
             addr_t  ulStart = (addr_t)pvStart;
             addr_t  ulEnd   = ulStart + stBytes;

    if (ulStart & ((addr_t)_G_SCache.CACHE_uiLineSize - 1)) {           /*  ��ʼ��ַ�� cache line ����  */
        ulStart &= ~((addr_t)_G_SCache.CACHE_uiLineSize - 1);
        __mipsL2R4kClear((PVOID)ulStart, (PVOID)ulStart, _G_SCache.CACHE_uiLineSize);
        ulStart += _G_SCache.CACHE_uiLineSize;
    }

    if (ulEnd & ((addr_t)_G_SCache.CACHE_uiLineSize - 1)) {             /*  ������ַ�� cache line ����  */
        ulEnd &= ~((addr_t)_G_SCache.CACHE_uiLineSize - 1);
        __mipsL2R4kClear((PVOID)ulEnd, (PVOID)ulEnd, _G_SCache.CACHE_uiLineSize);
    }

    for (pcAddr = (PCHAR)ulStart; pcAddr < (PCHAR)ulEnd; pcAddr += _G_SCache.CACHE_uiLineSize) {
        mipsL2R4kLineInvalidate(pcAddr);                                /*  ����Ч���벿��              */
    }
    MIPS_PIPE_FLUSH();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsL2R4kInitHw
** ��������: SCACHE Ӳ����ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kInitHw (VOID)
{
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    REGISTER PCHAR   pcEndAddr;
    REGISTER INT32   iWay;
    REGISTER CHAR    cTemp;

    (VOID)cTemp;

    /*
     * init L2
     */
    if (_G_SCache.CACHE_bPresent) {
        pcEndAddr = (PCHAR)(A_K0BASE + _G_SCache.CACHE_uiSize);

        /*
         * clear all tags
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay ++) {
                mipsL2R4kIndexStoreTag(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }

        /*
         * load from each line (in cached space)
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            cTemp = *pcLineAddr;
        }

        /*
         * clear all tags again
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay ++) {
                mipsL2R4kIndexStoreTag(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: loongson2SCacheProbe
** ��������: loongson2 SCACHE ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  loongson2SCacheProbe (VOID)
{
    _G_SCache.CACHE_bPresent   = LW_TRUE;
    _G_SCache.CACHE_uiSize     = 512 * 1024;
    _G_SCache.CACHE_uiLineSize = 32;
    _G_SCache.CACHE_uiWayNr    = 4;
    _G_SCache.CACHE_uiWayBit   = 0;
    _G_SCache.CACHE_uiWayStep  = 128 * 1024;
    _G_SCache.CACHE_uiSetNr    =   4 * 1024;
}
/*********************************************************************************************************
** ��������: loongson2HSCacheProbe
** ��������: loongson2H SCACHE ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  loongson2HSCacheProbe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiTemp;

    uiConfig = mipsCp0ConfigRead();                                     /*  �� Config0                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  û�� Config1, �˳�          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config1 Register!\r\n");
        return;
    }

    uiConfig = mipsCp0Config1Read();                                    /*  �� Config1                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  û�� Config2, �˳�          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config2 Register!\r\n");
        return;
    }

    uiConfig = mipsCp0Config2Read();                                    /*  �� Config2                  */
    uiTemp   = (uiConfig & M_Config2SL) >> S_Config2SL;
    if (uiTemp) {
        _G_SCache.CACHE_bPresent   = LW_TRUE;
        _G_SCache.CACHE_uiLineSize = 2 << uiTemp;
        uiTemp                     = (uiConfig & M_Config2SS) >> S_Config2SS;
        _G_SCache.CACHE_uiSetNr    = 64 << uiTemp;
        uiTemp                     = (uiConfig & M_Config2SA) >> S_Config2SA;
        _G_SCache.CACHE_uiWayNr    = 1 + uiTemp;
        _G_SCache.CACHE_uiSize     = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiWayNr *
                                     _G_SCache.CACHE_uiLineSize;
        _G_SCache.CACHE_uiWayStep  = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiLineSize;
        _G_SCache.CACHE_uiWayBit   = 0;
    }
}
/*********************************************************************************************************
** ��������: mipsL2R4kSCacheProbe
** ��������: mips ��׼ SCACHE ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsL2R4kSCacheProbe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiTemp;

    uiConfig = mipsCp0ConfigRead();                                     /*  �� Config0                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  �� Config1, �˳�            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config1 Register!\r\n");
        return;
    }
    uiConfig = mipsCp0Config1Read();                                    /*  �� Config1                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  �� Config2, �˳�            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config2 Register!\r\n");
        return;
    }

    uiConfig = mipsCp0Config2Read();                                    /*  �� Config2                  */
    uiTemp   = (uiConfig & M_Config2SL) >> S_Config2SL;
    if (uiTemp) {
        _G_SCache.CACHE_bPresent   = LW_TRUE;
        _G_SCache.CACHE_uiLineSize = 2 << uiTemp;
        uiTemp                     = (uiConfig & M_Config2SS) >> S_Config2SS;
        _G_SCache.CACHE_uiSetNr    = 64 << uiTemp;
        uiTemp                     = (uiConfig & M_Config2SA) >> S_Config2SA;
        _G_SCache.CACHE_uiWayNr    = 1 + uiTemp;
        _G_SCache.CACHE_uiSize     = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiWayNr *
                                     _G_SCache.CACHE_uiLineSize;
        _G_SCache.CACHE_uiWayStep  = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiLineSize;
        _G_SCache.CACHE_uiWayBit   = lib_ffs(_G_SCache.CACHE_uiWayStep) - 1;
    }
}
/*********************************************************************************************************
** ��������: mipsL2R4kProbe
** ��������: L2 CACHE ̽��
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  mipsL2R4kProbe (VOID)
{
    _G_SCache.CACHE_bPresent = LW_FALSE;

    switch (_G_uiMipsCpuType) {

    case CPU_LOONGSON2:
        loongson2SCacheProbe();
        break;

    case CPU_LOONGSON3:
        loongson2HSCacheProbe();
        break;

    default:
        mipsL2R4kSCacheProbe();
        break;
    }

    if (_G_SCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL,"L2 S-CACHE size %dKB (%d line size, %d way, %d set, %d waybit).\r\n",
                     _G_SCache.CACHE_uiSize / 1024,
                     _G_SCache.CACHE_uiLineSize,
                     _G_SCache.CACHE_uiWayNr,
                     _G_SCache.CACHE_uiSetNr,
                     _G_SCache.CACHE_uiWayBit);
    }

    return  (_G_SCache.CACHE_bPresent ? (ERROR_NONE) : (PX_ERROR));
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
