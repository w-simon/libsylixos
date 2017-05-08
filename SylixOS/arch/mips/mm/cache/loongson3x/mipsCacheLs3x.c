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
** ��   ��   ��: mipsCacheLs3x.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 11 �� 02 ��
**
** ��        ��: Loongson3x ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  ls3xCacheEnableHw(VOID);
extern VOID  ls3xCacheDisableHw(VOID);
/*********************************************************************************************************
  �ڲ�����ǰ������
*********************************************************************************************************/
static INT   ls3xCacheProbe(VOID);
/*********************************************************************************************************
  L1 CACHE ״̬
*********************************************************************************************************/
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00

static INT      _G_iLs3xCacheStatus = L1_CACHE_DIS;
/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
typedef struct {
    BOOL        CACHE_bPresent;                                         /*  �Ƿ���� Cache              */
    UINT32      CACHE_uiSize;                                           /*  Cache ��С                  */
    UINT32      CACHE_uiLineSize;                                       /*  Cache �д�С                */
    UINT32      CACHE_uiSetNr;                                          /*  ����                        */
    UINT32      CACHE_uiWayNr;                                          /*  ·��                        */
    UINT32      CACHE_uiWayStep;                                        /*  ·����                      */
} MIPS_CACHE;

static MIPS_CACHE   _G_ICache, _G_DCache;                               /*  I-Cache �� D-Cache ��Ϣ     */
static MIPS_CACHE   _G_VCache, _G_SCache;                               /*  V-Cache �� S-Cache ��Ϣ     */
/*********************************************************************************************************
** ��������: ls3xBranchPredictionDisable
** ��������: ���ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3xBranchPredictionDisable (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag |= 1 << 0;                                                   /*  �� 1 ʱ���÷�֧Ԥ��         */
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** ��������: ls3xBranchPredictionEnable
** ��������: ʹ�ܷ�֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3xBranchPredictionEnable (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag &= ~(1 << 0);
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** ��������: ls3xBranchPredictorInvalidate
** ��������: ��Ч��֧Ԥ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3xBranchPredictorInvalidate (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag |= 1 << 1;                                                   /*  д�� 1 ��� BRBTB �� BTAC   */
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** ��������: ls3aR1CacheFlushAll
** ��������: Loongson3A R1 ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3aR1CacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  flush L1 ICache             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  flush L1 DCache             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x20  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** ��������: ls3aR2CacheFlushAll
** ��������: Loongson3A R2 ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3aR2CacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  flush L1 ICache             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  flush L1 DCache             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x40  \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "2: cache 2, 0(%[addr])           \n"                           /*  flush L1 VCache             */
        "   cache 2, 1(%[addr])           \n"
        "   cache 2, 2(%[addr])           \n"
        "   cache 2, 3(%[addr])           \n"
        "   cache 2, 4(%[addr])           \n"
        "   cache 2, 5(%[addr])           \n"
        "   cache 2, 6(%[addr])           \n"
        "   cache 2, 7(%[addr])           \n"
        "   cache 2, 8(%[addr])           \n"
        "   cache 2, 9(%[addr])           \n"
        "   cache 2, 10(%[addr])          \n"
        "   cache 2, 11(%[addr])          \n"
        "   cache 2, 12(%[addr])          \n"
        "   cache 2, 13(%[addr])          \n"
        "   cache 2, 14(%[addr])          \n"
        "   cache 2, 15(%[addr])          \n"
        "   addiu %[vsets], %[vsets], -1  \n"
        "   bnez  %[vsets], 2b            \n"
        "   addiu %[addr], %[addr], 0x40  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr),
          [vsets] "r" (_G_VCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** ��������: ls3bCacheFlushAll
** ��������: Loongson3B ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3bCacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  flush L1 ICache             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  flush L1 DCache             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x20  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** ��������: ls3xCacheFlushAll
** ��������: ��д���� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ls3xCacheFlushAll (VOID)
{
    INT     iError;

    iError = ls3xCacheProbe();
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No Cache!\r\n");
        return;
    }

    switch (mipsCp0PRIdRead() & 0xf) {

    case PRID_REV_LOONGSON3A_R2:
    case PRID_REV_LOONGSON3A_R3:
        ls3aR2CacheFlushAll();
        break;

    case PRID_REV_LOONGSON3B_R1:
    case PRID_REV_LOONGSON3B_R2:
        ls3bCacheFlushAll();
        break;

    case PRID_REV_LOONGSON2K:
        ls3aR1CacheFlushAll();
        break;

    case PRID_REV_LOONGSON3A_R1:
    default:
        ls3aR1CacheFlushAll();
        break;
    }
}
/*********************************************************************************************************
** ��������: ls3xCacheEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iLs3xCacheStatus |= L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iLs3xCacheStatus |= L1_CACHE_D_EN;
        }
    }

    if (_G_iLs3xCacheStatus == L1_CACHE_EN) {
        ls3xCacheEnableHw();
        ls3xBranchPredictionEnable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheDisable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iLs3xCacheStatus &= ~L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iLs3xCacheStatus &= ~L1_CACHE_D_EN;
        }
    }

    if (_G_iLs3xCacheStatus == L1_CACHE_DIS) {
        ls3xCacheDisableHw();
        ls3xBranchPredictionDisable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheFlushNone
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheFlushNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheFlushPageNone
** ��������: CACHE �����ݻ�д (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheFlushPageNone (LW_CACHE_TYPE  cachetype,
                                    PVOID          pvAdrs,
                                    PVOID          pvPdrs,
                                    size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheInvalidateNone
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheInvalidateNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheInvalidatePageNone
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheInvalidatePageNone (LW_CACHE_TYPE  cachetype,
                                         PVOID          pvAdrs,
                                         PVOID          pvPdrs,
                                         size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheClearNone
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����) (α����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheClearNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheClearPageNone
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheClearPageNone (LW_CACHE_TYPE  cachetype,
                                    PVOID          pvAdrs,
                                    PVOID          pvPdrs,
                                    size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheLockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ls3xCacheUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  ls3xCacheUnlockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ls3xCacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  ls3xCacheTextUpdateNone (PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  ls3xCacheDataUpdateNone (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xL3CacheProbe
** ��������: L3 S-CACHE ̽��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xL3SCacheProbe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiLineSize;
    UINT32  uiPrid;

    uiConfig = mipsCp0Config2Read();                                    /*  �� Config2                  */
    if ((uiLineSize = ((uiConfig >> 4) & 15))) {
        _G_SCache.CACHE_bPresent   = LW_TRUE;
        _G_SCache.CACHE_uiLineSize = 2 << uiLineSize;
        _G_SCache.CACHE_uiSetNr    = 64 << ((uiConfig >> 8) & 15);
        _G_SCache.CACHE_uiWayNr    = 1 + (uiConfig & 15);
        _G_SCache.CACHE_uiSize     = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiWayNr *
                                     _G_SCache.CACHE_uiLineSize;
        /*
         * Loongson-3 has 4 cores, 1MB scache for each. scaches are shared
         */
        uiPrid = mipsCp0PRIdRead() & 0xf;
        if (uiPrid != PRID_REV_LOONGSON2K) {                            /*  Loongson2K1000 Ϊ 1M        */
            _G_SCache.CACHE_uiSize *= 4;
        }

        _G_SCache.CACHE_uiWayStep  = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiLineSize;

    } else {
        _G_SCache.CACHE_bPresent = LW_FALSE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xL2CacheProbe
** ��������: L2 V-CACHE ̽��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xL2VCacheProbe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiLineSize;
    UINT32  uiPrid;

    uiPrid = mipsCp0PRIdRead() & 0xf;
    if (uiPrid == PRID_REV_LOONGSON2K) {                                /*  Loongson2K1000 �� V CACHE   */
        _G_VCache.CACHE_bPresent = LW_FALSE;
        return  (ERROR_NONE);
    }

    uiConfig = mipsCp0Config2Read();                                    /*  �� Config2                  */
    if ((uiLineSize = ((uiConfig >> 20) & 15))) {
        _G_VCache.CACHE_bPresent   = LW_TRUE;
        _G_VCache.CACHE_uiLineSize = 2 << uiLineSize;
        _G_VCache.CACHE_uiSetNr    = 64 << ((uiConfig >> 24) & 15);
        _G_VCache.CACHE_uiWayNr    = 1 + ((uiConfig >> 16) & 15);
        _G_VCache.CACHE_uiSize     = _G_VCache.CACHE_uiSetNr * _G_VCache.CACHE_uiWayNr *
                                     _G_VCache.CACHE_uiLineSize;
        _G_VCache.CACHE_uiWayStep  = _G_VCache.CACHE_uiSetNr * _G_VCache.CACHE_uiLineSize;

    } else {
        _G_VCache.CACHE_bPresent = LW_FALSE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheProbe
** ��������: CACHE ̽��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls3xCacheProbe (VOID)
{
    static BOOL    bIsProbed = LW_FALSE;
           UINT32  uiConfig;
           UINT32  uiTemp;

    if (bIsProbed) {
        return  (ERROR_NONE);
    }

    uiConfig = mipsCp0ConfigRead();                                     /*  �� Config0                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  û�� Config1, �˳�          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config1 Register!\r\n");
        return  (PX_ERROR);
    }

    uiConfig = mipsCp0Config1Read();                                    /*  �� Config1                  */
    uiTemp   = (uiConfig & M_Config1IL) >> S_Config1IL;
    if (uiTemp) {
        _G_ICache.CACHE_bPresent   = LW_TRUE;

        _G_ICache.CACHE_uiLineSize = 2 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1IS) >> S_Config1IS;
        _G_ICache.CACHE_uiSetNr    = 64 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1IA) >> S_Config1IA;
        _G_ICache.CACHE_uiWayNr    = 1 + uiTemp;

        _G_ICache.CACHE_uiSize     = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiWayNr *
                                     _G_ICache.CACHE_uiLineSize;

        _G_ICache.CACHE_uiWayStep  = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiLineSize;

    } else {                                                            /*  û�� I-Cache                */
        _G_ICache.CACHE_bPresent   = LW_FALSE;
    }

    uiTemp = (uiConfig & M_Config1DL) >> S_Config1DL;
    if (uiTemp) {
        _G_DCache.CACHE_bPresent   = LW_TRUE;

        _G_DCache.CACHE_uiLineSize = 2 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1DS) >> S_Config1DS;
        _G_DCache.CACHE_uiSetNr    = 64 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1DA) >> S_Config1DA;
        _G_DCache.CACHE_uiWayNr    = 1 + uiTemp;

        _G_DCache.CACHE_uiSize     = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiWayNr *
                                     _G_DCache.CACHE_uiLineSize;

        _G_DCache.CACHE_uiWayStep  = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiLineSize;

    } else {                                                            /*  û�� D-Cache                */
        _G_DCache.CACHE_bPresent   = LW_FALSE;
    }

    ls3xL2VCacheProbe();
    ls3xL3SCacheProbe();

    bIsProbed = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls3xCacheInfoShow
** ��������: CACHE ��Ϣ��ӡ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls3xCacheInfoShow (VOID)
{
    INT     iLevel = 2;

    if (_G_ICache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L1 I-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_ICache.CACHE_uiSize / 1024,
                     _G_ICache.CACHE_uiLineSize,
                     _G_ICache.CACHE_uiWayNr,
                     _G_ICache.CACHE_uiSetNr);
    }

    if (_G_DCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L1 D-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_DCache.CACHE_uiSize / 1024,
                     _G_DCache.CACHE_uiLineSize,
                     _G_DCache.CACHE_uiWayNr,
                     _G_DCache.CACHE_uiSetNr);
    }

    if (_G_VCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L2 V-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_VCache.CACHE_uiSize / 1024,
                     _G_VCache.CACHE_uiLineSize,
                     _G_VCache.CACHE_uiWayNr,
                     _G_VCache.CACHE_uiSetNr);
        iLevel++;
    }

    if (_G_SCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L%d S-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     iLevel,
                     _G_SCache.CACHE_uiSize / 1024,
                     _G_SCache.CACHE_uiLineSize,
                     _G_SCache.CACHE_uiWayNr,
                     _G_SCache.CACHE_uiSetNr);
    }
}
/*********************************************************************************************************
** ��������: mipsLs3xCacheInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsLs3xCacheInit (LW_CACHE_OP  *pcacheop,
                         CACHE_MODE    uiInstruction,
                         CACHE_MODE    uiData,
                         CPCHAR        pcMachineName)
{
    INT     iError;

    iError = ls3xCacheProbe();
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No Cache!\r\n");
        return;
    }

    ls3xCacheInfoShow();                                                /*  ��ӡ CACHE ��Ϣ             */

    pcacheop->CACHEOP_ulOption = 0ul;                                   /*  ���� TEXT_UPDATE_MP ѡ��    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;                      /*  VIPT                        */
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWayStep;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWayStep;

    pcacheop->CACHEOP_pfuncEnable  = ls3xCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = ls3xCacheDisable;
    /*
     * Loongson3x ʵ���˸��� CACHE ��Ӳ��һ���ԣ����� CACHE �������� NONE ����
     */
    pcacheop->CACHEOP_pfuncFlush          = ls3xCacheFlushNone;
    pcacheop->CACHEOP_pfuncFlushPage      = ls3xCacheFlushPageNone;
    pcacheop->CACHEOP_pfuncInvalidate     = ls3xCacheInvalidateNone;
    pcacheop->CACHEOP_pfuncInvalidatePage = ls3xCacheInvalidatePageNone;
    pcacheop->CACHEOP_pfuncClear          = ls3xCacheClearNone;
    pcacheop->CACHEOP_pfuncClearPage      = ls3xCacheClearPageNone;
    pcacheop->CACHEOP_pfuncTextUpdate     = ls3xCacheTextUpdateNone;
    pcacheop->CACHEOP_pfuncDataUpdate     = ls3xCacheDataUpdateNone;
    
    pcacheop->CACHEOP_pfuncLock           = ls3xCacheLockNone;          /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock         = ls3xCacheUnlockNone;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: mipsLs3xCacheReset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  mipsLs3xCacheReset (CPCHAR  pcMachineName)
{
    INT     iError;

    iError = ls3xCacheProbe();
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No Cache!\r\n");
        return;
    }

    ls3xBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
