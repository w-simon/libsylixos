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
** 文   件   名: x86LocalApic.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 IOAPIC 相关头文件.
*********************************************************************************************************/

#ifndef __ARCH_X86LOCALAPIC_H
#define __ARCH_X86LOCALAPIC_H

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*********************************************************************************************************
  Local APIC base address
*********************************************************************************************************/

#define LOCAL_APIC_BASE         0xfee00000

/*********************************************************************************************************
  函数声明
*********************************************************************************************************/

INT     x86LocalApicInit(VOID);
INT     x86LocalApicSecondaryInit(VOID);

VOID    x86LocalApicEoi(VOID);
UINT8   x86LocalApicId(VOID);

VOID    x86LocalApicTimerInitAsTick(VOID);

VOID    x86LocalApicIrqEnable(UINT8  ucVector);
VOID    x86LocalApicIrqDisable(UINT8  ucVector);

INT     x86LocalApicSecondaryStart(UINT8  ucLocalApicId, addr_t  ulEntryAddr);
VOID    x86LocalApicSendIPI(UINT8  ucLocalApicId,  UINT8  ucX86Vector);

VOID    x86LocalApicTimerHighResolution(struct timespec  *ptv);

#endif                                                                  /*  __ARCH_X86LOCALAPIC_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
