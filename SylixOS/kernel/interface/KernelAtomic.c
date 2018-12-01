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
** ��   ��   ��: KernelAtomic.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 09 �� 30 ��
**
** ��        ��: ϵͳ�ں�ԭ�Ӳ�����.

** BUG:
2013.03.30  ���� API_AtomicSwp ����.
2014.05.05  ��ԭ�Ӳ���������, Ϊ Qt4.8.6 ���ض�ϵͳ�ṩ����.
2018.11.30  ���� LLSC Nesting CPU bug ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  CPU ATOMIC BUG ̽��
*********************************************************************************************************/
#if (LW_CFG_CPU_ATOMIC_EN > 0) && defined(LW_CFG_CPU_ARCH_MIPS) && (LW_CFG_MIPS_NEST_LLSC_BUG > 0)
#define __LW_ATOMIC_INTREG(ireg)    INTREG  ireg
#define __LW_ATOMIC_INTDIS(ireg)    ireg = KN_INT_DISABLE()
#define __LW_ATOMIC_INTEN(ireg)     KN_INT_ENABLE(ireg)

#else                                                                   /*  LW_CFG_MIPS_NEST_LLSC_BUG   */
#define __LW_ATOMIC_INTREG(ireg)
#define __LW_ATOMIC_INTDIS(ireg)
#define __LW_ATOMIC_INTEN(ireg)
#endif                                                                  /*  !LW_CFG_MIPS_NEST_LLSC_BUG  */
/*********************************************************************************************************
** ��������: API_AtomicAdd
** ��������: ԭ�� + ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicAdd (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_ADD(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicSub
** ��������: ԭ�� - ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicSub (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_SUB(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicInc
** ��������: ԭ�� + 1����
** �䡡��  : patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicInc (atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_INC(patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicDec
** ��������: ԭ�� - 1����
** �䡡��  : patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicDec (atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_DEC(patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicAnd
** ��������: ԭ�� & ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicAnd (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_AND(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicNand
** ��������: ԭ�� &~ ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicNand (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_NAND(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicOr
** ��������: ԭ�� | ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicOr (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_OR(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicXor
** ��������: ԭ�� ^ ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicXor (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_XOR(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicSet
** ��������: ԭ�Ӹ�ֵ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_AtomicSet (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        __LW_ATOMIC_SET(iVal, patomic);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicGet
** ��������: ԭ�ӻ�ȡ����
** �䡡��  : patomic   ԭ�Ӳ�����
** �䡡��  : ԭ�Ӳ�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicGet (atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_GET(patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicSwp
** ��������: ԭ�ӽ�������
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : ֮ǰ����ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicSwp (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_SWP(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicCas
** ��������: ԭ�ӽ������� ()
** �䡡��  : patomic   ԭ�Ӳ�����
**           iOldVal   ��ֵ
**           iNewVal   ��ֵ
** �䡡��  : ��ֵ, �������ֵ�� iOldVal ��ͬ, ���ʾ���óɹ�.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicCas (atomic_t  *patomic, INT  iOldVal, INT  iNewVal)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_CAS(patomic, iOldVal, iNewVal);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
