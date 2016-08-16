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
** ��   ��   ��: x86IoApic.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 29 ��
**
** ��        ��: x86 ��ϵ���� IOAPIC ���ͷ�ļ�.
*********************************************************************************************************/

#ifndef __ARCH_X86IOAPIC_H
#define __ARCH_X86IOAPIC_H

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
/*********************************************************************************************************
  IO APIC base address
*********************************************************************************************************/

#define X86_IOAPIC_BASE                         0xfec00000

/*********************************************************************************************************
  Redirection table entry bits: upper 32 bit
*********************************************************************************************************/

#define IOAPIC_DESTINATION                      0xff000000

/*********************************************************************************************************
  Redirection table entry bits: lower 32 bit
*********************************************************************************************************/

#define IOAPIC_INT_MASK                         0x00010000
#define IOAPIC_TRIGGER_MASK                     0x00008000
#define IOAPIC_LEVEL                            0x00008000
#define IOAPIC_EDGE                             0x00000000
#define IOAPIC_REMOTE                           0x00004000
#define IOAPIC_LOW                              0x00002000
#define IOAPIC_HIGH                             0x00000000
#define IOAPIC_LOGICAL                          0x00000800
#define IOAPIC_PHYSICAL                         0x00000000
#define IOAPIC_FIXED                            0x00000000
#define IOAPIC_LOWEST                           0x00000100
#define IOAPIC_SMI                              0x00000200
#define IOAPIC_NMI                              0x00000400
#define IOAPIC_INIT                             0x00000500
#define IOAPIC_EXTINT                           0x00000700
#define IOAPIC_VEC_MASK                         0x000000ff

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

INT     x86IoApicInit(VOID);

UINT32  x86IoApicRedGetLo(UINT8     ucIrq);
UINT32  x86IoApicRedGetHi(UINT8     ucIrq);
VOID    x86IoApicRedSetLo(UINT8     ucIrq, UINT32  uiValue);
VOID    x86IoApicRedSetHi(UINT8     ucIrq, UINT32  uiValue);
VOID    x86IoApicRedUpdateLo(UINT8  ucIrq, UINT32  uiValue, UINT32  uiMask);

VOID    x86IoApicIrqEoi(UINT8       ucIrq);
VOID    x86IoApicIrqEnable(UINT8    ucIrq);
VOID    x86IoApicIrqDisable(UINT8   ucIrq);
BOOL    x86IoApicIrqIsEnable(UINT8  ucIrq);
VOID    x86IoApicIrqSetTarget(UINT8 ucIrq,  UINT8  ucTargetLocalApicId);
VOID    x86IoApicIrqGetTarget(UINT8 ucIrq,  UINT8 *pucTargetLocalApicId);

#endif                                                                  /*  __ARCH_X86IOAPIC_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
