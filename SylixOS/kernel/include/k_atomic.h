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
** ��   ��   ��: k_atomic.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 09 �� 30 ��
**
** ��        ��: ϵͳ�ں�ԭ�Ӳ���.
*********************************************************************************************************/

#ifndef __K_ATOMIC_H
#define __K_ATOMIC_H

/*********************************************************************************************************
  ����ϵͳ����, ѡ��������
*********************************************************************************************************/

#define __LW_ATOMIC_LOCK(iregInterLevel)    { LW_SPIN_LOCK_RAW(&_K_slAtomic, &iregInterLevel); }
#define __LW_ATOMIC_UNLOCK(iregInterLevel)  { LW_SPIN_UNLOCK_RAW(&_K_slAtomic, iregInterLevel); }

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

static LW_INLINE INT  __LW_ATOMIC_ADD (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    patomic->counter += iVal;
    iRet = patomic->counter;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_SUB (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    patomic->counter -= iVal;
    iRet = patomic->counter;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_INC (atomic_t  *patomic)
{
    return  (__LW_ATOMIC_ADD(1, patomic));
}

static LW_INLINE INT  __LW_ATOMIC_DEC (atomic_t  *patomic)
{
    return  (__LW_ATOMIC_SUB(1, patomic));
}

static LW_INLINE INT  __LW_ATOMIC_AND (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter & iVal;
    patomic->counter = iRet;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_NAND (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter & iVal;
    patomic->counter = ~iRet;
    iRet = patomic->counter;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_OR (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter | iVal;
    patomic->counter = iRet;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE INT  __LW_ATOMIC_XOR (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter ^ iVal;
    patomic->counter = iRet;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

static LW_INLINE VOID  __LW_ATOMIC_SET (INT  iVal, atomic_t  *patomic)
{
    INTREG  iregInterLevel;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    patomic->counter = iVal;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
}

static LW_INLINE INT  __LW_ATOMIC_GET (atomic_t  *patomic)
{
    return  (patomic->counter);
}

static LW_INLINE INT  __LW_ATOMIC_SWP (INT  iVal, atomic_t  *patomic)
{
             INTREG  iregInterLevel;
    REGISTER INT     iRet;
    
    __LW_ATOMIC_LOCK(iregInterLevel);
    iRet = patomic->counter;
    patomic->counter = iVal;
    __LW_ATOMIC_UNLOCK(iregInterLevel);
    
    return  (iRet);
}

#endif                                                                  /*  __K_ATOMIC_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
