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
** ��   ��   ��: archprob.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 06 �� 08 ��
**
** ��        ��: ƽ̨����̽��.
*********************************************************************************************************/

#ifndef __ARCHPROB_H
#define __ARCHPROB_H

/*********************************************************************************************************
  ���� MIPS �ܹ��Ǵ�/С�˴洢
*********************************************************************************************************/

#if defined(MIPSEB) || defined(__MIPSEB__)
#undef __SYLIXOS_MIPS_BYTE_ORDER
#define __SYLIXOS_MIPS_BYTE_ORDER   _BIG_ENDIAN

#elif defined(MIPSEL) || defined(__MIPSEL__)
#undef __SYLIXOS_MIPS_BYTE_ORDER
#define __SYLIXOS_MIPS_BYTE_ORDER   _LITTLE_ENDIAN

#else
#warning "One of MIPSEL or MIPSEB must be defined."
#endif

/*********************************************************************************************************
  MIPS32 / MIPS64
*********************************************************************************************************/

#define __SYLIXOS_CPU_MIPS_32BIT    ((__SYLIXOS_CPU == __SYLIXOS_MIPS32)  || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI2)  || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI32) || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI32R2))

#define __SYLIXOS_CPU_MIPS_64BIT    ((__SYLIXOS_CPU == __SYLIXOS_MIPS64)  || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI3)  || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI64) || \
                                     (__SYLIXOS_CPU == __SYLIXOS_MIPSI64R2))

#endif                                                                  /*  __ARCHPROB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
