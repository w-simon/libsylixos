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
** 文   件   名: KernelAtomic.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 09 月 30 日
**
** 描        述: 系统内核原子操作库.

** BUG:
2013.03.30  加入 API_AtomicSwp 操作.
2014.05.05  将原子操作锁引出, 为 Qt4.8.6 等特定系统提供服务.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_AtomicAdd
** 功能描述: 原子 + 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicAdd (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_ADD(iVal, patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicSub
** 功能描述: 原子 - 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicSub (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_SUB(iVal, patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicInc
** 功能描述: 原子 + 1操作
** 输　入  : patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicInc (atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_INC(patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicDec
** 功能描述: 原子 - 1操作
** 输　入  : patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicDec (atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_DEC(patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicAnd
** 功能描述: 原子 & 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicAnd (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_AND(iVal, patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicNand
** 功能描述: 原子 &~ 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicNand (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_NAND(iVal, patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicOr
** 功能描述: 原子 | 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicOr (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_OR(iVal, patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicXor
** 功能描述: 原子 ^ 操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 操作结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicXor (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_XOR(iVal, patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicSet
** 功能描述: 原子赋值操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_AtomicSet (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        __LW_ATOMIC_SET(iVal, patomic);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicGet
** 功能描述: 原子获取操作
** 输　入  : patomic   原子操作数
** 输　出  : 原子操作数
** 全局变量: 
** 调用模块: 
                                           API 函数
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
** 函数名称: API_AtomicSwp
** 功能描述: 原子交换操作
** 输　入  : iVal      操作参数
**           patomic   原子操作数
** 输　出  : 之前的数值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicSwp (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_SWP(iVal, patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_AtomicCas
** 功能描述: 原子交换操作 ()
** 输　入  : patomic   原子操作数
**           iOldVal   旧值
**           iNewVal   新值
** 输　出  : 旧值, 如果返回值与 iOldVal 相同, 则表示设置成功.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_AtomicCas (atomic_t  *patomic, INT  iOldVal, INT  iNewVal)
{
    if (patomic) {
        return  (__LW_ATOMIC_CAS(patomic, iOldVal, iNewVal));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
