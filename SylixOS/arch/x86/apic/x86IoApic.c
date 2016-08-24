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
** 文   件   名: x86IoApic.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 IOAPIC 相关源文件.
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
  宏定义
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
   全局变量定义
*********************************************************************************************************/
static UINT8                _G_ucX86IoApicMaxIntr = 0;              /*  支持的最大中断数                */
static LW_SPINLOCK_DEFINE  (_G_slX86IoApic);                        /*  访问寄存器用自旋锁              */
/*********************************************************************************************************
** 函数名称: __x86IoApicRegGet
** 功能描述: IOAPIC 读寄存器
** 输　入  : ucRegIndex        寄存器索引
** 输　出  : 值
** 全局变量:
** 调用模块:
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
  读取 ID 和版本寄存器宏函数
*********************************************************************************************************/
#define __x86IoApicIdRegGet()       __x86IoApicRegGet(IOAPIC_ID)
#define __x86IoApicVersionRegGet()  __x86IoApicRegGet(IOAPIC_VERS)
/*********************************************************************************************************
** 函数名称: __x86IoApicRegSet
** 功能描述: IOAPIC 写寄存器
** 输　入  : ucRegIndex        寄存器索引
**           uiValue           值
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: x86IoApicRedGetLo
** 功能描述: 获得 Red 表条目的低 32 位
** 输　入  : ucIrq             索引
** 输　出  : 值
** 全局变量:
** 调用模块:
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
** 函数名称: x86IoApicRedGetHi
** 功能描述: 获得 Red 表条目的高 32 位
** 输　入  : ucIrq             索引
** 输　出  : 值
** 全局变量:
** 调用模块:
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
** 函数名称: x86IoApicRedSetLo
** 功能描述: 设置 Red 表条目的低 32 位
** 输　入  : ucIrq             索引
**           uiValue           值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86IoApicRedSetLo (UINT8  ucIrq, UINT32  uiValue)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        UINT8  ucRegIndex = IOAPIC_REDTBL + (ucIrq << 1);
        __x86IoApicRegSet(ucRegIndex, uiValue);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicRedSetLo
** 功能描述: 设置 Red 表条目的高 32 位
** 输　入  : ucIrq             索引
**           uiValue           值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86IoApicRedSetHi (UINT8  ucIrq, UINT32  uiValue)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        UINT8  ucRegIndex = IOAPIC_REDTBL + (ucIrq << 1) + 1;
        __x86IoApicRegSet(ucRegIndex, uiValue);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicRedUpdateLo
** 功能描述: 更新 Red 表条目的低 32 位
** 输　入  : ucIrq             索引
**           uiValue           值
**           uiMask            掩码
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86IoApicRedUpdateLo (UINT8  ucIrq, UINT32  uiValue, UINT32  uiMask)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        x86IoApicRedSetLo(ucIrq, (x86IoApicRedGetLo(ucIrq) & ~uiMask) | (uiValue & uiMask));
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqEoi
** 功能描述: IOAPIC 结束中断
** 输　入  : ucIrq             索引
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: x86IoApicIrqEnable
** 功能描述: IOAPIC 使能 IRQ
** 输　入  : ucIrq             索引
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86IoApicIrqEnable (UINT8  ucIrq)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        x86IoApicRedUpdateLo(ucIrq, 0, IOAPIC_INT_MASK);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqDisable
** 功能描述: IOAPIC 禁能 IRQ
** 输　入  : ucIrq             索引
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86IoApicIrqDisable (UINT8  ucIrq)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        x86IoApicRedUpdateLo(ucIrq, IOAPIC_INT_MASK, IOAPIC_INT_MASK);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqIsEnable
** 功能描述: IOAPIC 是否使能指定的 IRQ
** 输　入  : ucIrq             索引
** 输　出  : TRUE OR FALSE
** 全局变量:
** 调用模块:
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
** 函数名称: x86IoApicIrqSetTarget
** 功能描述: IOAPIC 设置 IRQ 中断目标 CPU
** 输　入  : ucIrq                 索引
**           ucTargetLocalApicId   目标 CPU Local APIC ID
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86IoApicIrqSetTarget (UINT8  ucIrq, UINT8  ucTargetLocalApicId)
{
    if (ucIrq < _G_ucX86IoApicMaxIntr) {
        x86IoApicRedSetHi(ucIrq, ucTargetLocalApicId << 24);            /*  分发到该指定的 CPU          */
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqGetTarget
** 功能描述: IOAPIC 获得 IRQ 中断目标 CPU
** 输　入  : ucIrq                 索引
**           pucTargetLocalApicId  目标 CPU Local APIC ID
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: x86IoApicInit
** 功能描述: 初始化 IOAPIC
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicInit (VOID)
{
    UINT32  uiRteValue;
    INT     i;
    
    LW_SPIN_INIT(&_G_slX86IoApic);

    _G_ucX86IoApicMaxIntr = ((__x86IoApicVersionRegGet() >> 16) & 0xff) + 1;

    uiRteValue = IOAPIC_EDGE     |                                      /*  边沿信号触发                */
                 IOAPIC_HIGH     |                                      /*  高电平有效(PCI为低电平)     */
                 IOAPIC_FIXED    |                                      /*  固定分发到 DEST 域的所有 CPU*/
                 IOAPIC_INT_MASK |                                      /*  屏蔽中断                    */
                 IOAPIC_PHYSICAL;                                       /*  物理模式, DEST = APIC ID    */

    for (i = 0; i < _G_ucX86IoApicMaxIntr; i++) {
        x86IoApicRedSetLo(i, uiRteValue | (X86_IRQ_BASE + i));          /*  低 8 位为 x86 中断向量      */
        x86IoApicRedSetHi(i, 0 << 24);                                  /*  DEST 域为 0(分发到 BSP)     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
