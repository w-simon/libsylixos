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
** ��   ��   ��: elf_arch.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: ����ϵ�ṹ��ص� elf �ļ��ض�λ�ӿ�
*********************************************************************************************************/

#ifndef __ELF_ARCH_H
#define __ELF_ARCH_H

/*********************************************************************************************************
  ��ϵ�ṹ��ص��ض�λ�����ӿ�����
*********************************************************************************************************/

INT archElfRGetJmpBuffItemLen(PVOID  pmodule);                          /*  ��ȡ��ת�����С            */

#if !defined(LW_CFG_CPU_ARCH_C6X) && !defined(LW_CFG_CPU_ARCH_SPARC)
INT archElfRelocateRela(PVOID       pmodule,
                        Elf_Rela   *prela,
                        Elf_Addr    addrSymVal,
                        PCHAR       pcTargetSec,
                        PCHAR       pcBuffer,
                        size_t      stBuffLen);                         /*  RELA ���ض�λ               */
#else
INT archElfRelocateRela(PVOID       pmodule,
                        Elf_Rela   *prela,
                        Elf_Sym    *psym,
                        Elf_Addr    addrSymVal,
                        PCHAR       pcTargetSec,
                        PCHAR       pcBuffer,
                        size_t      stBuffLen);                         /*  RELA ���ض�λ               */
#endif

INT archElfRelocateRel(PVOID        pmodule,
                       Elf_Rel     *prel,
                       Elf_Addr     addrSymVal,
                       PCHAR        pcTargetSec,
                       PCHAR        pcBuffer,
                       size_t       stBuffLen);                         /*  REL ���ض�λ                */

#if defined(LW_CFG_CPU_ARCH_PPC) || defined(LW_CFG_CPU_ARCH_MIPS) || defined(LW_CFG_CPU_ARCH_C6X)
INT archElfGotInit(PVOID  pmodule);                                     /*  ��ʼ��GOT��                 */
#endif

#if defined(LW_CFG_CPU_ARCH_C6X)
INT archElfDSBTRemove(PVOID  pmodule);
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

#endif                                                                  /*  __ELF_ARCH_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
