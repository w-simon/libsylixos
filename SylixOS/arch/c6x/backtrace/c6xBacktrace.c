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
** ��   ��   ��: c6xBacktrace.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 03 �� 17 ��
**
** ��        ��: c6x ��ϵ���ܶ�ջ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "c6xBacktrace.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "unistd.h"
#include "loader/include/loader_lib.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: getEndStack
** ��������: ��ö�ջ������ַ
** �䡡��  : NONE
** �䡡��  : ��ջ������ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  getEndStack (VOID)
{
    PLW_CLASS_TCB  ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  ((PVOID)ptcbCur->TCB_pstkStackTop);
}
/*********************************************************************************************************
** ��������: backtrace
** ��������: ��õ�ǰ�������ջ
** �䡡��  : array     ��ȡ����
**           size      �����С
** �䡡��  : ��ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  backtrace (void **array, int size)
{
    INT                 iCnt     = 0;
    ULONG              *pulEnd   = getEndStack();
    ULONG              *pulBegin = (ULONG *)archStackPointerGet();
    ULONG               ulValue;
#if LW_CFG_MODULELOADER_EN > 0
    INT                 i;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    BOOL                bStart;
    LW_LD_EXEC_SEGMENT *psegment;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    extern CHAR         __text[];
    extern CHAR         __etext[];

#if LW_CFG_MODULELOADER_EN > 0
    if (getpid() == 0) {
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        while (pulBegin <= pulEnd) {
            ulValue = *pulBegin++;
            if ((ulValue >= (ULONG)__text) && (ulValue < (ULONG)__etext)) {
                array[iCnt++] = (VOID *)ulValue;
            }
        }

#if LW_CFG_MODULELOADER_EN > 0
    } else {
        pvproc = vprocGetCur();

        LW_VP_LOCK(pvproc);

        while (pulBegin <= pulEnd) {
__next:
            ulValue = *pulBegin++;

            if ((ulValue >= (ULONG)__text) && (ulValue < (ULONG)__etext)) {
                array[iCnt++] = (VOID *)ulValue;
                continue;
            }

            for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
                 pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
                 pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

                pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
                psegment = pmodTemp->EMOD_psegmentArry;
                for (i = 0; i < pmodTemp->EMOD_ulSegCount; i++, psegment++) {
                    if (psegment->ESEG_stLen == 0) {
                        continue;
                    }
                    if (psegment->ESEG_bCanExec) {
                        if ((ulValue >= psegment->ESEG_ulAddr) &&
                            (ulValue < (psegment->ESEG_ulAddr + psegment->ESEG_stLen))) {
                            array[iCnt++] = (VOID *)ulValue;
                            goto    __next;
                        }
                    }
                }
            }
        }

        LW_VP_UNLOCK(pvproc);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    return  (iCnt);
}

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
