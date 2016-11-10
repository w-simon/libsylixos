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

#ifndef __MIPS_LS3X_CACHE_H
#define __MIPS_LS3X_CACHE_H

VOID  ls3xCacheFlushAll(VOID);
VOID  ls3xCacheEnableHw(VOID);

VOID  mipsLs3xCacheInit(LW_CACHE_OP *pcacheop,
                        CACHE_MODE   uiInstruction,
                        CACHE_MODE   uiData,
                        CPCHAR       pcMachineName);
VOID  mipsLs3xCacheReset(CPCHAR  pcMachineName);

#endif                                                                  /*  __MIPS_LS3X_CACHE_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
