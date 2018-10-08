/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: arm64_atomic.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 08 月 21 日
**
** 描        述: ARM64 体系构架 ATOMIC 接口.
*********************************************************************************************************/

#ifndef __ARCH_ARM64_ATOMIC_H
#define __ARCH_ARM64_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#define ATOMIC_OP_RETURN(op, c_op, asm_op)                                  \
static LW_INLINE INT  archAtomic##op (INT  i, atomic_t  *v)                 \
{                                                                           \
    ULONG   ulTemp;                                                         \
    INT     iResult;                                                        \
                                                                            \
    ARM_PREFETCH_W(&v->counter);                                            \
                                                                            \
    __asm__ __volatile__(                                                   \
        "1: ldxr            %w0, %2         \n"                             \
        "   " #asm_op "     %w0, %w0, %w3   \n"                             \
        "   stlxr           %w1, %w0, %2    \n"                             \
        "   cbnz            %w1, 1b         \n"                             \
        "   dmb ish"                                                        \
            : "=&r" (iResult), "=&r" (ulTemp), "+Q" (v->counter)            \
            : "Ir" (i)                                                      \
            : "memory");                                                    \
                                                                            \
    return  (iResult);                                                      \
}

ATOMIC_OP_RETURN(Add,  +=,  add)
ATOMIC_OP_RETURN(Sub,  -=,  sub)
ATOMIC_OP_RETURN(And,  &=,  and)
ATOMIC_OP_RETURN(Or,   |=,  orr)
ATOMIC_OP_RETURN(Xor,  ^=,  eor)

/*********************************************************************************************************
  On ARM64, ordinary assignment (str instruction) doesn't clear the local
  strex/ldrex monitor on some implementations. The reason we can use it for
  archAtomicSet() is the clrex or dummy strex done on every exception return.
*********************************************************************************************************/

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
    INT    iOldVal;
    INT    iTmp;

    ARM_PREFETCH_W(&v->counter);

    __asm__ __volatile__(
        "1: ldxr   %w[oldval], %[v]                  \n"
        "   eor    %w[tmp],    %w[oldval], %w[old]   \n"
        "   cbnz   %w[tmp],    2f                    \n"
        "   stxr   %w[tmp],    %w[new], %[v]         \n"
        "   cbnz   %w[tmp],    1b                    \n"
        "   dmb ish                                  \n"
        "   mov    %w[oldval], %w[old]               \n"
        "2:"
        : [tmp] "=&r" (iTmp),  [oldval] "=&r" (iOldVal),
          [v] "+Q" (v->counter)
        : [old] "Lr" (iOld), [new] "r" (iNew)
        :);

    return  (iOldVal);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulTmp;
    addr_t  ulOldVal;

    ARM_PREFETCH_W(p);

    __asm__ __volatile__(           
        "1: ldxr   %[oldval], %[v]                  \n"
        "   eor    %[tmp],    %[oldval], %[old]     \n"
        "   cbnz   %[tmp],    2f                    \n"
        "   stxr   %[tmp],    %[new], %[v]          \n"
        "   cbnz   %[tmp],    1b                    \n"
        "   dmb ish                                 \n"
        "   mov    %[oldval], %[old]                \n"
        "2:"
        : [tmp] "=&r" (ulTmp),  [oldval] "=&r" (ulOld),
          [v] "+Q" (*p)
        : [old] "Lr" (ulOld), [new] "r" (ulNew)
        :);

    return  (ulOldVal);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_ARM64_ATOMIC_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
