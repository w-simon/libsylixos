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
** ��   ��   ��: armCacheV4.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARMv4 ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARM920 ��ϵ����
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../armCacheCommon.h"
#include "../../mmu/armMmuCommon.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern VOID  armDCacheV4Disable(VOID);
extern VOID  armDCacheV4FlushAll(VOID);
extern VOID  armDCacheV4ClearAll(VOID);
/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/
#define ARMv4_CACHE_LINE_SIZE           32
#define ARMv4_CACHE_LOOP_OP_MAX_SIZE    (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** ��������: armCacheV4Enable
** ��������: ʹ�� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV4Enable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheEnable();
    
    } else {
        armDCacheEnable();
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Disable
** ��������: ���� CACHE 
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  armCacheV4Disable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheDisable();
    
    } else {
        armDCacheV4Disable();
    }
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Flush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Flush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4FlushPage
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4FlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Invalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Invalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & (ARMv4_CACHE_LINE_SIZE - 1)) {                /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~(ARMv4_CACHE_LINE_SIZE - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, ARMv4_CACHE_LINE_SIZE);
                ulStart += ARMv4_CACHE_LINE_SIZE;
            }
            
            if (ulEnd & (ARMv4_CACHE_LINE_SIZE - 1)) {                  /*  ������ַ�� cache line ����  */
                ulEnd &= ~(ARMv4_CACHE_LINE_SIZE - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
            }
                                                                        /*  ����Ч���벿��              */
            armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4InvalidatePage
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4InvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
        }
    } else {
        if (stBytes > 0) {                                              /*  ���� > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & (ARMv4_CACHE_LINE_SIZE - 1)) {                /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~(ARMv4_CACHE_LINE_SIZE - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, ARMv4_CACHE_LINE_SIZE);
                ulStart += ARMv4_CACHE_LINE_SIZE;
            }
            
            if (ulEnd & (ARMv4_CACHE_LINE_SIZE - 1)) {                  /*  ������ַ�� cache line ����  */
                ulEnd &= ~(ARMv4_CACHE_LINE_SIZE - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
            }
                                                                        /*  ����Ч���벿��              */
            armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Clear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Clear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
        }
    } else {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);/*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4ClearPage
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           pvPdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4ClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE ȫ����Ч             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
        }
    } else {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4ClearAll();                                      /*  ȫ����д����Ч              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);/*  ���ֻ�д����Ч              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4Lock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Lock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV4Unlock
** ��������: ����ָ�����͵� CACHE 
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT	armCacheV4Unlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV4TextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV4TextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
        armDCacheV4FlushAll();                                          /*  DCACHE ȫ����д             */
        armICacheInvalidateAll();                                       /*  ICACHE ȫ����Ч             */
        
    } else {
        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
        armDCacheFlush(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);    /*  ���ֻ�д                    */
        armICacheInvalidate(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV4DataUpdate
** ��������: ��д D CACHE (����д CPU ���� CACHE)
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
**           bInv                          �Ƿ�Ϊ��д��Ч
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : L2 cache Ϊͳһ CACHE ���� data update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT	armCacheV4DataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4FlushAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);/*  ���ֻ�д                    */
        }
    
    } else {
        if (stBytes >= ARMv4_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV4ClearAll();                                      /*  ȫ����д                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, ARMv4_CACHE_LINE_SIZE);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, ARMv4_CACHE_LINE_SIZE);/*  ���ֻ�д                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheV4Init
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armCacheV4Init (LW_CACHE_OP *pcacheop, 
                      CACHE_MODE   uiInstruction, 
                      CACHE_MODE   uiData, 
                      CPCHAR       pcMachineName)
{
    pcacheop->CACHEOP_ulOption = 0ul;

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIVT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIVT;
    
    pcacheop->CACHEOP_iICacheLine = ARMv4_CACHE_LINE_SIZE;
    pcacheop->CACHEOP_iDCacheLine = ARMv4_CACHE_LINE_SIZE;
    
    pcacheop->CACHEOP_iICacheWaySize = ((16 * LW_CFG_KB_SIZE) / 64);
    pcacheop->CACHEOP_iDCacheWaySize = ((16 * LW_CFG_KB_SIZE) / 64);
    
    pcacheop->CACHEOP_pfuncEnable  = armCacheV4Enable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV4Disable;
    
    pcacheop->CACHEOP_pfuncLock    = armCacheV4Lock;                    /*  ��ʱ��֧����������          */
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV4Unlock;
    
    pcacheop->CACHEOP_pfuncFlush          = armCacheV4Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = armCacheV4FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV4Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = armCacheV4InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = armCacheV4Clear;
    pcacheop->CACHEOP_pfuncClearPage      = armCacheV4ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV4TextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = armCacheV4DataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: archCacheV4Reset
** ��������: ��λ CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armCacheV4Reset (CPCHAR  pcMachineName)
{
    armICacheInvalidateAll();
    armDCacheV4Disable();
    armICacheDisable();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
