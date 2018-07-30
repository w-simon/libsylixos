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
** ��   ��   ��: x86_atomic.h
**
** ��   ��   ��: Jiao.Jinxing (������)
**
** �ļ���������: 2018 �� 07 �� 27 ��
**
** ��        ��: x86 ��ϵ���� ATOMIC �ӿ�.
*********************************************************************************************************/

#ifndef __ARCH_X86_ATOMIC_H
#define __ARCH_X86_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

static LW_INLINE VOID  archAtomicSet (INT  i, atomic_t  *v)
{
    __asm__ __volatile__("xchgl  %1, %0"
                        : "+m" (v->counter)
                        : "r" (i)
                        : "memory");
}

static LW_INLINE INT  archAtomicGet (atomic_t  *v)
{
    return  (LW_ACCESS_ONCE(INT, v->counter));
}

INT  archAtomicAdd(INT  i, atomic_t  *v);
INT  archAtomicSub(INT  i, atomic_t  *v);
INT  archAtomicAnd(INT  i, atomic_t  *v);
INT  archAtomicOr (INT  i, atomic_t  *v);
INT  archAtomicXor(INT  i, atomic_t  *v);

/*********************************************************************************************************
  atomic cas op
*********************************************************************************************************/

static LW_INLINE INT  archAtomicCas (atomic_t  *v, INT  iOld, INT  iNew)
{
    INT  iOldValue;

    __asm__ __volatile__("lock; cmpxchgl  %2, %1"
                        : "=a" (iOldValue), "+m" (v->counter)
                        : "r" (iNew), "0" (iOld)
                        : "memory");
    return  (iOldValue);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulOldValue;

#if LW_CFG_CPU_WORD_LENGHT == 64
    __asm__ __volatile__("lock; cmpxchgq  %2, %1"
                        : "=a" (ulOldValue), "+m" (*p)
                        : "r" (ulNew), "0" (ulOld)
                        : "memory");

#else                                                                   /*  LW_CFG_CPU_WORD_LENGHT 64   */
    __asm__ __volatile__("lock; cmpxchgl  %2, %1"
                        : "=a" (ulOldValue), "+m" (*p)
                        : "r" (ulNew), "0" (ulOld)
                        : "memory");
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT 32   */

    return  (ulOldValue);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_X86_ATOMIC_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
