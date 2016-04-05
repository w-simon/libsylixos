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
** ��   ��   ��: arch_inc.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 05 �� 04 ��
**
** ��        ��: ��ϵ���ͷ�ļ�.
*********************************************************************************************************/

#ifndef __ARCH_INC_H
#define __ARCH_INC_H

#include "config/cpu/cpu_cfg.h"

#if (defined LW_CFG_CPU_ARCH_ARM)
#include "arm/arch_types.h"
#include "arm/arch_compiler.h"
#include "arm/arch_float.h"
#include "arm/arch_limits.h"
#include "arm/arch_regs.h"
#include "arm/arch_mmu.h"

#elif (defined LW_CFG_CPU_ARCH_X86)
#include "x86/arch_types.h"
#include "x86/arch_compiler.h"
#include "x86/arch_float.h"
#include "x86/arch_limits.h"
#include "x86/arch_regs.h"
#include "x86/arch_mmu.h"

#elif (defined LW_CFG_CPU_ARCH_MIPS)
#include "mips/arch_types.h"
#include "mips/arch_def.h"
#include "mips/arch_compiler.h"
#include "mips/arch_float.h"
#include "mips/arch_limits.h"
#include "mips/arch_regs.h"
#include "mips/arch_mmu.h"

#elif (defined LW_CFG_CPU_ARCH_PPC)
#include "ppc/arch_types.h"
#include "ppc/arch_def.h"
#include "ppc/arch_compiler.h"
#include "ppc/arch_float.h"
#include "ppc/arch_limits.h"
#include "ppc/arch_regs.h"
#include "ppc/arch_mmu.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_INC_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
