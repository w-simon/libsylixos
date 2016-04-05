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
** ��   ��   ��: ppcBacktrace.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ���ܶ�ջ���� (��Դ�� glibc).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "ppcBacktrace.h"
/*********************************************************************************************************
   This is the stack layout we see with every stack frame.
   Note that every routine is required by the ABI to lay out the stack
   like this.

            +----------------+        +-----------------+
    %r1  -> | %r1 last frame--------> | %r1 last frame--->...  --> NULL
            |                |        |                 |
            | (unused)       |        | return address  |
            +----------------+        +-----------------+
*********************************************************************************************************/
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
    struct layout  *current;
    int             count;

    /* Force gcc to spill LR.  */
    asm volatile ("" : "=l"(current));

    /* Get the address on top-of-stack.  */
    asm volatile ("lwz %0 , 0(1)" : "=r"(current));
    current = BOUNDED_1(current);

    for (count = 0;
         current != NULL && count < size;
         current = BOUNDED_1(current->next), count++) {
        array[count] = current->return_address;
    }

    /* It's possible the second-last stack frame can't return
       (that is, it's __libc_start_main), in which case
       the CRT startup code will have set its LR to 'NULL'.  */
    if (count > 0 && array[count - 1] == NULL) {
        count--;
    }

    return  (count);
}
#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
