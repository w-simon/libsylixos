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
** 文   件   名: x86LocalApic.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 Local APIC 相关源文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86Idt.h"
#include "x86IoApic.h"
#include "x86LocalApic.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*********************************************************************************************************
  Local APIC Register Offset
*********************************************************************************************************/
#define LOAPIC_ID               0x020                           /*  Local APIC ID Reg                   */
#define LOAPIC_VER              0x030                           /*  Local APIC Version Reg              */
#define LOAPIC_TPR              0x080                           /*  Task Priority Reg                   */
#define LOAPIC_APR              0x090                           /*  Arbitration Priority Reg            */
#define LOAPIC_PPR              0x0a0                           /*  Processor Priority Reg              */
#define LOAPIC_EOI              0x0b0                           /*  EOI Reg                             */
#define LOAPIC_LDR              0x0d0                           /*  Logical Destination Reg             */
#define LOAPIC_DFR              0x0e0                           /*  Destination Format Reg              */
#define LOAPIC_SVR              0x0f0                           /*  Spurious Interrupt Reg              */
#define LOAPIC_ISR              0x100                           /*  In-service Reg                      */
#define LOAPIC_TMR              0x180                           /*  Trigger Mode Reg                    */
#define LOAPIC_IRR              0x200                           /*  Interrupt Request Reg               */
#define LOAPIC_ESR              0x280                           /*  Error Status Reg                    */
#define LOAPIC_ICRLO            0x300                           /*  Interrupt Command Reg               */
#define LOAPIC_ICRHI            0x310                           /*  Interrupt Command Reg               */
#define LOAPIC_TIMER            0x320                           /*  LVT (Timer)                         */
#define LOAPIC_THERMAL          0x330                           /*  LVT (Thermal)                       */
#define LOAPIC_PMC              0x340                           /*  LVT (PMC)                           */
#define LOAPIC_LINT0            0x350                           /*  LVT (LINT0)                         */
#define LOAPIC_LINT1            0x360                           /*  LVT (LINT1)                         */
#define LOAPIC_ERROR            0x370                           /*  LVT (ERROR)                         */
#define LOAPIC_TIMER_ICR        0x380                           /*  Timer Initial Count Reg             */
#define LOAPIC_TIMER_CCR        0x390                           /*  Timer Current Count Reg             */
#define LOAPIC_TIMER_CONFIG     0x3e0                           /*  Timer Divide Config Reg             */
/*********************************************************************************************************
  IA32_APIC_BASE MSR Bits
*********************************************************************************************************/
#define LOAPIC_BASE_MASK        0xfffff000                      /*  LO APIC Base Addr mask              */
#define LOAPIC_GLOBAL_ENABLE    0x00000800                      /*  LO APIC Global Enable               */
#define LOAPIC_BSP              0x00000100                      /*  LO APIC BSP                         */
/*********************************************************************************************************
  Local APIC ID Register Bits
*********************************************************************************************************/
#define LOAPIC_ID_MASK          0x0f000000                      /*  LO APIC ID mask                     */
/*********************************************************************************************************
  Local APIC Version Register Bits
*********************************************************************************************************/
#define LOAPIC_VERSION_MASK     0x000000ff                      /*  LO APIC Version mask                */
#define LOAPIC_MAXLVT_MASK      0x00ff0000                      /*  LO APIC Max LVT mask                */
#define LOAPIC_PENTIUM4         0x00000014                      /*  LO APIC in Pentium4                 */
#define LOAPIC_LVT_PENTIUM4     5                               /*  LO APIC LVT - Pentium4              */
#define LOAPIC_LVT_P6           4                               /*  LO APIC LVT - P6                    */
#define LOAPIC_LVT_P5           3                               /*  LO APIC LVT - P5                    */
/*********************************************************************************************************
  Local APIC Vector Table Bits
*********************************************************************************************************/
#define LOAPIC_VECTOR           0x000000ff                      /*  Vector number                       */
#define LOAPIC_MODE             0x00000700                      /*  Delivery mode                       */
#define LOAPIC_FIXED            0x00000000                      /*  Delivery mode: FIXED                */
#define LOAPIC_SMI              0x00000200                      /*  Delivery mode: SMI                  */
#define LOAPIC_NMI              0x00000400                      /*  Delivery mode: NMI                  */
#define LOAPIC_EXT              0x00000700                      /*  Delivery mode: ExtINT               */
#define LOAPIC_IDLE             0x00000000                      /*  Delivery status: Idle               */
#define LOAPIC_PEND             0x00001000                      /*  Delivery status: Pend               */
#define LOAPIC_HIGH             0x00000000                      /*  Polarity: High                      */
#define LOAPIC_LOW              0x00002000                      /*  Polarity: Low                       */
#define LOAPIC_REMOTE           0x00004000                      /*  Remote IRR                          */
#define LOAPIC_EDGE             0x00000000                      /*  Trigger mode: Edge                  */
#define LOAPIC_LEVEL            0x00008000                      /*  Trigger mode: Level                 */
#define LOAPIC_MASK             0x00010000                      /*  Mask                                */
/*********************************************************************************************************
  Local APIC Spurious-Interrupt Register Bits
*********************************************************************************************************/
#define LOAPIC_ENABLE           0x100                           /*  APIC Enabled                        */
#define LOAPIC_FOCUS_DISABLE    0x200                           /*  Focus Processor Checking            */
/*********************************************************************************************************
  Local APIC Timer Bits
*********************************************************************************************************/
#define LOAPIC_TIMER_DIVBY_2    0x0                             /*  Divide by 2                         */
#define LOAPIC_TIMER_DIVBY_4    0x1                             /*  Divide by 4                         */
#define LOAPIC_TIMER_DIVBY_8    0x2                             /*  Divide by 8                         */
#define LOAPIC_TIMER_DIVBY_16   0x3                             /*  Divide by 16                        */
#define LOAPIC_TIMER_DIVBY_32   0x8                             /*  Divide by 32                        */
#define LOAPIC_TIMER_DIVBY_64   0x9                             /*  Divide by 64                        */
#define LOAPIC_TIMER_DIVBY_128  0xa                             /*  Divide by 128                       */
#define LOAPIC_TIMER_DIVBY_1    0xb                             /*  Divide by 1                         */
#define LOAPIC_TIMER_DIVBY_MASK 0xf                             /*  Mask bits                           */
#define LOAPIC_TIMER_PERIODIC   0x00020000                      /*  Timer Mode: Periodic                */

#define LOCKED_TIMER            0x01
#define LOCKED_PMC              0x02
#define LOCKED_LINT0            0x04
#define LOCKED_LINT1            0x08
#define LOCKED_ERROR            0x10
#define LOCKED_THERMAL          0x20
/*********************************************************************************************************
  Interrupt Command Register: delivery mode and status
*********************************************************************************************************/
#define MODE_FIXED              0x0                             /*  Delivery mode: Fixed                */
#define MODE_LOWEST             0x1                             /*  Delivery mode: Lowest               */
#define MODE_SMI                0x2                             /*  Delivery mode: SMI                  */
#define MODE_NMI                0x4                             /*  Delivery mode: NMI                  */
#define MODE_INIT               0x5                             /*  Delivery mode: INIT                 */
#define MODE_STARTUP            0x6                             /*  Delivery mode: StartUp              */
#define STATUS_PEND             0x1000                          /*  Delivery status: Pend               */
/*********************************************************************************************************
  IMCR related bits
*********************************************************************************************************/
#define IMCR_ADRS               0x22                            /*  IMCR addr reg                       */
#define IMCR_DATA               0x23                            /*  IMCR data reg                       */
#define IMCR_REG_SEL            0x70                            /*  IMCR reg select                     */
#define IMCR_IOAPIC_ON          0x01                            /*  IMCR IOAPIC route enable            */
#define IMCR_IOAPIC_OFF         0x00                            /*  IMCR IOAPIC route disable           */
/*********************************************************************************************************
  ICRLO related bits
*********************************************************************************************************/
#define ICRLO_INIT              0x00000500                      /*  INIT/RESET                          */
#define ICRLO_STARTUP           0x00000600                      /*  Startup IPI                         */
#define ICRLO_DELIVS            0x00001000                      /*  Delivery status                     */
#define ICRLO_ASSERT            0x00004000                      /*  Assert interrupt (vs deassert)      */
#define ICRLO_DEASSERT          0x00000000
#define ICRLO_LEVEL             0x00008000                      /*  Level triggered                     */
#define ICRLO_BCAST             0x00080000                      /*  Send to all APICs, including self.  */
#define ICRLO_BUSY              0x00001000
#define ICRLO_FIXED             0x00000000
/*********************************************************************************************************
  CMOS IO 端口定义
*********************************************************************************************************/
#define CMOS_PORT               0x70
#define CMOS_RETURN             0x71
/*********************************************************************************************************
** 函数名称: x86LocalApicWrite
** 功能描述: 写 Local APIC 寄存器
** 输　入  : uiReg     寄存器
**           uiValue   值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE VOID  x86LocalApicWrite (UINT32  uiReg, UINT32  uiValue)
{
    write32(uiValue, LOCAL_APIC_BASE + uiReg);

    x86LocalApicId();                                           /* Wait for write to finish, by reading */
}
/*********************************************************************************************************
** 函数名称: x86LocalApicRead
** 功能描述: 读 Local APIC 寄存器
** 输　入  : uiReg     寄存器
** 输　出  : 值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE UINT32  x86LocalApicRead (UINT32  uiReg)
{
    return  (read32(LOCAL_APIC_BASE + uiReg));
}
/*********************************************************************************************************
** 函数名称: x86LocalApicInit
** 功能描述: 初始化 Local APIC
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicInit (VOID)
{
    UINT8  ucLocalApicMaxLvt;

    /*
     * Enable local APIC
     * set spurious interrupt vector.
     */
    x86LocalApicWrite(LOAPIC_SVR, LOAPIC_ENABLE | (X86_IRQ_BASE + X86_IRQ_SPURIOUS));

    ucLocalApicMaxLvt = (x86LocalApicRead(LOAPIC_VER) & LOAPIC_MAXLVT_MASK) >> 16;

    /*
     * Reset the DFR, TIMER_CONFIG, and TIMER_ICR
     */
    x86LocalApicWrite(LOAPIC_DFR, 0xffffffff);

    x86LocalApicWrite(LOAPIC_TIMER_CONFIG, 0x0);
    x86LocalApicWrite(LOAPIC_TIMER_ICR, 0x0);

    /*
     * Program Local Vector Table for the Virtual Wire Mode
     */
    /*
     * Set LINT0: extern interrupt, high-polarity, edge-trigger, not-masked
     */
    x86LocalApicWrite(LOAPIC_LINT0, (x86LocalApicRead(LOAPIC_LINT0) &
                      ~(LOAPIC_MODE | LOAPIC_LOW  | LOAPIC_LEVEL | LOAPIC_MASK)) |
                       (LOAPIC_EXT  | LOAPIC_HIGH | LOAPIC_EDGE));

    /*
     * Set LINT1: NMI, high-polarity, edge-trigger, not-masked
     */
    x86LocalApicWrite(LOAPIC_LINT1, (x86LocalApicRead(LOAPIC_LINT1) &
                      ~(LOAPIC_MODE | LOAPIC_LOW  | LOAPIC_LEVEL | LOAPIC_MASK)) |
                       (LOAPIC_NMI  | LOAPIC_HIGH | LOAPIC_EDGE));

    /*
     * Map error interrupt to X86_IRQ_ERROR.
     */
    x86LocalApicWrite(LOAPIC_ERROR, X86_IRQ_BASE + X86_IRQ_ERROR);

    /*
     * Disable performance counter overflow interrupts
     * on machines that provide that interrupt entry.
     */
    if (ucLocalApicMaxLvt >= LOAPIC_LVT_P6) {
        x86LocalApicWrite(LOAPIC_PMC, LOAPIC_MASK);
    }

    if (ucLocalApicMaxLvt >= LOAPIC_LVT_PENTIUM4) {
        x86LocalApicWrite(LOAPIC_THERMAL, LOAPIC_MASK);
    }

    x86LocalApicWrite(LOAPIC_TIMER, LOAPIC_MASK);

    /*
     * Clear error status register (requires back-to-back writes).
     */
    x86LocalApicWrite(LOAPIC_ESR, 0);
    x86LocalApicWrite(LOAPIC_ESR, 0);

    /*
     * Ack any outstanding interrupts.
     */
    x86LocalApicWrite(LOAPIC_EOI, 0);

    /*
     * Send an Init Level De-Assert to synchronise arbitration ID's.
     */
    x86LocalApicWrite(LOAPIC_ICRHI, 0);
    x86LocalApicWrite(LOAPIC_ICRLO, ICRLO_BCAST | ICRLO_INIT | ICRLO_LEVEL);

    while (x86LocalApicRead(LOAPIC_ICRLO) & ICRLO_DELIVS) {
    }

    /*
     * Enable interrupts on the APIC (but not on the processor).
     */
    x86LocalApicWrite(LOAPIC_TPR, 0);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicIrqEnable
** 功能描述: Local APIC 使能 IRQ
** 输　入  : ucVector   中断向量号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86LocalApicIrqEnable (UINT8  ucVector)
{
}
/*********************************************************************************************************
** 函数名称: x86LocalApicIrqDisable
** 功能描述: Local APIC 禁能 IRQ
** 输　入  : ucVector   中断向量号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86LocalApicIrqDisable (UINT8  ucVector)
{
}
/*********************************************************************************************************
** 函数名称: x86LocalApicSecondaryInit
** 功能描述: Secondary CPU 初始化 Local APIC
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicSecondaryInit (VOID)
{
    return  (x86LocalApicInit());
}
/*********************************************************************************************************
** 函数名称: x86LocalApicEoi
** 功能描述: Local APIC 结束中断
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86LocalApicEoi (VOID)
{
    x86LocalApicWrite(LOAPIC_EOI, 0);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicSecondaryStart
** 功能描述: Local APIC 启动 AP Processor
** 输　入  : ucLocalApicID     Local APIC ID
**           uiEntryAddr       实模式进入点地址
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicSecondaryStart (UINT8  ucLocalApicId, addr_t  ulEntryAddr)
{
    UINT16  *pusWarmResetVector;
    INT      i;

    /*
     * "The BSP must initialize CMOS shutdown code to 0AH
     * and the warm reset vector (DWORD based at 40:67) to point at
     * the AP startup code prior to the [universal startup algorithm]."
     */
    out8(0x0F, CMOS_PORT);                                              /*  offset 0xF is shutdown code */
    out8(0x0A, CMOS_PORT + 1);

    pusWarmResetVector = (UINT16 *)((0x40 << 4) | 0x67);                /*  Warm reset vector           */

    write16(0, (addr_t)&pusWarmResetVector[0]);
    write16(ulEntryAddr >> 4, (addr_t)(&pusWarmResetVector[1]));

    /*
     * "Universal startup algorithm."
     * Send INIT (level-triggered) interrupt to reset other CPU.
     */
    x86LocalApicWrite(LOAPIC_ICRHI, ucLocalApicId << 24);
    x86LocalApicWrite(LOAPIC_ICRLO, ICRLO_INIT | ICRLO_LEVEL | ICRLO_ASSERT);
    bspDelayUs(200);

    x86LocalApicWrite(LOAPIC_ICRLO, ICRLO_INIT | ICRLO_LEVEL);
    bspDelayUs(10000);

    /*
     * Send startup IPI (twice!) to enter code.
     * Regular hardware is supposed to only accept a STARTUP
     * when it is in the halted state due to an INIT.  So the second
     * should be ignored, but it is part of the official Intel algorithm.
     */
    for (i = 0; i < 2; i++) {
        x86LocalApicWrite(LOAPIC_ICRHI, ucLocalApicId << 24);
        x86LocalApicWrite(LOAPIC_ICRLO, ICRLO_STARTUP | (ulEntryAddr >> 12));
        bspDelayUs(200);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicSendIpi
** 功能描述: Local APIC 发送 IPI
** 输　入  : ucLocalApicID     Local APIC ID
**           ucX86Vector       x86 中断向量号
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86LocalApicSendIpi (UINT8  ucLocalApicId,  UINT8  ucX86Vector)
{
    x86LocalApicWrite(LOAPIC_ICRHI, ucLocalApicId << 24);
    x86LocalApicWrite(LOAPIC_ICRLO, ICRLO_FIXED | ICRLO_ASSERT | ucX86Vector);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
