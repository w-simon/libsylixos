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
** ��   ��   ��: config.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 08 �� 27 ��
**
** ��        ��: PowerPC ��ϵ���ܷǶ�������.
*********************************************************************************************************/

#ifndef __ARCH_PPC_UNALIGNED_CONFIG_H
#define __ARCH_PPC_UNALIGNED_CONFIG_H

#if LW_CFG_CPU_WORD_LENGHT == 32
#define CONFIG_PPC32
#else
#define CONFIG_PPC64
#ifndef __powerpc64__
#define __powerpc64__
#endif
#endif

#if LW_CFG_CPU_FPU_EN >0
#define CONFIG_PPC_FPU
#define CONFIG_SPE
#endif

#if LW_CFG_CPU_DSP_EN >0
#define CONFIG_ALTIVEC
#endif

#if LW_CFG_CPU_ENDIAN == 0
#define __LITTLE_ENDIAN__
#endif

/*********************************************************************************************************
  VSX (Vector Scalar Extension)
  https://en.wikipedia.org/wiki/AltiVec
  ���ڲ�֧�� VSX
*********************************************************************************************************/
#undef CONFIG_VSX

#define L1_CACHE_BYTES      LW_CFG_CPU_ARCH_CACHE_LINE

#endif                                                                  /* __ARCH_PPC_UNALIGNED_CONFIG_H*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
