/**********************************************************************************************************
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
** ��   ��   ��: mips32Cache.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 01 ��
**
** ��        ��: MIPS32 ��ϵ���� CACHE ����.
**
** BUG:
2016.04.06  Add Cache Init ��CP0_ECC Register Init(loongson2H֧��)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  L2 CACHE ֧��
*********************************************************************************************************/
#if LW_CFG_MIPS_CACHE_L2 > 0
/*********************************************************************************************************
  L1 CACHE ״̬
*********************************************************************************************************/
static INT      iCacheStatus = 0;
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern VOID     mips32DCacheDisable(VOID);
extern VOID     mips32DCacheEnable(VOID);
extern VOID     mips32ICacheDisable(VOID);
extern VOID     mips32ICacheEnable(VOID);

extern VOID     mips32BranchPredictionDisable(VOID);
extern VOID     mips32BranchPredictionEnable(VOID);
extern VOID     mips32BranchPredictorInvalidate(VOID);

extern VOID     mips32DCacheLineFlush(PCHAR  pcAddr);
extern VOID     mips32DCacheLineClear(PCHAR  pcAddr);
extern VOID     mips32DCacheLineInvalidate(PCHAR  pcAddr);

extern VOID     mips32DCacheIndexClear(PCHAR  pcAddr);

extern VOID     mips32ICacheLineInvalidate(PCHAR   pcAddr);
extern VOID     mips32ICacheIndexInvalidate(PCHAR  pcAddr);

extern VOID     mips32ICacheFill(PCHAR  pcAddr);
extern VOID     mips32ICacheIndexStoreTag(PCHAR  pcAddr);
extern VOID     mips32DCacheIndexStoreTag(PCHAR  pcAddr);
/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
static BOOL     _G_bHaveTagHi         = LW_FALSE;                       /*  �Ƿ��� TagHi �Ĵ���         */
static BOOL     _G_bHaveFillI         = LW_FALSE;                       /*  �Ƿ��� FillI ����           */
static BOOL     _G_bHaveHitWritebackD = LW_FALSE;                       /*  �Ƿ��� HitWritebackD ����   */

typedef struct {
    BOOL        CACHE_bPresent;                                         /*  �Ƿ���� Cache              */
    UINT32      CACHE_uiSize;                                           /*  Cache ��С                  */
    UINT32      CACHE_uiLineSize;                                       /*  Cache �д�С                */
    UINT32      CACHE_uiSetNr;                                          /*  ����                        */
    UINT32      CACHE_uiWayNr;                                          /*  ·��                        */
    UINT32      CACHE_uiWayStep;                                        /*  ·����                      */
} MIPS_CACHE;

static MIPS_CACHE   _G_ICache, _G_DCache;                               /*  I-Cache �� D-Cache ��Ϣ     */
/*********************************************************************************************************
  CACHE ѭ������ʱ���������С, ���ڸô�Сʱ��ʹ�� All ����
*********************************************************************************************************/
#define MIPS_CACHE_LOOP_OP_MAX_SIZE     (8 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/
#define MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)              \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE ��д����
*********************************************************************************************************/
#define MIPS_PIPE_FLUSH()               MIPS_EXEC_INS("sync")
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: mips32DCacheClear
** ��������: D-CACHE �����ݻ�д����Ч
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID    mips32DCacheClear (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mips32DCacheLineClear(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mips32DCacheFlush
** ��������: D-CACHE �����ݻ�д
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID    mips32DCacheFlush (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    PCHAR   pcAddr;

    if (_G_bHaveHitWritebackD) {
        for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
            mips32DCacheLineFlush(pcAddr);
        }
        MIPS_PIPE_FLUSH();
    } else {
        mips32DCacheClear(pvStart, pvEnd, uiStep);
    }
}
/*********************************************************************************************************
** ��������: mips32DCacheInvalidate
** ��������: D-CACHE ��������Ч
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID    mips32DCacheInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mips32DCacheLineInvalidate(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mips32ICacheInvalidate
** ��������: I-CACHE ��������Ч
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID    mips32ICacheInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mips32ICacheLineInvalidate(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mips32DCacheClearAll
** ��������: D-CACHE �������ݻ�д����Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID    mips32DCacheClearAll (VOID)
{
    INT     iWay;
    PCHAR   pcAddr;
    PCHAR   pcLineAddr;
    PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    PCHAR   pcEndAddr  = (PCHAR)(A_K0BASE + _G_DCache.CACHE_uiSize);

    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        for (pcAddr = pcLineAddr, iWay = 0;
             iWay < _G_DCache.CACHE_uiWayNr;
             iWay++, pcAddr += _G_DCache.CACHE_uiWayStep) {
            mips32DCacheIndexClear(pcAddr);
        }
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mips32DCacheFlushAll
** ��������: D-CACHE �������ݻ�д
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID    mips32DCacheFlushAll (VOID)
{
    mips32DCacheClearAll();
}
/*********************************************************************************************************
** ��������: mips32ICacheInvalidateAll
** ��������: I-CACHE ����������Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID    mips32ICacheInvalidateAll (VOID)
{
    INT     iWay;
    PCHAR   pcAddr;
    PCHAR   pcLineAddr;
    PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    PCHAR   pcEndAddr  = (PCHAR)(A_K0BASE + _G_ICache.CACHE_uiSize);

    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
        for (pcAddr = pcLineAddr, iWay = 0;
             iWay < _G_ICache.CACHE_uiWayNr;
             iWay++, pcAddr += _G_ICache.CACHE_uiWayStep) {
            mips32ICacheIndexInvalidate(pcAddr);
        }
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mips32CacheEnable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mips32CacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        mips32ICacheEnable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        mips32BranchPredictionEnable();

    } else {
        mips32DCacheEnable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }
    
#if LW_CFG_MIPS_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) && 
        (iCacheStatus == L1_CACHE_EN)) {
        mips32L2Enable();
    }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheDisable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mips32CacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        mips32ICacheDisable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        mips32BranchPredictionDisable();
        
    } else {
        mips32DCacheDisable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }
    
#if LW_CFG_MIPS_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) && 
        (iCacheStatus == L1_CACHE_DIS)) {
        mips32L2Disable();
    }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����дָ��.
*********************************************************************************************************/
static INT	mips32CacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32DCacheFlushAll();                                     /*  ȫ����д                    */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mips32DCacheFlush(pvAdrs, (PVOID)ulEnd,                     /*  ���ֻ�д                    */
                              _G_DCache.CACHE_uiLineSize);
        }
        
#if LW_CFG_MIPS_CACHE_L2 > 0
        mips32L2FlushAll();
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	mips32CacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32DCacheFlushAll();                                     /*  ȫ����д                    */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mips32DCacheFlush(pvAdrs, (PVOID)ulEnd,                     /*  ���ֻ�д                    */
                              _G_DCache.CACHE_uiLineSize);
        }
        
#if LW_CFG_MIPS_CACHE_L2 > 0
        mips32L2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ (pvAdrs ������������ַ)
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺���������� DCACHE pvAdrs �����ַ�������ַ������ͬ.
*********************************************************************************************************/
static INT	mips32CacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32ICacheInvalidateAll();                                /*  ICACHE ȫ����Ч             */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                mips32DCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }
            
            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                mips32DCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  ����Ч���벿��              */
            mips32DCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            
#if LW_CFG_MIPS_CACHE_L2 > 0
            mips32L2Invalidate(pvAdrs, stBytes);                        /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	mips32CacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32ICacheInvalidateAll();                                /*  ICACHE ȫ����Ч             */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                mips32DCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }
            
            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  ������ַ�� cache line ����  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                mips32DCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  ����Ч���벿��              */
            mips32DCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            
#if LW_CFG_MIPS_CACHE_L2 > 0
            mips32L2Invalidate(pvPdrs, stBytes);                        /*  �����������ַ������ͬ      */
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���� L2 Ϊ�����ַ tag ����������ʱʹ�� L2 ȫ����д����Чָ��.
*********************************************************************************************************/
static INT	mips32CacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32ICacheInvalidateAll();                                /*  ICACHE ȫ����Ч             */
            
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32DCacheClearAll();                                     /*  ȫ����д����Ч              */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mips32DCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  ���ֻ�д����Ч              */
        }
        
#if LW_CFG_MIPS_CACHE_L2 > 0
        mips32L2ClearAll();
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	mips32CacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32ICacheInvalidateAll();                                /*  ICACHE ȫ����Ч             */
            
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32DCacheClearAll();                                     /*  ȫ����д����Ч              */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mips32DCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  ���ֻ�д����Ч              */
        }
        
#if LW_CFG_MIPS_CACHE_L2 > 0
        mips32L2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	mips32CacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mips32CacheUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	mips32CacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mips32CacheTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	mips32CacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
        mips32DCacheFlushAll();                                         /*  DCACHE ȫ����д             */
        mips32ICacheInvalidateAll();                                    /*  ICACHE ȫ����Ч             */
        
    } else {
    	PVOID   pvAdrsBak = pvAdrs;

        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);

        mips32DCacheFlush(pvAdrs, (PVOID)ulEnd,
                          _G_DCache.CACHE_uiLineSize);                  /*  ���ֻ�д                    */

        MIPS_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);

        mips32ICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheProbe
** ��������: CACHE ̽��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mips32CacheProbe (VOID)
{
    UINT32  uiTemp;
    UINT32  uiConfig = mipsCp0ConfigRead();                             /*  �� Config0                  */

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

    uiTemp   = (uiConfig & M_Config1DL) >> S_Config1DL;
    if (uiTemp) {
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

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mips32CacheProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName  ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  mips32CacheHwInit (CPCHAR   pcMachineName)
{
    PCHAR   pcLineAddr;
    PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    PCHAR   pcEndAddr;
    CHAR    cTemp;

    (VOID)cTemp;

    mipsCp0TagLoWrite(0);

    if (_G_bHaveTagHi) {
        mipsCp0TagHiWrite(0);
    }

    if (lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X) == 0) {
        mipsCp0ECCWrite(0x00);
    }

    pcEndAddr = (PCHAR)(A_K0BASE + _G_ICache.CACHE_uiSize);
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
        /*
         * clear tag to invalidate
         */
        mips32ICacheIndexStoreTag(pcLineAddr);

        if (_G_bHaveFillI) {
            /*
             * fill so data field parity is correct
             */
            mips32ICacheFill(pcLineAddr);

            /*
             * invalidate again �C prudent but not strictly necessay
             */
            mips32ICacheIndexStoreTag(pcLineAddr);
        }
    }

    if (lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X) == 0) {
        mipsCp0ECCWrite(0x22);
    }

    pcEndAddr = (PCHAR)(A_K0BASE + _G_DCache.CACHE_uiSize);
    /*
     * clear all tags
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        mips32DCacheIndexStoreTag(pcLineAddr);
    }

    /*
     * load from each line (in cached space)
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        cTemp = *pcLineAddr;
    }

    /*
     * clear all tags
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        mips32DCacheIndexStoreTag(pcLineAddr);
    }

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
VOID  mips32CacheInit (LW_CACHE_OP *pcacheop,
                       CACHE_MODE   uiInstruction,
                       CACHE_MODE   uiData,
                       CPCHAR       pcMachineName)
{
    INT     iError;

    if (lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X) == 0) {
        _G_bHaveTagHi         = LW_FALSE;
        _G_bHaveFillI         = LW_FALSE;
        _G_bHaveHitWritebackD = LW_FALSE;
    }

    iError = mips32CacheProbe();
    if (iError != ERROR_NONE) {
        return;
    }

    iError = mips32CacheHwInit(pcMachineName);
    if (iError != ERROR_NONE) {
        return;
    }

#if LW_CFG_MIPS_CACHE_L2 > 0
    mips32L2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    if (_G_ICache.CACHE_uiLineSize && _G_DCache.CACHE_uiLineSize) {
        pcacheop->CACHEOP_iCacheLine = min(_G_ICache.CACHE_uiLineSize, _G_DCache.CACHE_uiLineSize);
    } else {
        pcacheop->CACHEOP_iCacheLine = max(_G_ICache.CACHE_uiLineSize, _G_DCache.CACHE_uiLineSize);
    }

    pcacheop->CACHEOP_iILoc                 = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iDLoc                 = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iCacheWaySize 		= _G_DCache.CACHE_uiWayStep;

    pcacheop->CACHEOP_pfuncEnable           = mips32CacheEnable;
    pcacheop->CACHEOP_pfuncDisable          = mips32CacheDisable;
    
    pcacheop->CACHEOP_pfuncLock             = mips32CacheLock;          /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock           = mips32CacheUnlock;
    
    pcacheop->CACHEOP_pfuncFlush            = mips32CacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage        = mips32CacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate       = mips32CacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage   = mips32CacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear            = mips32CacheClear;
    pcacheop->CACHEOP_pfuncClearPage        = mips32CacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate       = mips32CacheTextUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc        = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign   = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree          = API_VmmDmaFree;
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
VOID  mips32CacheReset (CPCHAR  pcMachineName)
{
    mips32ICacheInvalidateAll();
    mips32DCacheDisable();
    mips32ICacheDisable();
    mips32BranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
