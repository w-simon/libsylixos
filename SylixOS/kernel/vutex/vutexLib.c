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
** ��   ��   ��: vutexLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2020 �� 12 �� 26 ��
**
** ��        ��: �ȴ��������ڲ�ʵ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define LW_VUTEX_ALIGN_LOG      3
#define LW_VUTEX_HASH_SIZE      1024
#define LW_VUTEX_HASH_MASK      0x3ff
#define LW_VUTEX_HASH_INDEX(a)  (((a) >> LW_VUTEX_ALIGN_LOG) & LW_VUTEX_HASH_MASK)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER   _k_plineVutexHashHeader[LW_VUTEX_HASH_SIZE];
/*********************************************************************************************************
** ��������: _VutexWaitQueue
** ��������: ����ǰ�������ȴ�����
** �䡡��  : ptcbCur   ��ǰ����
**           phyaddr   �ȴ��������ַ
**           iExpect   ������ֵ
**           iFlags    ����ѡ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _VutexWaitQueue (PLW_CLASS_TCB  ptcbCur, phys_addr_t  phyaddr, INT32  iExpect, INT  iFlags)
{
    REGISTER PLW_CLASS_PCB         ppcb;
             LW_LIST_LINE_HEADER  *pplineHeader;
             PLW_VUTEX_CONTEXT     pvutex = &ptcbCur->TCB_vutex;

    pvutex->VUTEX_phyaddrVutex = phyaddr;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ���������ɾ��            */

    pvutex->VUTEX_iFlags       = iFlags;
    pvutex->VUTEX_iVutexExpect = iExpect;
    pvutex->VUTEX_uiVutexHash  = LW_VUTEX_HASH_INDEX(phyaddr);
    pplineHeader = &_k_plineVutexHashHeader[pvutex->VUTEX_uiVutexHash];
    _List_Line_Add_Ahead(&pvutex->VUTEX_lineVutex, pplineHeader);       /*  ����ȴ�����                */

    if (ptcbCur->TCB_ulDelay) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  ����ȴ�ɨ����              */
    }
}
/*********************************************************************************************************
** ��������: _VutexWaitQueue
** ��������: ���ݵ�ַ���������ݻ���Ŀ������
** �䡡��  : ptcbCur   ��ǰ����
**           phyaddr   �����ַ
**           iValue    д���ֵ
**           iFlags    ����ѡ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _VutexWakeQueue (PLW_CLASS_TCB  ptcbCur, phys_addr_t  phyaddr, INT32  iValue, INT  iFlags)
{
    UINT32                uiHash = LW_VUTEX_HASH_INDEX(phyaddr);
    BOOL                  bMatch, bGlobal = iFlags & LW_OPTION_VUTEX_GLOBAL;
    PLW_CLASS_TCB         ptcb;
    PLW_CLASS_PCB         ppcb;
    PLW_VUTEX_CONTEXT     pvutex;
    LW_LIST_LINE_HEADER  *pplineHeader;
    PLW_LIST_LINE         plineTemp, plineNext;

    pplineHeader = &_k_plineVutexHashHeader[uiHash];
    plineTemp    = *pplineHeader;

    while (plineTemp) {
        plineNext = _list_line_get_next(plineTemp);

        pvutex = _LIST_ENTRY(plineTemp, LW_VUTEX_CONTEXT, VUTEX_lineVutex);
        bMatch = LW_FALSE;

        if (bGlobal && (pvutex->VUTEX_iFlags & LW_OPTION_VUTEX_GLOBAL)) {
            if ((pvutex->VUTEX_phyaddrVutex == phyaddr) &&
                (pvutex->VUTEX_iVutexExpect == iValue || iValue == __ARCH_INT_MAX)) {
                ptcb   = _LIST_ENTRY(pvutex, LW_CLASS_TCB, TCB_vutex);
                bMatch = LW_TRUE;                                       /*  ȫ��ƥ��                    */
            }

        } else if (!(pvutex->VUTEX_iFlags & LW_OPTION_VUTEX_GLOBAL)) {
            if ((pvutex->VUTEX_phyaddrVutex == phyaddr) &&
                (pvutex->VUTEX_iVutexExpect == iValue || iValue == __ARCH_INT_MAX)) {
                ptcb = _LIST_ENTRY(pvutex, LW_CLASS_TCB, TCB_vutex);
                if (ptcbCur->TCB_pvVProcessContext == ptcb->TCB_pvVProcessContext) {
                    bMatch = LW_TRUE;                                   /*  ������ƥ��                  */
                }
            }
        }

        if (bMatch) {
            _List_Line_Del(&pvutex->VUTEX_lineVutex, pplineHeader);
            if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
                __DEL_FROM_WAKEUP_LINE(ptcb);                           /*  �˳��ȴ�����                */
                ptcb->TCB_ulDelay = 0ul;
            }

            if (ptcb->TCB_ucWaitTimeout) {
                ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;           /*  �����ʱλ                  */
            } else {
                ptcb->TCB_usStatus = (UINT16)(ptcb->TCB_usStatus & ~LW_THREAD_STATUS_VUTEX);
                if (__LW_THREAD_IS_READY(ptcb)) {                       /*  �Ƿ����                    */
                    ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;     /*  ���ȼ��ʽ                */
                    ppcb = _GetPcb(ptcb);
                    __ADD_TO_READY_RING(ptcb, ppcb);                    /*  ���뵽������ȼ�������      */
                }
            }
        }

        plineTemp = plineNext;
    }
}
/*********************************************************************************************************
** ��������: _VutexInitCtx
** ��������: ��ʼ��������ƿ� vutex ������
** �䡡��  : ptcb      ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _VutexInitCtx (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT  pvutex = &ptcb->TCB_vutex;

    pvutex->VUTEX_phyaddrVutex = LW_PHY_ADDR_INVALID;
    _LIST_LINE_INIT_IN_CODE(pvutex->VUTEX_lineVutex);
}
/*********************************************************************************************************
** ��������: _VutexUnQueue
** ��������: ��Ŀ�������˳��ȴ�����
** �䡡��  : ptcb      Ŀ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _VutexUnQueue (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT     pvutex = &ptcb->TCB_vutex;
    LW_LIST_LINE_HEADER  *pplineHeader;

    pplineHeader = &_k_plineVutexHashHeader[pvutex->VUTEX_uiVutexHash];
    _List_Line_Del(&pvutex->VUTEX_lineVutex, pplineHeader);

    pvutex->VUTEX_uiVutexHash  = 0;
    pvutex->VUTEX_phyaddrVutex = LW_PHY_ADDR_INVALID;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
