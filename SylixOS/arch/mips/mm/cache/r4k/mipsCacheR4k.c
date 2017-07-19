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
** ��   ��   ��: mipsCacheR4k.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 01 ��
**
** ��        ��: MIPS R4K ��ϵ���� CACHE ����.
**
** BUG:
2016.04.06  Add Cache Init �� CP0_ECC Register Init(loongson2H ֧��)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
#include "arch/mips/mm/cache/mipsCacheCommon.h"
#if LW_CFG_MIPS_CACHE_L2 > 0
#include "arch/mips/mm/cache/l2/mipsL2R4k.h"
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID     mipsCacheR4kDisableHw(VOID);
extern VOID     mipsCacheR4kEnableHw(VOID);

extern VOID     mipsDCacheR4kLineFlush(PCHAR  pcAddr);
extern VOID     mipsDCacheR4kLineClear(PCHAR  pcAddr);
extern VOID     mipsDCacheR4kLineInvalidate(PCHAR  pcAddr);
extern VOID     mipsDCacheR4kIndexClear(PCHAR  pcAddr);
extern VOID     mipsDCacheR4kIndexStoreTag(PCHAR  pcAddr);

extern VOID     mipsICacheR4kLineInvalidate(PCHAR   pcAddr);
extern VOID     mipsICacheR4kIndexInvalidate(PCHAR  pcAddr);
extern VOID     mipsICacheR4kFill(PCHAR  pcAddr);
extern VOID     mipsICacheR4kIndexStoreTag(PCHAR  pcAddr);

extern VOID     mipsBranchPredictionDisable(VOID);
extern VOID     mipsBranchPredictionEnable(VOID);
extern VOID     mipsBranchPredictorInvalidate(VOID);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static INT          _G_iMachineType = MIPS_MACHINE_TYPE_24KF;           /*  ��������                    */
/*********************************************************************************************************
  CACHE ״̬
*********************************************************************************************************/
static INT          _G_iCacheStatus = L1_CACHE_DIS;
/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
static MIPS_CACHE   _G_ICache, _G_DCache;                               /*  I-CACHE �� D-CACHE ��Ϣ     */
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
static BOOL         _G_bHaveTagHi         = LW_FALSE;                   /*  �Ƿ��� TagHi �Ĵ���         */
static BOOL         _G_bHaveFillI         = LW_FALSE;                   /*  �Ƿ��� FillI ����           */
static BOOL         _G_bHaveHitWritebackD = LW_FALSE;                   /*  �Ƿ��� HitWritebackD ����   */
static BOOL         _G_bHaveL2            = LW_FALSE;                   /*  �Ƿ��� L2-CACHE             */
/*********************************************************************************************************
  CACHE ѭ������ʱ���������С, ���ڸô�Сʱ��ʹ�� All ����
*********************************************************************************************************/
#define MIPS_CACHE_LOOP_OP_MAX_SIZE     (8 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  Loongson-2F ������ config0
*********************************************************************************************************/
#define CONF_CU     ( 1  <<  3)
#define CONF_DB     ( 1  <<  4)
#define CONF_IB     ( 1  <<  5)
#define CONF_DC     ( 7  <<  6)
#define CONF_IC     ( 7  <<  9)
#define CONF_EB     ( 1  << 13)
#define CONF_EM     ( 1  << 14)
#define CONF_SM     ( 1  << 16)
#define CONF_SC     ( 1  << 17)
#define CONF_EW     ( 3  << 18)
#define CONF_EP     (15  << 24)
#define CONF_EC     ( 7  << 28)
#define CONF_CM     ( 1  << 31)
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: mipsDCacheR4kClear
** ��������: D-CACHE �����ݻ�д����Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDCacheR4kClear (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsDCacheR4kLineClear(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kFlush
** ��������: D-CACHE �����ݻ�д
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDCacheR4kFlush (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    REGISTER PCHAR   pcAddr;

    if (_G_bHaveHitWritebackD) {
        for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
            mipsDCacheR4kLineFlush(pcAddr);
        }
        MIPS_PIPE_FLUSH();

    } else {
        mipsDCacheR4kClear(pvStart, pvEnd, uiStep);
    }
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kInvalidate
** ��������: D-CACHE ��������Ч
** �䡡��  : pvStart        ��ʼ��ַ
**           pvEnd          ������ַ
**           uiStep         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDCacheR4kInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsDCacheR4kLineInvalidate(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsICacheR4kInvalidate
** ��������: I-CACHE ��������Ч
** �䡡��  : pvStart       ��ʼ��ַ
**           pvEnd         ������ַ
**           uiStep        ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsICacheR4kInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsICacheR4kLineInvalidate(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kClearAll
** ��������: D-CACHE �������ݻ�д����Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsDCacheR4kClearAll (VOID)
{
    REGISTER INT     iWay;
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    REGISTER PCHAR   pcEndAddr  = (PCHAR)(A_K0BASE + _G_DCache.CACHE_uiWayStep);

    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        for (iWay = 0; iWay < _G_DCache.CACHE_uiWayNr; iWay++) {
            mipsDCacheR4kIndexClear(pcLineAddr + (iWay << _G_DCache.CACHE_uiWayBit));
        }
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsDCacheR4kFlushAll
** ��������: D-CACHE �������ݻ�д
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDCacheR4kFlushAll (VOID)
{
    mipsDCacheR4kClearAll();
}
/*********************************************************************************************************
** ��������: mipsICacheR4kInvalidateAll
** ��������: I-CACHE ����������Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID    mipsICacheR4kInvalidateAll (VOID)
{
    REGISTER INT     iWay;
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    REGISTER PCHAR   pcEndAddr  = (PCHAR)(A_K0BASE + _G_ICache.CACHE_uiSize);

    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
        for (iWay = 0; iWay < _G_ICache.CACHE_uiWayNr; iWay++) {
            mipsICacheR4kIndexInvalidate(pcLineAddr + (iWay << _G_ICache.CACHE_uiWayBit));
        }
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** ��������: mipsCacheR4kEnable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_D_EN;
        }
    }
    
    if (_G_iCacheStatus == L1_CACHE_EN) {
        mipsCacheR4kEnableHw();
        mipsBranchPredictionEnable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kDisable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_D_EN;
        }
    }
    
    if (_G_iCacheStatus == L1_CACHE_DIS) {
        mipsCacheR4kDisableHw();
        mipsBranchPredictionDisable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes == 0) {
    	return  (ERROR_NONE);
    }

    if (cachetype == DATA_CACHE) {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (_G_bHaveL2) {
            return  (mipsL2R4kFlush(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsDCacheR4kFlushAll();                                    /*  ȫ����д                    */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mipsDCacheR4kFlush(pvAdrs, (PVOID)ulEnd,                    /*  ���ֻ�д                    */
                               _G_DCache.CACHE_uiLineSize);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kFlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��:
*********************************************************************************************************/
static INT  mipsCacheR4kFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes == 0) {
    	return  (ERROR_NONE);
    }

    if (cachetype == DATA_CACHE) {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (_G_bHaveL2) {
            return  (mipsL2R4kFlush(pvAdrs, stBytes));
        }
#endif
    	if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
    		mipsDCacheR4kFlushAll();                                    /*  ȫ����д                    */
        
    	} else {
    		MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
    		mipsDCacheR4kFlush(pvAdrs, (PVOID)ulEnd,                    /*  ���ֻ�д                    */
                               _G_DCache.CACHE_uiLineSize);
    	}
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes == 0) {
    	return  (ERROR_NONE);
    }

    if (cachetype == INSTRUCTION_CACHE) {
    	if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
    		mipsICacheR4kInvalidateAll();                               /*  ICACHE ȫ����Ч             */

    	} else {
    		MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
    		mipsICacheR4kInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    	}

    } else {
#if LW_CFG_MIPS_CACHE_L2 > 0
    	if (_G_bHaveL2) {
            return  (mipsL2R4kInvalidate(pvAdrs, stBytes));
    	}
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    	addr_t  ulStart = (addr_t)pvAdrs;
    	        ulEnd   = ulStart + stBytes;
            
    	if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {       /*  ��ʼ��ַ�� cache line ����  */
    	    ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
    	    mipsDCacheR4kClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
    	    ulStart += _G_DCache.CACHE_uiLineSize;
    	}
            
    	if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {         /*  ������ַ�� cache line ����  */
    		ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
    		mipsDCacheR4kClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
    	}
                                                                        /*  ����Ч���벿��              */
    	mipsDCacheR4kInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kInvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��:
*********************************************************************************************************/
static INT  mipsCacheR4kInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes == 0) {
    	return  (ERROR_NONE);
    }

    if (cachetype == INSTRUCTION_CACHE) {
    	if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
    		mipsICacheR4kInvalidateAll();                               /*  ICACHE ȫ����Ч             */

    	} else {
    		MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
    		mipsICacheR4kInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    	}

    } else {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (_G_bHaveL2) {
            return  (mipsL2R4kInvalidate(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    	addr_t  ulStart = (addr_t)pvAdrs;
    	        ulEnd   = ulStart + stBytes;
                    
    	if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {       /*  ��ʼ��ַ�� cache line ����  */
    		ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
    		mipsDCacheR4kClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
    		ulStart += _G_DCache.CACHE_uiLineSize;
    	}
            
    	if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {         /*  ������ַ�� cache line ����  */
    		ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
    		mipsDCacheR4kClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
    	}
                                                                        /*  ����Ч���벿��              */
    	mipsDCacheR4kInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes == 0) {
    	return  (ERROR_NONE);
    }

    if (cachetype == INSTRUCTION_CACHE) {
    	if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
    		mipsICacheR4kInvalidateAll();                               /*  ICACHE ȫ����Ч             */

    	} else {
    		MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
    		mipsICacheR4kInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    	}

    } else {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (_G_bHaveL2) {
            return  (mipsL2R4kClear(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    	if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
    		mipsDCacheR4kClearAll();                                    /*  ȫ����д����Ч              */

    	} else {
    		MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
    		mipsDCacheR4kClear(pvAdrs, (PVOID)ulEnd,
                               _G_DCache.CACHE_uiLineSize);             /*  ���ֻ�д����Ч              */
    	}
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
	addr_t  ulEnd;
    
    if (stBytes == 0) {
    	return  (ERROR_NONE);
    }

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheR4kInvalidateAll();                               /*  ICACHE ȫ����Ч             */
            
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mipsICacheR4kInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }

    } else {
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (_G_bHaveL2) {
            return  (mipsL2R4kClear(pvAdrs, stBytes));
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
		if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
			mipsDCacheR4kClearAll();                                    /*  ȫ����д����Ч              */

		} else {
			MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
			mipsDCacheR4kClear(pvAdrs, (PVOID)ulEnd,
							   _G_DCache.CACHE_uiLineSize);             /*  ���ֻ�д����Ч              */
		}
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kLock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kUnlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  mipsCacheR4kTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes == 0) {
    	return  (ERROR_NONE);
    }

    if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
        mipsDCacheR4kFlushAll();                                        /*  DCACHE ȫ����д             */
        mipsICacheR4kInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
    } else {
    	PVOID   pvAdrsBak = pvAdrs;

        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        mipsDCacheR4kFlush(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  ���ֻ�д                    */

        MIPS_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
        mipsICacheR4kInvalidate(pvAdrsBak, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kDataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
INT  mipsCacheR4kDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (stBytes == 0) {
    	return  (ERROR_NONE);
    }

    if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {                       /*  ȫ����д                    */
        if (bInv) {
            mipsDCacheR4kClearAll();
        } else {
            mipsDCacheR4kFlushAll();
        }

    } else {                                                            /*  ���ֻ�д                    */
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        if (bInv) {
            mipsDCacheR4kClear(pvAdrs, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
        } else {
            mipsDCacheR4kFlush(pvAdrs, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kL1Probe
** ��������: L1 CACHE ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsCacheR4kL1Probe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiTemp;

    _G_ICache.CACHE_bPresent = LW_FALSE;
    _G_DCache.CACHE_bPresent = LW_FALSE;

    uiConfig = mipsCp0ConfigRead();                                     /*  �� Config0                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  û�� Config1, �˳�          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config1 Register!\r\n");
        return;
    }

    switch (_G_uiMipsCpuType) {

    case CPU_LOONGSON2:                                                 /*  loongson-2F                 */
    	_G_ICache.CACHE_uiSize      = 1  << (12 + ((uiConfig & CONF_IC) >> 9));
    	_G_ICache.CACHE_uiLineSize  = 16 << ((uiConfig & CONF_IB) >> 5);
    	if (_G_uiMipsProcessorId & 0x3) {
    		_G_ICache.CACHE_uiWayNr = 4;
    	} else {
    		_G_ICache.CACHE_uiWayNr = 2;
    	}
        /*
         * LOONGSON2 has 4 way icache, but when using indexed cache op,
         * one op will act on all 4 ways
         */
    	_G_ICache.CACHE_uiWayBit    = 1;
    	_G_ICache.CACHE_bPresent    = LW_TRUE;

    	_G_DCache.CACHE_uiSize      = 1 << (12 + ((uiConfig & CONF_DC) >> 6));
    	_G_DCache.CACHE_uiLineSize  = 16 << ((uiConfig & CONF_DB) >> 4);
    	if (_G_uiMipsProcessorId & 0x3) {
    		_G_DCache.CACHE_uiWayNr = 4;
    	} else {
    		_G_DCache.CACHE_uiWayNr = 2;
    	}
    	_G_DCache.CACHE_uiWayBit    = 0;
    	_G_DCache.CACHE_bPresent    = LW_TRUE;
    	break;

    case CPU_LOONGSON3:                                                 /*  loongson-2H                 */
	    uiConfig = mipsCp0Config1Read();                                /*  �� Config1                  */

	    uiTemp = (uiConfig >> 19) & 7;
		if (uiTemp) {
			_G_ICache.CACHE_uiLineSize = 2 << uiTemp;
		} else {
			_G_ICache.CACHE_uiLineSize = 0;
		}
		_G_ICache.CACHE_uiSetNr  = 64 << ((uiConfig >> 22) & 7);
		_G_ICache.CACHE_uiWayNr  = 1 + ((uiConfig >> 16) & 7);
		_G_ICache.CACHE_uiSize   = _G_ICache.CACHE_uiSetNr *
								   _G_ICache.CACHE_uiWayNr * _G_ICache.CACHE_uiLineSize;
		_G_ICache.CACHE_uiWayBit = 0;
	    _G_ICache.CACHE_bPresent = LW_TRUE;

		uiTemp = (uiConfig >> 10) & 7;
		if (uiTemp) {
			_G_DCache.CACHE_uiLineSize = 2 << uiTemp;
		} else {
			_G_DCache.CACHE_uiLineSize = 0;
		}
		_G_DCache.CACHE_uiSetNr  = 64 << ((uiConfig >> 13) & 7);
		_G_DCache.CACHE_uiWayNr  = 1 + ((uiConfig >> 7) & 7);
		_G_DCache.CACHE_uiSize   = _G_DCache.CACHE_uiSetNr *
								   _G_DCache.CACHE_uiWayNr * _G_DCache.CACHE_uiLineSize;
		_G_DCache.CACHE_uiWayBit = 0;
		_G_DCache.CACHE_bPresent = LW_TRUE;
    	break;

    default:
		/*
		 * So we seem to be a MIPS32 or MIPS64 CPU
		 * So let's probe the I-cache ...
		 */
	    uiConfig = mipsCp0Config1Read();                                /*  �� Config1                  */

	    uiTemp = (uiConfig >> 19) & 7;

		_G_ICache.CACHE_uiLineSize = uiTemp ? 2 << uiTemp : 0;
		_G_ICache.CACHE_uiSetNr    = 32 << (((uiConfig >> 22) + 1) & 7);
		_G_ICache.CACHE_uiWayNr    = 1 + ((uiConfig >> 16) & 7);
		_G_ICache.CACHE_uiSize     = _G_ICache.CACHE_uiSetNr *
				                     _G_ICache.CACHE_uiWayNr *
				                     _G_ICache.CACHE_uiLineSize;
		_G_ICache.CACHE_uiWayBit   = lib_ffs(_G_ICache.CACHE_uiSize / _G_ICache.CACHE_uiWayNr) - 1;
		_G_ICache.CACHE_bPresent   = LW_TRUE;

		/*
		 * Now probe the MIPS32 / MIPS64 data cache.
		 */
		uiTemp = (uiConfig >> 10) & 7;
		_G_DCache.CACHE_uiLineSize = uiTemp ? 2 << uiTemp : 0;
		_G_DCache.CACHE_uiSetNr    = 32 << (((uiConfig >> 13) + 1) & 7);
		_G_DCache.CACHE_uiWayNr    = 1 + ((uiConfig >> 7) & 7);
		_G_DCache.CACHE_uiSize     = _G_DCache.CACHE_uiSetNr *
				                     _G_DCache.CACHE_uiWayNr *
				                     _G_DCache.CACHE_uiLineSize;
		_G_DCache.CACHE_uiWayBit   = lib_ffs(_G_DCache.CACHE_uiSize / _G_DCache.CACHE_uiWayNr) - 1;
		_G_DCache.CACHE_bPresent   = LW_TRUE;
		break;
	}

	/*
	 * compute a couple of other cache variables
	 */
	_G_ICache.CACHE_uiWayStep = _G_ICache.CACHE_uiSize / _G_ICache.CACHE_uiWayNr;
	_G_DCache.CACHE_uiWayStep = _G_DCache.CACHE_uiSize / _G_DCache.CACHE_uiWayNr;

	if (_G_ICache.CACHE_bPresent) {
	    _DebugFormat(__LOGMESSAGE_LEVEL,"L1 I-CACHE size %dKB (%d line size, %d way, %d set, %d waybit).\r\n",
	                 _G_ICache.CACHE_uiSize / 1024,
	                 _G_ICache.CACHE_uiLineSize,
	                 _G_ICache.CACHE_uiWayNr,
	                 _G_ICache.CACHE_uiSetNr,
	                 _G_ICache.CACHE_uiWayBit);
	}

    if (_G_DCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL,"L1 D-CACHE size %dKB (%d line size, %d way, %d set, %d waybit).\r\n",
                     _G_DCache.CACHE_uiSize / 1024,
                     _G_DCache.CACHE_uiLineSize,
                     _G_DCache.CACHE_uiWayNr,
                     _G_DCache.CACHE_uiSetNr,
                     _G_DCache.CACHE_uiWayBit);
    }
}
/*********************************************************************************************************
** ��������: mipsCacheR4kProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName  ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  mipsCacheR4kProbe (CPCHAR   pcMachineName)
{
    static BOOL    bIsProbed = LW_FALSE;

    if (bIsProbed) {
        return  (ERROR_NONE);
    }

    mipsCpuProbe();

    if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS1X;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS2X;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS3X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS3X;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_JZ47XX) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_JZ47XX;
    }

    mipsCacheR4kL1Probe();

#if LW_CFG_MIPS_CACHE_L2 > 0
    if (mipsL2R4kProbe() == ERROR_NONE) {
        _G_bHaveL2 = LW_TRUE;
    }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    bIsProbed = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsCacheR4kInitHw
** ��������: CACHE Ӳ����ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsCacheR4kInitHw (VOID)
{
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    REGISTER PCHAR   pcEndAddr;
    REGISTER PCHAR   pcWayEndAddr;
    REGISTER CHAR    cTemp;
    REGISTER INT32   iWay;

    (VOID)cTemp;

    mipsCp0TagLoWrite(0);

    if (_G_bHaveTagHi) {
        mipsCp0TagHiWrite(0);
    }

    if (_G_iMachineType == MIPS_MACHINE_TYPE_LS2X) {
        mipsCp0ECCWrite(0x00);
    }

    pcEndAddr     = (PCHAR)(A_K0BASE + _G_ICache.CACHE_uiSize);
    pcWayEndAddr  = (PCHAR)(A_K0BASE + _G_ICache.CACHE_uiWayStep);

    /*
     * clear tag to invalidate
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
    	for (iWay = 0; iWay < _G_ICache.CACHE_uiWayNr; iWay ++) {
    		mipsICacheR4kIndexStoreTag(pcLineAddr + (iWay << _G_ICache.CACHE_uiWayBit));
    	}
    }

    if (_G_bHaveFillI) {
        /*
         * fill so data field parity is correct
         */
    	for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
    		mipsICacheR4kFill(pcLineAddr);
    	}
        /*
         * invalidate again �C prudent but not strictly necessay
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
        	for (iWay = 0; iWay < _G_ICache.CACHE_uiWayNr; iWay ++) {
        		mipsICacheR4kIndexStoreTag(pcLineAddr + (iWay << _G_ICache.CACHE_uiWayBit));
        	}
        }
    }

    if (_G_iMachineType == MIPS_MACHINE_TYPE_LS2X) {
        mipsCp0ECCWrite(0x22);
    }

    pcEndAddr     = (PCHAR)(A_K0BASE + _G_DCache.CACHE_uiSize);
    pcWayEndAddr  = (PCHAR)(A_K0BASE + _G_DCache.CACHE_uiWayStep);

    /*
     * clear all tags
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
    	for (iWay = 0; iWay < _G_DCache.CACHE_uiWayNr; iWay ++) {
    		mipsDCacheR4kIndexStoreTag(pcLineAddr + (iWay << _G_DCache.CACHE_uiWayBit));
    	}
    }

    /*
     * load from each line (in cached space)
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        cTemp = *pcLineAddr;
    }

    /*
     * clear all tags again
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcWayEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
    	for (iWay = 0; iWay < _G_DCache.CACHE_uiWayNr; iWay ++) {
    		mipsDCacheR4kIndexStoreTag(pcLineAddr + (iWay << _G_DCache.CACHE_uiWayBit));
    	}
    }

#if LW_CFG_MIPS_CACHE_L2 > 0
    mipsL2R4kInitHw();
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
}
/*********************************************************************************************************
** ��������: mipsCacheR4kInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsCacheR4kInit (LW_CACHE_OP *pcacheop,
                       CACHE_MODE   uiInstruction,
                       CACHE_MODE   uiData,
                       CPCHAR       pcMachineName)
{
    INT     iError;

    iError = mipsCacheR4kProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No Cache!\r\n");
        return;
    }

    mipsCacheR4kDisableHw();
    mipsCacheR4kInitHw();

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

    pcacheop->CACHEOP_pfuncEnable  = mipsCacheR4kEnable;
    pcacheop->CACHEOP_pfuncDisable = mipsCacheR4kDisable;
    
    pcacheop->CACHEOP_pfuncLock   = mipsCacheR4kLock;                    /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock = mipsCacheR4kUnlock;
    
    pcacheop->CACHEOP_pfuncFlush          = mipsCacheR4kFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = mipsCacheR4kFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = mipsCacheR4kInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = mipsCacheR4kInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = mipsCacheR4kClear;
    pcacheop->CACHEOP_pfuncClearPage      = mipsCacheR4kClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = mipsCacheR4kTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = mipsCacheR4kDataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: mipsCacheR4kReset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����� lockdown �������� unlock & invalidate ��������
*********************************************************************************************************/
VOID  mipsCacheR4kReset (CPCHAR  pcMachineName)
{
    INT     iError;

    iError = mipsCacheR4kProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No Cache!\r\n");
        return;
    }

    mipsCacheR4kDisableHw();
    mipsBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
