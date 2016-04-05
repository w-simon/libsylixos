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
** ��   ��   ��: ppcCache.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "cache/ppc60x/ppcCache60x.h"
/*********************************************************************************************************
** ��������: archCacheInit
** ��������: ��ʼ�� CACHE
** �䡡��  : uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archCacheInit (CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName)
{
    LW_CACHE_OP *pcacheop = API_CacheGetLibBlock();

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s L1 cache controller initialization.\r\n", pcMachineName);

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_750)     == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_MPC83XX) == 0)) {
        ppc60xCacheInit(pcacheop, uiInstruction, uiData, pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}
/*********************************************************************************************************
** ��������: archCacheReset
** ��������: ��λ CACHE, MMU ��ʼ��ʱ��Ҫ���ô˺���
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archCacheReset (CPCHAR  pcMachineName)
{
    if ((lib_strcmp(pcMachineName, PPC_MACHINE_750)     == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_MPC83XX) == 0)) {
        ppc60xCacheReset(pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
