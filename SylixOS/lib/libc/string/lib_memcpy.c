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
** ��   ��   ��: lib_memcpy.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ��
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ���ֶ��뷽ʽ����
*********************************************************************************************************/
#define __LONGSIZE              sizeof(ULONG)
#define __LONGMASK              (__LONGSIZE - 1)
#define __TLOOP(s)              if (ulTemp) {       \
                                    __TLOOP1(s);    \
                                }
#define __TLOOP1(s)             do {                \
                                    s;              \
                                } while (--ulTemp)
/*********************************************************************************************************
** ��������: lib_memcpy
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
PVOID  lib_memcpy (PVOID  pvDest, CPVOID   pvSrc, size_t  stCount)
{
#ifdef __ARCH_MEMCPY
    return  (__ARCH_MEMCPY(pvDest, pvSrc, stCount));
    
#else
    REGISTER PUCHAR    pucDest;
    REGISTER PUCHAR    pucSrc;
             ULONG     ulTemp;
    
    pucDest = (PUCHAR)pvDest;
    pucSrc  = (PUCHAR)pvSrc;
    
    if (stCount == 0 || pucDest == pucSrc) {
        return  (pvDest);
    }
    
    if (pucDest < pucSrc) {
        /*
         *  ����ѭ�򿽱�
         */
        ulTemp = (ULONG)pucSrc;
        if ((ulTemp | (ULONG)pucDest) & __LONGMASK) {
            /*
             *  �����Ƕ��벿��
             */
            if (((ulTemp ^ (ULONG)pucDest) & __LONGMASK) || (stCount < __LONGSIZE)) {
                ulTemp = (ULONG)stCount;
            } else {
                ulTemp = (ULONG)(__LONGSIZE - (ulTemp & __LONGMASK));
            }
            stCount -= (UINT)ulTemp;
            __TLOOP1(*pucDest++ = *pucSrc++);
        }
        /*
         *  ���ֶ��뿽��
         */
        ulTemp = (ULONG)(stCount / __LONGSIZE);
        __TLOOP(*(ULONG *)pucDest = *(ULONG *)pucSrc; pucSrc += __LONGSIZE; pucDest += __LONGSIZE);
        ulTemp = (ULONG)(stCount & __LONGMASK);
        __TLOOP(*pucDest++ = *pucSrc++);
    
    } else {
        /*
         *  ����ѭ�򿽱�
         */
        pucSrc  += stCount;
        pucDest += stCount;
        
        ulTemp = (ULONG)pucSrc;
        if ((ulTemp | (ULONG)pucDest) & __LONGMASK) {
            /*
             *  �����Ƕ��벿��
             */
            if (((ulTemp ^ (ULONG)pucDest) & __LONGMASK) || (stCount <= __LONGSIZE)) {
                ulTemp = (ULONG)stCount;
            } else {
                ulTemp &= __LONGMASK;
            }
            stCount -= (UINT)ulTemp;
            __TLOOP1(*--pucDest = *--pucSrc);
        }
        
        ulTemp = (ULONG)(stCount / __LONGSIZE);
        __TLOOP(pucSrc -= __LONGSIZE; pucDest -= __LONGSIZE; *(ULONG *)pucDest = *(ULONG *)pucSrc);
        ulTemp = (ULONG)(stCount & __LONGMASK);
        __TLOOP(*--pucDest = *--pucSrc);
    }
    
    return  (pvDest);
#endif                                                                  /*  __ARCH_MEMCPY               */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
