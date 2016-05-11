/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: ppcExcE500.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 04 ��
**
** ��        ��: PowerPC E500 ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "ppcSpr.h"
#include "ppcSprE500.h"
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/mm/mmu/common/ppcMmu.h"
#endif
#include "arch/ppc/arch_e500.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: archE500DataStorageExceptionHandle
** ��������: ���ݴ洢�쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500DataStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcE500GetDEAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

    {
        /*
         * TODO
         */
    }

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archE500InstructionStorageExceptionHandle
** ��������: ָ������쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500InstructionStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ulRetAddr;
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

    {
        /*
         * TODO
         */
    }

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archE500AlignmentExceptionHandle
** ��������: �Ƕ����쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500AlignmentExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcE500GetDEAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BUS;

    LW_TCB_GET_CUR(ptcbCur);

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
