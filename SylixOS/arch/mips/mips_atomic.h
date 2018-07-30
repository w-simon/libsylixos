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
** ��   ��   ��: mips_atomic.h
**
** ��   ��   ��: Jiao.Jinxing (������)
**
** �ļ���������: 2018 �� 07 �� 27 ��
**
** ��        ��: MIPS ��ϵ���� ATOMIC �ӿ�.
*********************************************************************************************************/

#ifndef __ARCH_MIPS_ATOMIC_H
#define __ARCH_MIPS_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#define ATOMIC_OP_RETURN(op, c_op, asm_op)                                  \
static LW_INLINE INT  archAtomic##op (INT  i, atomic_t  *v)                 \
{                                                                           \
    INT  iTemp;                                                             \
    INT  iResult;                                                           \
                                                                            \
    do {                                                                    \
        __asm__ __volatile__(                                               \
            "   ll      %1, %2                      \n"                     \
            "   " #asm_op " %0, %1, %3              \n"                     \
            "   sc      %0, %2                      \n"                     \
            : "=&r" (iResult), "=&r" (iTemp),                               \
              "+R" (v->counter)                                             \
            : "Ir" (i));                                                    \
    } while (!iResult);                                                     \
                                                                            \
    iResult = iTemp;                                                        \
    iResult c_op i;                                                         \
                                                                            \
    return  (iResult);                                                      \
}

ATOMIC_OP_RETURN(Add,  +=,  addu)
ATOMIC_OP_RETURN(Sub,  -=,  subu)
ATOMIC_OP_RETURN(And,  &=,  and)
ATOMIC_OP_RETURN(Or,   |=,  or)
ATOMIC_OP_RETURN(Xor,  ^=,  xor)

static LW_INLINE VOID  archAtomicSet (INT  i, atomic_t  *v)
{
    LW_ACCESS_ONCE(INT, v->counter) = i;
}

static LW_INLINE INT  archAtomicGet (atomic_t  *v)
{
    return  (LW_ACCESS_ONCE(INT, v->counter));
}

/*********************************************************************************************************
  atomic cas op
*********************************************************************************************************/

static LW_INLINE INT  archAtomicCas (atomic_t  *v, INT  iOld, INT  iNew)
{
    INT  iResult;

    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noat                \n"
        "1: ll      %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   move    $1, %z4             \n"
        "   sc      $1, %1              \n"
        "   beqz    $1, 1b              \n"
        "   .set    pop                 \n"
        "2:                             \n"
        : "=&r" (iResult), "=R" (v->counter)
        : "R" (v->counter), "Jr" (iOld), "Jr" (iNew)
        : "memory");

    return  (iResult);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulResult;

#if LW_CFG_CPU_WORD_LENGHT == 64
    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noat                \n"
        "1: lld     %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   move    $1, %z4             \n"
        "   scd     $1, %1              \n"
        "   beqz    $1, 1b              \n"
        "   .set    pop                 \n"
        "2:                             \n"
        : "=&r" (ulResult), "=R" (*p)
        : "R" (*p), "Jr" (ulOld), "Jr" (ulNew)
        : "memory");

#else                                                                   /*  LW_CFG_CPU_WORD_LENGHT 64   */
    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noat                \n"
        "1: ll      %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   move    $1, %z4             \n"
        "   sc      $1, %1              \n"
        "   beqz    $1, 1b              \n"
        "   .set    pop                 \n"
        "2:                             \n"
        : "=&r" (ulResult), "=R" (*p)
        : "R" (*p), "Jr" (ulOld), "Jr" (ulNew)
        : "memory");
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT 32   */

    return  (ulResult);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_MIPS_ATOMIC_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
