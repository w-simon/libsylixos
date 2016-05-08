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
** ��   ��   ��: loader_vpstack.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2015 �� 11 �� 25 ��
**
** ��        ��: �������̶߳�ջ�ڴ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
/*********************************************************************************************************
** ��������: vprocStackAlloc
** ��������: ���� stack
** �䡡��  : ptcbNew       �½�����
**           ulOption      ���񴴽�ѡ��
**           stSize        ��ջ��С
** �䡡��  : ��ջ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  vprocStackAlloc (PLW_CLASS_TCB  ptcbNew, ULONG  ulOption, size_t  stSize)
{
    PVOID           pvRet;

#if (LW_CFG_VMM_EN > 0) && !defined(LW_CFG_CPU_ARCH_PPC)
    PLW_CLASS_TCB   ptcbCur;
    LW_LD_VPROC    *pvproc;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (ptcbNew->TCB_iStkLocation == LW_TCB_STK_NONE) {                 /*  ��û��ȷ����ջ��λ��        */
        pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
        if ((pvproc && !(ulOption & LW_OPTION_OBJECT_GLOBAL)) ||
            (ulOption & LW_OPTION_THREAD_STK_MAIN)) {
            ptcbNew->TCB_iStkLocation = LW_TCB_STK_VMM;                 /*  ʹ�� VMM ��ջ               */

        } else {
            ptcbNew->TCB_iStkLocation = LW_TCB_STK_HEAP;                /*  ʹ��ϵͳ��ջ                */
        }
    }

    if (ptcbNew->TCB_iStkLocation == LW_TCB_STK_VMM) {
        pvRet = API_VmmMalloc(stSize);

    } else
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    {
        ptcbNew->TCB_iStkLocation = LW_TCB_STK_HEAP;
        pvRet = __KHEAP_ALLOC(stSize);
    }
    
    return  (pvRet);
}
/*********************************************************************************************************
** ��������: vprocStackFree
** ��������: �ͷ� stack
** �䡡��  : ptcbDel       ��ɾ��������
**           pvStack       �̶߳�ջ
**           bImmed        ��������
** �䡡��  : ��ջ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  vprocStackFree (PLW_CLASS_TCB  ptcbDel, PVOID  pvStack, BOOL  bImmed)
{
#if (LW_CFG_VMM_EN > 0) && !defined(LW_CFG_CPU_ARCH_PPC)
    LW_LD_VPROC     *pvproc;
    BOOL             bFree = LW_TRUE;

    switch (ptcbDel->TCB_iStkLocation) {
    
    case LW_TCB_STK_VMM:
        pvproc = __LW_VP_GET_TCB_PROC(ptcbDel);
        if (pvproc && (pvproc->VP_ulMainThread == ptcbDel->TCB_ulId)) { /*  ��������߳�                */
            if (bImmed) {
#if LW_CFG_COROUTINE_EN > 0
                if (pvproc->VP_pvMainStack == pvStack) {
                    pvproc->VP_pvMainStack =  LW_NULL;                  /*  ���̶߳�ջɾ��              */
                }
#else
                pvproc->VP_pvMainStack = LW_NULL;
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
            } else {
                bFree = LW_FALSE;                                       /*  �ӳ�ɾ�����̶߳�ջ          */
            }
        }
        if (bFree) {
            API_VmmFree(pvStack);
        }
        break;
        
    case LW_TCB_STK_HEAP:
        __KHEAP_FREE(pvStack);
        break;
        
    default:
        _BugHandle(LW_TRUE, LW_FALSE, "unknown stack property!\r\n");
        break;
    }
#else
    __KHEAP_FREE(pvStack);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
