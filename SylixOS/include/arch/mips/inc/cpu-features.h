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
** ��   ��   ��: cpu-features.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 28 ��
**
** ��        ��: MIPS CPU ����.
*********************************************************************************************************/

#ifndef __ARCH_MIPSCPUFEATURES_H
#define __ARCH_MIPSCPUFEATURES_H

#define cpu_has_mmips                   LW_FALSE

#if defined(_MIPS_ARCH_MIPS3)
#define cpu_has_mips_r                  LW_FALSE
#else
#define cpu_has_mips_r                  LW_TRUE
#endif

#define cpu_has_mips_2_3_4_5_r          LW_TRUE

#if defined(_MIPS_ARCH_MIPS64R2) || \
    defined(_MIPS_ARCH_MIPS64R3) || \
    defined(_MIPS_ARCH_MIPS64R5) || \
    defined(_MIPS_ARCH_MIPS64R6) || \
    defined(_MIPS_ARCH_MIPS32R2) || \
    defined(_MIPS_ARCH_MIPS32R3) || \
    defined(_MIPS_ARCH_MIPS32R5) || \
    defined(_MIPS_ARCH_MIPS32R6)
#define cpu_has_mips_r2_r6              LW_TRUE
#else
#define cpu_has_mips_r2_r6              LW_FALSE
#endif

#if defined(_MIPS_ARCH_MIPS64R6) || \
    defined(_MIPS_ARCH_MIPS32R6)
#define cpu_has_mips_r6                 LW_TRUE
#else
#define cpu_has_mips_r6                 LW_FALSE
#endif

#if LW_CFG_CPU_WORD_LENGHT == 32
#define cpu_has_mips64                  LW_FALSE
#define cpu_has_mips_3_4_5              LW_FALSE
#define cpu_has_mips_3_4_5_64_r2_r6     LW_FALSE
#define cpu_has_mips_4_5_r              LW_FALSE
#define cpu_has_mips_4_5_64_r2_r6       LW_FALSE
#else
#define cpu_has_mips64                  LW_TRUE
#define cpu_has_mips_3_4_5              LW_TRUE
#define cpu_has_mips_3_4_5_64_r2_r6     LW_TRUE
#define cpu_has_mips_4_5_r              LW_TRUE
#define cpu_has_mips_4_5_64_r2_r6       LW_TRUE
#endif

#if LW_CFG_CPU_DSP_EN > 0
#define cpu_has_dsp                     LW_TRUE
#else
#define cpu_has_dsp                     LW_FALSE
#endif

#if LW_CFG_MIPS_HAS_MSA_INSTR > 0
#define cpu_has_msa                     LW_TRUE
#else
#define cpu_has_msa                     LW_FALSE
#endif

#endif                                                                  /*  __ARCH_MIPSCPUFEATURES_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
