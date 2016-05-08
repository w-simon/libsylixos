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
** ��   ��   ��: lib_memset.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ��

** BUG:
2011.06.22  �� iCount С�� 0 ʱ, ������.
2013.03.29  memset iC ��ת��Ϊ uchar ����.
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ���ֶ��뷽ʽ����
*********************************************************************************************************/
#define __LONGSIZE              sizeof(ULONG)
#define __LONGMASK              (__LONGSIZE - 1)
#define __TLOOP(s)              if (iTemp) {        \
                                    __TLOOP1(s);    \
                                }
#define __TLOOP1(s)             do {                \
                                    s;              \
                                } while (--iTemp)
/*********************************************************************************************************
** ��������: lib_memset
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
PVOID  lib_memset (PVOID  pvDest, INT  iC, size_t  stCount)
{
#ifdef __ARCH_MEMSET
    return  (__ARCH_MEMSET(pvDest, pvSrc, stCount));
    
#else
    REGISTER INT       i;
    REGISTER INT       iTemp;
    REGISTER PUCHAR    pucDest = (PUCHAR)pvDest;
             UCHAR     ucC     = (UCHAR)iC;
             ULONG     ulFill  = (ULONG)ucC;
             
    if (stCount == 0) {
        return  (pvDest);
    }
             
    for (i = 1; i < (__LONGSIZE / sizeof(UCHAR)); i++) {                /*  ���� ulong ����ĸ�ֵ����   */
        ulFill = (ulFill << 8) + ucC;
    }

    if ((ULONG)pucDest & __LONGMASK) {                                  /*  ����ǰ�˷Ƕ��벿��          */
        if (stCount < __LONGSIZE) {
            iTemp = stCount;
        } else {
            iTemp = (INT)(__LONGSIZE - ((ULONG)pucDest & __LONGMASK));
        }
        stCount -= iTemp;
        __TLOOP1(*pucDest++ = ucC);
    }
    
    iTemp = (INT)(stCount / __LONGSIZE);
    __TLOOP(*(ULONG *)pucDest = ulFill; pucDest += __LONGSIZE);
    iTemp = (INT)(stCount & __LONGMASK);
    __TLOOP(*pucDest++ = ucC);
    
    return  (pvDest);
#endif                                                                  /*  __ARCH_MEMSET               */
}
/*********************************************************************************************************
** ��������: lib_bzero
** ��������: ���ֽ��ַ���s��ǰn���ֽ�Ϊ�㡣
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : 
*********************************************************************************************************/
VOID    lib_bzero (PVOID   pvStr, size_t  stCount)
{
#ifdef __ARCH_MEMSET
    __ARCH_MEMSET(pvStr, 0, stCount);

#else
    REGISTER INT       iTemp;
    REGISTER PUCHAR    pucDest = (PUCHAR)pvStr;
    
    if ((ULONG)pucDest & __LONGMASK) {                                  /*  ����ǰ�˷Ƕ��벿��          */
        if (stCount < __LONGSIZE) {
            iTemp = stCount;
        } else {
            iTemp = (INT)(__LONGSIZE - ((ULONG)pucDest & __LONGMASK));
        }
        stCount -= iTemp;
        __TLOOP1(*pucDest++ = 0);
    }
    
    iTemp = (INT)(stCount / __LONGSIZE);
    __TLOOP(*(ULONG *)pucDest = 0ul; pucDest += __LONGSIZE);
    iTemp = (INT)(stCount & __LONGMASK);
    __TLOOP(*pucDest++ = 0);
#endif                                                                  /*  __ARCH_MEMSET               */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
