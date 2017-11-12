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
** ��   ��   ��: armCacheV6.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARMv6 ��ϵ���� CACHE ����.
**
** BUG:
2015.08.21  ���� Invalidate ����������ַ�������.
2016.04.29  ���� data update ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARM1176 ��ϵ����
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../armCacheCommon.h"
#include "../../mmu/armMmuCommon.h"
#include "../../../common/cp15/armCp15.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern VOID  armDCacheV6Disable(VOID);
extern VOID  armDCacheV6FlushAll(VOID);
extern VOID  armDCacheV6ClearAll(VOID);
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
#define ARMv6_CACHE_LINE_SIZE           32
#define ARMv6_CACHE_LOOP_OP_MAX_SIZE    (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** ��������: armCacheV6Enable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV6Enable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheEnable();
        armBranchPredictionEnable();
        
    } else {
        armDCacheEnable();
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6Disable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV6Disable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheDisable();
        armBranchPredictionDisable();
        
    } else {
        armDCacheV6Disable();
    }
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6Flush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV6Flush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV6FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6FlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV6FlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV6FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6Invalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV6Invalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & (ARMv6_CACHE_LINE_SIZE - 1)) {                /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~(ARMv6_CACHE_LINE_SIZE - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, ARMv6_CACHE_LINE_SIZE);
                ulStart += ARMv6_CACHE_LINE_SIZE;
            }
            
            if (ulEnd & (ARMv6_CACHE_LINE_SIZE - 1)) {                  /*  ������ַ�� cache line ����  */
                ulEnd &= ~(ARMv6_CACHE_LINE_SIZE - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
            }
                                                                        /*  ����Ч���벿��              */
            armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6InvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV6InvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & (ARMv6_CACHE_LINE_SIZE - 1)) {                /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~(ARMv6_CACHE_LINE_SIZE - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, ARMv6_CACHE_LINE_SIZE);
                ulStart += ARMv6_CACHE_LINE_SIZE;
            }
            
            if (ulEnd & (ARMv6_CACHE_LINE_SIZE - 1)) {                  /*  ������ַ�� cache line ����  */
                ulEnd &= ~(ARMv6_CACHE_LINE_SIZE - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
            }
                                                                        /*  ����Ч���벿��              */
            armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6Clear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV6Clear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
        }
    } else {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV6ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);/*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6ClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV6ClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
        }
    } else {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV6ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);/*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6Lock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV6Lock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV6Unlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV6Unlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV6TextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV6TextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
        armDCacheV6FlushAll();                                          /*  DCACHE ȫ����д             */
        armICacheInvalidateAll();                                       /*  ICACHE ȫ����Ч             */
        
    } else {
        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
        armDCacheFlush(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);    /*  ���ֻ�д                    */
        armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV6DataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV6DataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV6FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);/*  ���ֻ�д                    */
        }
    
    } else {
        if (stBytes >= ARMv6_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV6ClearAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv6_CACHE_LINE_SIZE);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, ARMv6_CACHE_LINE_SIZE);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheV6Init
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armCacheV6Init (LW_CACHE_OP *pcacheop, 
                      CACHE_MODE   uiInstruction, 
                      CACHE_MODE   uiData, 
                      CPCHAR       pcMachineName)
{
#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;
    
    pcacheop->CACHEOP_iICacheLine = ARMv6_CACHE_LINE_SIZE;
    pcacheop->CACHEOP_iDCacheLine = ARMv6_CACHE_LINE_SIZE;
    
    pcacheop->CACHEOP_iICacheWaySize = (4 * LW_CFG_KB_SIZE);
    pcacheop->CACHEOP_iDCacheWaySize = (4 * LW_CFG_KB_SIZE);
    
    pcacheop->CACHEOP_pfuncEnable  = armCacheV6Enable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV6Disable;
    
    pcacheop->CACHEOP_pfuncLock    = armCacheV6Lock;                    /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV6Unlock;
    
    pcacheop->CACHEOP_pfuncFlush          = armCacheV6Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = armCacheV6FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV6Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = armCacheV6InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = armCacheV6Clear;
    pcacheop->CACHEOP_pfuncClearPage      = armCacheV6ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV6TextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = armCacheV6DataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: archCacheV6Reset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armCacheV6Reset (CPCHAR  pcMachineName)
{
    armICacheInvalidateAll();
    armDCacheV6Disable();
    armICacheDisable();
    armBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
