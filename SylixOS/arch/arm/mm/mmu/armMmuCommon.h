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
** ��   ��   ��: armMmuCommon.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ�ܹ� MMU ͨ�ú���֧��.
*********************************************************************************************************/

#ifndef __ARMMMUCOMMON_H
#define __ARMMMUCOMMON_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

UINT32  armMmuAbtFaultStatus(VOID);
UINT32  armMmuPreFaultStatus(VOID);
UINT32  armMmuAbtFaultAddr(VOID);
VOID    armMmuInitSysRom(VOID);
VOID    armMmuEnable(VOID);
VOID    armMmuDisable(VOID);
VOID    armMmuEnableWriteBuffer(VOID);
VOID    armMmuDisableWriteBuffer(VOID);
VOID    armMmuEnableAlignFault(VOID);
VOID    armMmuDisableAlignFault(VOID);
VOID    armMmuSetDomain(UINT32  uiDomainAttr);
VOID    armMmuSetTTBase(LW_PGD_TRANSENTRY  *pgdEntry);
VOID    armMmuSetTTBase1(LW_PGD_TRANSENTRY  *pgdEntry);
VOID    armMmuInvalidateTLB(VOID);
VOID    armMmuInvalidateTLBMVA(PVOID pvVAddr);
VOID    armMmuSetProcessId(pid_t  pid);

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  �쳣��Ϣ��ȡ
*********************************************************************************************************/

addr_t  armGetAbtAddr(VOID);
VOID    armGetAbtType(PLW_VMM_ABORT  pabtInfo);
addr_t  armGetPreAddr(addr_t  ulRetLr);
VOID    armGetPreType(PLW_VMM_ABORT  pabtInfo);

#endif                                                                  /*  __ARMMMUCOMMON_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
