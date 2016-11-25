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
** ��   ��   ��: mipsBranch.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 25 ��
**
** ��        ��: MIPS ��֧Ԥ��.
*********************************************************************************************************/
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 1998, 2001 by Ralf Baechle
 */
#ifndef _ASM_BRANCH_H
#define _ASM_BRANCH_H

static inline int delay_slot(ARCH_REG_CTX *regs)
{
    return regs->REG_uiCP0Cause & M_CauseBD;
}

static inline void clear_delay_slot(ARCH_REG_CTX *regs)
{
    regs->REG_uiCP0Cause &= ~M_CauseBD;
}

static inline void set_delay_slot(ARCH_REG_CTX *regs)
{
    regs->REG_uiCP0Cause |= M_CauseBD;
}

static inline unsigned long exception_epc(ARCH_REG_CTX *regs)
{
    if (!delay_slot(regs))
        return regs->REG_uiCP0EPC;

    return regs->REG_uiCP0EPC + 4;
}

extern int __compute_return_epc(ARCH_REG_CTX *regs);

static inline int compute_return_epc(ARCH_REG_CTX *regs)
{
    if (!delay_slot(regs)) {
        regs->REG_uiCP0EPC += 4;
        return 0;
    }

    return __compute_return_epc(regs);
}

#endif                                                                  /*  _ASM_BRANCH_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
