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
** ��   ��   ��: x86IoApic.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 29 ��
**
** ��        ��: x86 ��ϵ���� IOAPIC ���Դ�ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86Idt.h"
#include "x86IoApic.h"
#include "x86LocalApic.h"
/*********************************************************************************************************
  The I/O APIC manages hardware interrupts for an SMP system.

  http://www.intel.com/design/chipsets/datashts/29056601.pdf
*********************************************************************************************************/
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
/*********************************************************************************************************
  IO APIC direct register offset
*********************************************************************************************************/
#define IOAPIC_IND              0x00                                /*  Index Register                  */
#define IOAPIC_DATA             0x10                                /*  IO window (data)                */
#define IOAPIC_IRQPA            0x20                                /*  IRQ Pin Assertion Register      */
#define IOAPIC_EOI              0x40                                /*  EOI Register                    */
/*********************************************************************************************************
  IO APIC indirect register offset
*********************************************************************************************************/
#define IOAPIC_ID               0x00                                /*  IOAPIC ID                       */
#define IOAPIC_VERS             0x01                                /*  IOAPIC Version                  */
#define IOAPIC_ARB              0x02                                /*  IOAPIC Arbitration ID           */
#define IOAPIC_BOOT             0x03                                /*  IOAPIC Boot Configuration       */
#define IOAPIC_REDTBL           0x10                                /*  Redirection Table (24 * 64bit)  */
/*********************************************************************************************************
  Version register bits
*********************************************************************************************************/
#define IOAPIC_MRE_MASK         0x00ff0000                          /*  Max Red. entry mask             */
#define IOAPIC_PRQ              0x00008000                          /*  This has IRQ reg                */
#define IOAPIC_VERSION          0x000000ff                          /*  Version number                  */
/*********************************************************************************************************
  Interrupt delivery type
*********************************************************************************************************/
#define IOAPIC_DT_APIC          0x0                                 /*  APIC serial bus                 */
#define IOAPIC_DT_FS            0x1                                 /*  Front side bus message          */
/*********************************************************************************************************
   ȫ�ֱ�������
*********************************************************************************************************/
static UINT8                _G_ucX86IoApicMaxIntr = 0;              /*  ֧�ֵ�����ж���                */
static LW_SPINLOCK_DEFINE  (_G_slX86IoApic);                        /*  ���ʼĴ�����������              */
/*********************************************************************************************************
** ��������: __x86IoApicRegGet
** ��������: IOAPIC ���Ĵ���
** �䡡��  : ucRegIndex        �Ĵ�������
** �䡡��  : ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  __x86IoApicRegGet (UINT8  ucRegIndex)
{
    UINT32  uiValue;
    INTREG  iregInterLevel;

    LW_SPIN_LOCK_QUICK(&_G_slX86IoApic, &iregInterLevel);

    write8(ucRegIndex, X86_IOAPIC_BASE + IOAPIC_IND);
    uiValue = read32(X86_IOAPIC_BASE + IOAPIC_DATA);

    LW_SPIN_UNLOCK_QUICK(&_G_slX86IoApic, iregInterLevel);

    return  (uiValue);
}
/*********************************************************************************************************
  ��ȡ ID �Ͱ汾�Ĵ����꺯��
*********************************************************************************************************/
#define __x86IoApicIdRegGet()       __x86IoApicRegGet(IOAPIC_ID)
#define __x86IoApicVersionRegGet()  __x86IoApicRegGet(IOAPIC_VERS)
/*********************************************************************************************************
** ��������: __x86IoApicRegSet
** ��������: IOAPIC д�Ĵ���
** �䡡��  : ucRegIndex        �Ĵ�������
**           uiValue           ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __x86IoApicRegSet (UINT8  ucRegIndex, UINT32  uiValue)
{
    INTREG  iregInterLevel;

    LW_SPIN_LOCK_QUICK(&_G_slX86IoApic, &iregInterLevel);

    write8(ucRegIndex, X86_IOAPIC_BASE + IOAPIC_IND);
    write32(uiValue, X86_IOAPIC_BASE + IOAPIC_DATA);

    LW_SPIN_UNLOCK_QUICK(&_G_slX86IoApic, iregInterLevel);
}
/*********************************************************************************************************
** ��������: x86IoApicRedGetLo
** ��������: ��� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : ucIrq             ����
** �䡡��  : ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  x86IoApicRedGetLo (UINT8  ucIrq)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        UINT8  ucRegIndex = IOAPIC_REDTBL + (ucIrq << 1);
        return  (__x86IoApicRegGet(ucRegIndex));

    } else {
        return  ((UINT32)~0);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicRedGetHi
** ��������: ��� Red ����Ŀ�ĸ� 32 λ
** �䡡��  : ucIrq             ����
** �䡡��  : ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  x86IoApicRedGetHi (UINT8  ucIrq)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        UINT8  ucRegIndex = IOAPIC_REDTBL + (ucIrq << 1) + 1;
        return  (__x86IoApicRegGet(ucRegIndex));

    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicRedSetLo
** ��������: ���� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : ucIrq             ����
**           uiValue           ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86IoApicRedSetLo (UINT8  ucIrq, UINT32  uiValue)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        UINT8  ucRegIndex = IOAPIC_REDTBL + (ucIrq << 1);
        __x86IoApicRegSet(ucRegIndex, uiValue);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicRedSetLo
** ��������: ���� Red ����Ŀ�ĸ� 32 λ
** �䡡��  : ucIrq             ����
**           uiValue           ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86IoApicRedSetHi (UINT8  ucIrq, UINT32  uiValue)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        UINT8  ucRegIndex = IOAPIC_REDTBL + (ucIrq << 1) + 1;
        __x86IoApicRegSet(ucRegIndex, uiValue);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicRedUpdateLo
** ��������: ���� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : ucIrq             ����
**           uiValue           ֵ
**           uiMask            ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86IoApicRedUpdateLo (UINT8  ucIrq, UINT32  uiValue, UINT32  uiMask)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        x86IoApicRedSetLo(ucIrq, (x86IoApicRedGetLo(ucIrq) & ~uiMask) | (uiValue & uiMask));
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqEoi
** ��������: IOAPIC �����ж�
** �䡡��  : ucIrq             ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86IoApicIrqEoi (UINT8  ucIrq)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        if (__x86IoApicVersionRegGet() & IOAPIC_PRQ) {
            UINT8  ucX86Vector = x86IoApicRedGetLo(ucIrq) & IOAPIC_VEC_MASK;
            write32(ucX86Vector, X86_IOAPIC_BASE + IOAPIC_EOI);
        }
    }

    x86LocalApicEoi();
}
/*********************************************************************************************************
** ��������: x86IoApicIrqEnable
** ��������: IOAPIC ʹ�� IRQ
** �䡡��  : ucIrq             ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86IoApicIrqEnable (UINT8  ucIrq)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        x86IoApicRedUpdateLo(ucIrq, 0, IOAPIC_INT_MASK);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqDisable
** ��������: IOAPIC ���� IRQ
** �䡡��  : ucIrq             ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86IoApicIrqDisable (UINT8  ucIrq)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        x86IoApicRedUpdateLo(ucIrq, IOAPIC_INT_MASK, IOAPIC_INT_MASK);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqIsEnable
** ��������: IOAPIC �Ƿ�ʹ��ָ���� IRQ
** �䡡��  : ucIrq             ����
** �䡡��  : TRUE OR FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  x86IoApicIrqIsEnable (UINT8  ucIrq)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        return  ((x86IoApicRedGetLo(ucIrq) & IOAPIC_INT_MASK) ? (LW_FALSE) : (LW_TRUE));

    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqSetTarget
** ��������: IOAPIC ���� IRQ �ж�Ŀ�� CPU
** �䡡��  : ucIrq                 ����
**           ucTargetLocalApicId   Ŀ�� CPU Local APIC ID
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86IoApicIrqSetTarget (UINT8  ucIrq, UINT8  ucTargetLocalApicId)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        x86IoApicRedSetHi(ucIrq, ucTargetLocalApicId << 24);            /*  �ַ�����ָ���� CPU          */
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqGetTarget
** ��������: IOAPIC ��� IRQ �ж�Ŀ�� CPU
** �䡡��  : ucIrq                 ����
**           pucTargetLocalApicId  Ŀ�� CPU Local APIC ID
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID    x86IoApicIrqGetTarget(UINT8 ucIrq, UINT8 *pucTargetLocalApicId)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        *pucTargetLocalApicId = x86IoApicRedGetHi(ucIrq) >> 24;

    } else {
        *pucTargetLocalApicId = 0;
    }
}
/*********************************************************************************************************
** ��������: x86IoApicInit
** ��������: ��ʼ�� IOAPIC
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicInit (VOID)
{
    UINT32  uiRteValue;
    INT     i;
    
    LW_SPIN_INIT(&_G_slX86IoApic);

    _G_ucX86IoApicMaxIntr = ((__x86IoApicVersionRegGet() >> 16) & 0xff) + 1;

    uiRteValue = IOAPIC_EDGE     |                                      /*  �����źŴ���                */
                 IOAPIC_HIGH     |                                      /*  �ߵ�ƽ��Ч(PCIΪ�͵�ƽ)     */
                 IOAPIC_FIXED    |                                      /*  �̶��ַ��� DEST ������� CPU*/
                 IOAPIC_INT_MASK |                                      /*  �����ж�                    */
                 IOAPIC_PHYSICAL;                                       /*  ����ģʽ, DEST = APIC ID    */

    for (i = 0; i < _G_ucX86IoApicMaxIntr; i++) {
        x86IoApicRedSetLo(i, uiRteValue | (X86_IRQ_BASE + i));          /*  �� 8 λΪ x86 �ж�����      */
        x86IoApicRedSetHi(i, 0 << 24);                                  /*  DEST ��Ϊ 0(�ַ��� BSP)     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
