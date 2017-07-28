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
** ��   ��   ��: _TimeCvt.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 07 �� 28 ��
**
** ��        ��: ʱ��任.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ѡ��, ���� 1 �ٶȿ�, ������ 292,471,208 ����, �������ʱ������. ������ʵ������, �����Ͻ��ľ���,
  �����ṩ�˸�Ϊ�ϸ�ķ��� 2 , ��������ʧ����Ч.
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: __timevalToTickDiff
** ��������: ��������ʱ���ֻ��, ��ת��Ϊ tick
** �䡡��  : ptvS, ptvE     ʱ��㿪ʼ�����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __timevalToTickDiff64
** ��������: ��������ʱ���ֻ��, ��ת��Ϊ tick
** �䡡��  : ptvS, ptvE     ʱ��㿪ʼ�����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __timespecToTickDiff
** ��������: ��������ʱ���ֻ��, ��ת��Ϊ tick
** �䡡��  : ptvS, ptvE     ʱ��㿪ʼ�����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __timespecToTickDiff64
** ��������: ��������ʱ���ֻ��, ��ת��Ϊ tick
** �䡡��  : ptvS, ptvE     ʱ��㿪ʼ�����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
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
