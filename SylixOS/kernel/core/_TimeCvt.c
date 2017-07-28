/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: _TimeCvt.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 07 月 28 日
**
** 描        述: 时间变换.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  方法选择, 方法 1 速度快, 适用于 292,471,208 年内, 超过这个时间会出错. 哈哈其实够用了, 本着严谨的精神,
  这里提供了更为严格的方法 2 , 到宇宙消失都有效.
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: __timevalToTickDiff
** 功能描述: 计算两个时间点只差, 并转换为 tick
** 输　入  : ptvS, ptvE     时间点开始与结束
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __timevalToTickDiff (const struct timeval  *ptvS, const struct timeval  *ptvE)
{
    REGISTER ULONG    ulRes = LW_TIME_MILLION / LW_TICK_HZ;

#ifdef __SYLIXOS_TIMECVT_METHOD_2
    struct   timeval  tvS = *ptvS;
    struct   timeval  tvE = *ptvE;
    REGISTER INT64    i64S, i64E;
             
    tvE.tv_sec -= tvS.tv_sec;
    tvS.tv_sec  = 0;
    
    i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(&tvS, tv_usec, ulRes, INT64);
    i64E = LW_CONVERT_TO_TICK(&tvE, tv_usec, ulRes, INT64);

#else
    REGISTER INT64  i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(ptvS, tv_usec, ulRes, INT64);
    REGISTER INT64  i64E = LW_CONVERT_TO_TICK(ptvE, tv_usec, ulRes, INT64);
#endif

    return  ((ULONG)(i64E - i64S));
}
/*********************************************************************************************************
** 函数名称: __timevalToTickDiff64
** 功能描述: 计算两个时间点只差, 并转换为 tick
** 输　入  : ptvS, ptvE     时间点开始与结束
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT64  __timevalToTickDiff64 (const struct timeval  *ptvS, const struct timeval  *ptvE)
{
    REGISTER ULONG    ulRes = LW_TIME_MILLION / LW_TICK_HZ;

#ifdef __SYLIXOS_TIMECVT_METHOD_2
    struct   timeval  tvS = *ptvS;
    struct   timeval  tvE = *ptvE;
    REGISTER INT64    i64S, i64E;
             
    tvE.tv_sec -= tvS.tv_sec;
    tvS.tv_sec  = 0;
    
    i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(&tvS, tv_usec, ulRes, INT64);
    i64E = LW_CONVERT_TO_TICK(&tvE, tv_usec, ulRes, INT64);

#else
    REGISTER INT64  i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(ptvS, tv_usec, ulRes, INT64);
    REGISTER INT64  i64E = LW_CONVERT_TO_TICK(ptvE, tv_usec, ulRes, INT64);
#endif

    return  (i64E - i64S);
}
/*********************************************************************************************************
** 函数名称: __timespecToTickDiff
** 功能描述: 计算两个时间点只差, 并转换为 tick
** 输　入  : ptvS, ptvE     时间点开始与结束
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __timespecToTickDiff (const struct timespec  *ptvS, const struct timespec  *ptvE)
{
    REGISTER ULONG    ulRes = LW_TIME_BILLION / LW_TICK_HZ;

#ifdef __SYLIXOS_TIMECVT_METHOD_2
    struct   timespec tvS = *ptvS;
    struct   timespec tvE = *ptvE;
    REGISTER INT64    i64S, i64E;
             
    tvE.tv_sec -= tvS.tv_sec;
    tvS.tv_sec  = 0;
    
    i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(&tvS, tv_nsec, ulRes, INT64);
    i64E = LW_CONVERT_TO_TICK(&tvE, tv_nsec, ulRes, INT64);

#else
    REGISTER INT64  i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(ptvS, tv_nsec, ulRes, INT64);
    REGISTER INT64  i64E = LW_CONVERT_TO_TICK(ptvE, tv_nsec, ulRes, INT64);
#endif

    return  ((ULONG)(i64E - i64S));
}
/*********************************************************************************************************
** 函数名称: __timespecToTickDiff64
** 功能描述: 计算两个时间点只差, 并转换为 tick
** 输　入  : ptvS, ptvE     时间点开始与结束
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT64  __timespecToTickDiff64 (const struct timespec  *ptvS, const struct timespec  *ptvE)
{
    REGISTER ULONG    ulRes = LW_TIME_BILLION / LW_TICK_HZ;

#ifdef __SYLIXOS_TIMECVT_METHOD_2
    struct   timespec tvS = *ptvS;
    struct   timespec tvE = *ptvE;
    REGISTER INT64    i64S, i64E;
             
    tvE.tv_sec -= tvS.tv_sec;
    tvS.tv_sec  = 0;
    
    i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(&tvS, tv_nsec, ulRes, INT64);
    i64E = LW_CONVERT_TO_TICK(&tvE, tv_nsec, ulRes, INT64);

#else
    REGISTER INT64  i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(ptvS, tv_nsec, ulRes, INT64);
    REGISTER INT64  i64E = LW_CONVERT_TO_TICK(ptvE, tv_nsec, ulRes, INT64);
#endif

    return  (i64E - i64S);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
