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
** ��   ��   ��: armCacheV7M.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 11 �� 08 ��
**
** ��        ��: ARMv7 Cortex-Mx ��ϵ���� CACHE ����.
**
** CHANGELOG:
2018.01.02 Hou.JinYu (�����)    ʵ�� Cortex-M7 ��ϵ�ܹ� CACHE ����.
2018.01.08 Jiao.JinXing (������) ���� SCB CACHE ����.
2018.07.11 Han.Hui (����) ���� Cache line size ̽��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../../mpu/v7m/armMpuV7M.h"
/*********************************************************************************************************
  CACHE ��
*********************************************************************************************************/
static UINT32                               uiArmV7MCacheLineSize;
#define CACHE_LINE_MASK                     ((addr_t)uiArmV7MCacheLineSize - 1)
#define CACHE_LOOP_OP_MAX_SIZE              (8 * LW_CFG_KB_SIZE)
#define CACHE_ALIGN_ADDR(pvAdrs)            ((PVOID)((addr_t)pvAdrs & (~CACHE_LINE_MASK)))
#define CACHE_ALIGN_SIZE(pvAdrs, stBytes)   (((addr_t)pvAdrs + stBytes) - (addr_t)CACHE_ALIGN_ADDR(pvAdrs))
/*********************************************************************************************************
  Structure type to access the System Control Block (SCB).
*********************************************************************************************************/
typedef struct {
    UINT32  CPUID;                          /*  CPUID Base Register                                     */
    UINT32  ICSR;                           /*  Interrupt Control and State Register                    */
    UINT32  VTOR;                           /*  Vector Table Offset Register                            */
    UINT32  AIRCR;                          /*  Application Interrupt and Reset Control Register        */
    UINT32  SCR;                            /*  System Control Register                                 */
    UINT32  CCR;                            /*  Configuration Control Register                          */
    UINT8   SHPR[12U];                      /*  System Handlers Priority Registers (4-7, 8-11, 12-15)   */
    UINT32  SHCSR;                          /*  System Handler Control and State Register               */
    UINT32  CFSR;                           /*  Configurable Fault Status Register                      */
    UINT32  HFSR;                           /*  HardFault Status Register                               */
    UINT32  DFSR;                           /*  Debug Fault Status Register                             */
    UINT32  MMFAR;                          /*  MemManage Fault Address Register                        */
    UINT32  BFAR;                           /*  BusFault Address Register                               */
    UINT32  AFSR;                           /*  Auxiliary Fault Status Register                         */
    UINT32  ID_PFR[2U];                     /*  Processor Feature Register                              */
    UINT32  ID_DFR;                         /*  Debug Feature Register                                  */
    UINT32  ID_AFR;                         /*  Auxiliary Feature Register                              */
    UINT32  ID_MFR[4U];                     /*  Memory Model Feature Register                           */
    UINT32  ID_ISAR[5U];                    /*  Instruction Set Attributes Register                     */
    UINT32  RESERVED0[1U];
    UINT32  CLIDR;                          /*  Cache Level ID register                                 */
    UINT32  CTR;                            /*  Cache Type register                                     */
    UINT32  CCSIDR;                         /*  Cache Size ID Register                                  */
    UINT32  CSSELR;                         /*  Cache Size Selection Register                           */
    UINT32  CPACR;                          /*  Coprocessor Access Control Register                     */
    UINT32  RESERVED3[93U];
    UINT32  STIR;                           /*  Software Triggered Interrupt Register                   */
    UINT32  RESERVED4[15U];
    UINT32  MVFR0;                          /*  Media and VFP Feature Register 0                        */
    UINT32  MVFR1;                          /*  Media and VFP Feature Register 1                        */
    UINT32  MVFR2;                          /*  Media and VFP Feature Register 2                        */
    UINT32  RESERVED5[1U];
    UINT32  ICIALLU;                        /*  I-Cache Invalidate All to PoU                           */
    UINT32  RESERVED6[1U];
    UINT32  ICIMVAU;                        /*  I-Cache Invalidate by MVA to PoU                        */
    UINT32  DCIMVAC;                        /*  D-Cache Invalidate by MVA to PoC                        */
    UINT32  DCISW;                          /*  D-Cache Invalidate by Set-way                           */
    UINT32  DCCMVAU;                        /*  D-Cache Clean by MVA to PoU                             */
    UINT32  DCCMVAC;                        /*  D-Cache Clean by MVA to PoC                             */
    UINT32  DCCSW;                          /*  D-Cache Clean by Set-way                                */
    UINT32  DCCIMVAC;                       /*  D-Cache Clean and Invalidate by MVA to PoC              */
    UINT32  DCCISW;                         /*  D-Cache Clean and Invalidate by Set-way                 */
    UINT32  RESERVED7[6U];
    UINT32  ITCMCR;                         /*  Instruction Tightly-Coupled Memory Control Register     */
    UINT32  DTCMCR;                         /*  Data Tightly-Coupled Memory Control Registers           */
    UINT32  AHBPCR;                         /*  AHBP Control Register                                   */
    UINT32  CACR;                           /*  L1 Cache Control Register                               */
    UINT32  AHBSCR;                         /*  AHB Slave Control Register                              */
    UINT32  RESERVED8[1U];
    UINT32  ABFSR;                          /*  Auxiliary Bus Fault Status Register                     */
} SCB_Type;
/*********************************************************************************************************
  Memory mapping of Core Hardware
*********************************************************************************************************/
#define SCS_BASE                            (0xe000e000UL)              /*  System Control Space Address*/
#define SCB_BASE                            (SCS_BASE + 0x0d00UL)       /*  System Control Block Address*/

#define SCB                                 ((SCB_Type *)SCB_BASE)      /*  SCB configuration struct    */
/*********************************************************************************************************
  SCB Configuration Control Register Definitions
*********************************************************************************************************/
#define SCB_CCR_BP_Pos                      18U                         /*  Branch prediction enable bit*/
#define SCB_CCR_BP_Msk                      (1UL << SCB_CCR_BP_Pos)

#define SCB_CCR_IC_Pos                      17U                         /*  Instruction cache enable bit*/
#define SCB_CCR_IC_Msk                      (1UL << SCB_CCR_IC_Pos)

#define SCB_CCR_DC_Pos                      16U                         /*  Cache enable bit            */
#define SCB_CCR_DC_Msk                      (1UL << SCB_CCR_DC_Pos)

#define SCB_CCR_STKALIGN_Pos                9U                          /*  STKALIGN                    */
#define SCB_CCR_STKALIGN_Msk                (1UL << SCB_CCR_STKALIGN_Pos)

#define SCB_CCR_BFHFNMIGN_Pos               8U                          /*  BFHFNMIGN                   */
#define SCB_CCR_BFHFNMIGN_Msk               (1UL << SCB_CCR_BFHFNMIGN_Pos)

#define SCB_CCR_DIV_0_TRP_Pos               4U                          /*  DIV_0_TRP                   */
#define SCB_CCR_DIV_0_TRP_Msk               (1UL << SCB_CCR_DIV_0_TRP_Pos)

#define SCB_CCR_UNALIGN_TRP_Pos             3U                          /*  UNALIGN_TRP                 */
#define SCB_CCR_UNALIGN_TRP_Msk             (1UL << SCB_CCR_UNALIGN_TRP_Pos)

#define SCB_CCR_USERSETMPEND_Pos            1U                          /*  USERSETMPEND                */
#define SCB_CCR_USERSETMPEND_Msk            (1UL << SCB_CCR_USERSETMPEND_Pos)

#define SCB_CCR_NONBASETHRDENA_Pos          0U                          /*  NONBASETHRDENA              */
#define SCB_CCR_NONBASETHRDENA_Msk          (1UL << SCB_CCR_NONBASETHRDENA_Pos)
/*********************************************************************************************************
  SCB Cache Level ID Register Definitions
*********************************************************************************************************/
#define SCB_CLIDR_LOUU_Pos                  27U                         /*  LoUU Position               */
#define SCB_CLIDR_LOUU_Msk                  (7UL << SCB_CLIDR_LOUU_Pos)

#define SCB_CLIDR_LOC_Pos                   24U                         /*  LoC Position                */
#define SCB_CLIDR_LOC_Msk                   (7UL << SCB_CLIDR_LOC_Pos)
/*********************************************************************************************************
  SCB Cache Type Register Definitions
*********************************************************************************************************/
#define SCB_CTR_FORMAT_Pos                  29U                         /*  Format Position             */
#define SCB_CTR_FORMAT_Msk                  (7UL << SCB_CTR_FORMAT_Pos)

#define SCB_CTR_CWG_Pos                     24U                         /*  CWG Position                */
#define SCB_CTR_CWG_Msk                     (0xFUL << SCB_CTR_CWG_Pos)

#define SCB_CTR_ERG_Pos                     20U                         /*  ERG Position                */
#define SCB_CTR_ERG_Msk                     (0xFUL << SCB_CTR_ERG_Pos)

#define SCB_CTR_DMINLINE_Pos                16U                         /*  DminLine Position           */
#define SCB_CTR_DMINLINE_Msk                (0xFUL << SCB_CTR_DMINLINE_Pos)

#define SCB_CTR_IMINLINE_Pos                0U                          /*  ImInLine Position           */
#define SCB_CTR_IMINLINE_Msk                (0xFUL << SCB_CTR_IMINLINE_Pos)
/*********************************************************************************************************
  SCB Cache Size ID Register Definitions
*********************************************************************************************************/
#define SCB_CCSIDR_WT_Pos                   31U                         /*  WT Position                 */
#define SCB_CCSIDR_WT_Msk                   (1UL << SCB_CCSIDR_WT_Pos)

#define SCB_CCSIDR_WB_Pos                   30U                         /*  WB Position                 */
#define SCB_CCSIDR_WB_Msk                   (1UL << SCB_CCSIDR_WB_Pos)

#define SCB_CCSIDR_RA_Pos                   29U                         /*  RA Position                 */
#define SCB_CCSIDR_RA_Msk                   (1UL << SCB_CCSIDR_RA_Pos)

#define SCB_CCSIDR_WA_Pos                   28U                         /*  WA Position                 */
#define SCB_CCSIDR_WA_Msk                   (1UL << SCB_CCSIDR_WA_Pos)

#define SCB_CCSIDR_NUMSETS_Pos              13U                         /*  NumSets Position            */
#define SCB_CCSIDR_NUMSETS_Msk              (0x7FFFUL << SCB_CCSIDR_NUMSETS_Pos)

#define SCB_CCSIDR_ASSOCIATIVITY_Pos        3U                          /*  Associativity Position      */
#define SCB_CCSIDR_ASSOCIATIVITY_Msk        (0x3FFUL << SCB_CCSIDR_ASSOCIATIVITY_Pos)

#define SCB_CCSIDR_LINESIZE_Pos             0U                          /*  LineSize Position           */
#define SCB_CCSIDR_LINESIZE_Msk             (7UL << SCB_CCSIDR_LINESIZE_Pos)
/*********************************************************************************************************
  SCB Cache Size Selection Register Definitions
*********************************************************************************************************/
#define SCB_CSSELR_LEVEL_Pos                1U                          /*  Level Position              */
#define SCB_CSSELR_LEVEL_Msk                (7UL << SCB_CSSELR_LEVEL_Pos)

#define SCB_CSSELR_IND_Pos                  0U                          /*  InD Position                */
#define SCB_CSSELR_IND_Msk                  (1UL << SCB_CSSELR_IND_Pos)
/*********************************************************************************************************
  SCB D-Cache Invalidate by Set-way Register Definitions
*********************************************************************************************************/
#define SCB_DCISW_WAY_Pos                   30U                         /*  Way Position                */
#define SCB_DCISW_WAY_Msk                   (3UL << SCB_DCISW_WAY_Pos)

#define SCB_DCISW_SET_Pos                   5U                          /*  Set Position                */
#define SCB_DCISW_SET_Msk                   (0x1FFUL << SCB_DCISW_SET_Pos)
/*********************************************************************************************************
  SCB D-Cache Clean by Set-way Register Definitions
*********************************************************************************************************/
#define SCB_DCCSW_WAY_Pos                   30U                         /*  Way Position                */
#define SCB_DCCSW_WAY_Msk                   (3UL << SCB_DCCSW_WAY_Pos)

#define SCB_DCCSW_SET_Pos                   5U                          /*  Set Position                */
#define SCB_DCCSW_SET_Msk                   (0x1FFUL << SCB_DCCSW_SET_Pos)
/*********************************************************************************************************
  SCB D-Cache Clean and Invalidate by Set-way Register Definitions
*********************************************************************************************************/
#define SCB_DCCISW_WAY_Pos                  30U                         /*  Way Position                */
#define SCB_DCCISW_WAY_Msk                  (3UL << SCB_DCCISW_WAY_Pos)

#define SCB_DCCISW_SET_Pos                  5U                          /*  Set Position                */
#define SCB_DCCISW_SET_Msk                  (0x1FFUL << SCB_DCCISW_SET_Pos)
/*********************************************************************************************************
  Cache Size ID Register Macros
*********************************************************************************************************/
#define CCSIDR_WAYS(x)                      \
    (((x) & SCB_CCSIDR_ASSOCIATIVITY_Msk) >> SCB_CCSIDR_ASSOCIATIVITY_Pos)
#define CCSIDR_SETS(x)                      \
    (((x) & SCB_CCSIDR_NUMSETS_Msk) >> SCB_CCSIDR_NUMSETS_Pos)
#define CCSIDR_LINESIZE(x)                  \
    (((x) & SCB_CCSIDR_LINESIZE_Msk) >> SCB_CCSIDR_LINESIZE_Pos)
/*********************************************************************************************************
** ��������: SCB_EnableICache
** ��������: Enable I-Cache
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_EnableICache (VOID)
{
    armDsb();
    armIsb();
    SCB->ICIALLU = 0UL;                                                 /*  Invalidate I-Cache          */
    armDsb();
    armIsb();
    SCB->CCR |=  (UINT32)SCB_CCR_IC_Msk;                                /*  Enable I-Cache              */
    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_DisableICache
** ��������: Disable I-Cache
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_DisableICache (VOID)
{
    armDsb();
    armIsb();
    SCB->CCR &= ~(UINT32)SCB_CCR_IC_Msk;                                /*  Disable I-Cache             */
    SCB->ICIALLU = 0UL;                                                 /*  Invalidate I-Cache          */
    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_InvalidateICache
** ��������: Invalidate I-Cache
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_InvalidateICache (VOID)
{
    armDsb();
    armIsb();
    SCB->ICIALLU = 0UL;
    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_EnableDCache
** ��������: Enable D-Cache
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_EnableDCache (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    ccsidr = SCB->CCSIDR;

    /*
     * Invalidate D-Cache
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCISW = (((sets << SCB_DCISW_SET_Pos) & SCB_DCISW_SET_Msk) |
                          ((ways << SCB_DCISW_WAY_Pos) & SCB_DCISW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while(sets-- != 0U);
    armDsb();

    SCB->CCR |= (UINT32)SCB_CCR_DC_Msk;                                 /*  Enable D-Cache              */

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_DisableDCache
** ��������: Disable D-Cache
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_DisableDCache (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    SCB->CCR &= ~(UINT32)SCB_CCR_DC_Msk;                                /*  Disable D-Cache             */
    armDsb();

    ccsidr = SCB->CCSIDR;

    /*
     * Clean & invalidate D-Cache
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCCISW = (((sets << SCB_DCCISW_SET_Pos) & SCB_DCCISW_SET_Msk) |
                           ((ways << SCB_DCCISW_WAY_Pos) & SCB_DCCISW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while(sets-- != 0U);

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_InvalidateDCache
** ��������: Invalidate D-Cache
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_InvalidateDCache (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    ccsidr = SCB->CCSIDR;

    /*
     * Invalidate D-Cache
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCISW = (((sets << SCB_DCISW_SET_Pos) & SCB_DCISW_SET_Msk) |
                          ((ways << SCB_DCISW_WAY_Pos) & SCB_DCISW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while(sets-- != 0U);

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_CleanDCache
** ��������: Clean D-Cache
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_CleanDCache (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    ccsidr = SCB->CCSIDR;

    /*
     * Clean D-Cache
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCCSW = (((sets << SCB_DCCSW_SET_Pos) & SCB_DCCSW_SET_Msk) |
                          ((ways << SCB_DCCSW_WAY_Pos) & SCB_DCCSW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while(sets-- != 0U);

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_CleanInvalidateDCache
** ��������: Clean & Invalidate D-Cache
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_CleanInvalidateDCache (VOID)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  ways;

    SCB->CSSELR = (0U << 1U) | 0U;                                      /*  Level 1 data cache          */
    armDsb();

    ccsidr = SCB->CCSIDR;

    /*
     * Clean & invalidate D-Cache
     */
    sets = (UINT32)(CCSIDR_SETS(ccsidr));
    do {
        ways = (UINT32)(CCSIDR_WAYS(ccsidr));
        do {
            SCB->DCCISW = (((sets << SCB_DCCISW_SET_Pos) & SCB_DCCISW_SET_Msk) |
                           ((ways << SCB_DCCISW_WAY_Pos) & SCB_DCCISW_WAY_Msk));
#if defined(__CC_ARM)
            __schedule_barrier();
#elif defined(__GNUC__)
            KN_BARRIER();
#endif
        } while (ways-- != 0U);
    } while(sets-- != 0U);

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_InvalidateDCache_by_Addr
** ��������: D-Cache Invalidate by address
** �䡡��  : addr    address (aligned to 32-byte boundary)
**           dsize   size of memory block (in number of bytes)
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_InvalidateDCache_by_Addr (UINT32  *addr, INT32  dsize)
{
    INT32   op_size  = dsize;
    UINT32  op_addr  = (UINT32)addr;
    INT32   linesize = uiArmV7MCacheLineSize;

    armDsb();

    while (op_size > 0) {
        SCB->DCIMVAC = op_addr;
        op_addr += (UINT32)linesize;
        op_size -=         linesize;
    }

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_CleanDCache_by_Addr
** ��������: D-Cache Clean by address
** �䡡��  : addr    address (aligned to 32-byte boundary)
**           dsize   size of memory block (in number of bytes)
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID SCB_CleanDCache_by_Addr (UINT32  *addr, INT32  dsize)
{
    INT32  op_size  = dsize;
    UINT32 op_addr  = (UINT32)addr;
    INT32  linesize = uiArmV7MCacheLineSize;

    armDsb();

    while (op_size > 0) {
        SCB->DCCMVAC = op_addr;
        op_addr += (UINT32)linesize;
        op_size -=         linesize;
    }

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: SCB_CleanInvalidateDCache_by_Addr
** ��������: D-Cache Clean and Invalidate by address
** �䡡��  : addr    address (aligned to 32-byte boundary)
**           dsize   size of memory block (in number of bytes)
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  SCB_CleanInvalidateDCache_by_Addr (UINT32  *addr, INT32  dsize)
{
    INT32  op_size  = dsize;
    UINT32 op_addr  = (UINT32)addr;
    INT32  linesize = uiArmV7MCacheLineSize;

    armDsb();

    while (op_size > 0) {
        SCB->DCCIMVAC = op_addr;
        op_addr += (UINT32)linesize;
        op_size -=         linesize;
    }

    armDsb();
    armIsb();
}
/*********************************************************************************************************
** ��������: armCacheV7MEnable
** ��������: ʹ�� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        SCB_EnableICache();                                             /*  ʹ��ָ���                */

    } else {
        SCB_EnableDCache();                                             /*  ʹ�����ݻ���                */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MDisable
** ��������: ���� CACHE
** �䡡��  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        SCB_DisableICache();                                            /*  ����ָ���                */

    } else {
        SCB_DisableDCache();                                            /*  �������ݻ���                */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MFlush
** ��������: CACHE �����ݻ�д
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == DATA_CACHE) {
        if (stBytes >= CACHE_LOOP_OP_MAX_SIZE) {
            SCB_CleanDCache();                                          /*  ȫ����д���ݻ���            */

        } else {                                                        /*  ���ֻ�д���ݻ���            */
            SCB_CleanDCache_by_Addr(CACHE_ALIGN_ADDR(pvAdrs),
                                    CACHE_ALIGN_SIZE(pvAdrs, stBytes));
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MInvalidate
** ��������: ָ�����͵� CACHE ʹ������Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == DATA_CACHE) {
        if (stBytes >= CACHE_LOOP_OP_MAX_SIZE) {
            SCB_InvalidateDCache();                                     /*  ȫ����Ч���ݻ���            */

        } else {                                                        /*  ������Ч���ݻ���            */
            addr_t  ulStart = (addr_t)pvAdrs;
            addr_t  ulEnd   = (addr_t)pvAdrs + stBytes;

            if (ulStart & CACHE_LINE_MASK) {                            /*  ��ʼ��ַ�� cache line ����  */
                ulStart &= ~CACHE_LINE_MASK;
                SCB_CleanInvalidateDCache_by_Addr((VOID *)ulStart, uiArmV7MCacheLineSize);
                ulStart += uiArmV7MCacheLineSize;
            }

            if (ulEnd & CACHE_LINE_MASK) {                              /*  ������ַ�� cache line ����  */
                ulEnd &= ~CACHE_LINE_MASK;
                SCB_CleanInvalidateDCache_by_Addr((VOID *)ulEnd, uiArmV7MCacheLineSize);
            }

            if (ulEnd > ulStart) {                                      /*  ����Ч���벿��              */
                SCB_InvalidateDCache_by_Addr((VOID *)ulStart, ulEnd - ulStart);
            }
        }

    } else {
        SCB_InvalidateICache();                                         /*  ȫ����Чָ���            */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MClear
** ��������: ָ�����͵� CACHE ʹ���ֻ�ȫ�����(��д�ڴ�)����Ч(���ʲ�����)
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == DATA_CACHE) {
        if (stBytes >= CACHE_LOOP_OP_MAX_SIZE) {
            SCB_CleanInvalidateDCache();                                /*  ȫ����д����Ч��Ч���ݻ���  */

        } else {                                                        /*  ���ֻ�д����Ч��Ч���ݻ���  */
            SCB_CleanInvalidateDCache_by_Addr(CACHE_ALIGN_ADDR(pvAdrs),
                                              CACHE_ALIGN_SIZE(pvAdrs, stBytes));
        }

    } else {
        SCB_InvalidateICache();                                         /*  ȫ����Чָ���            */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: armCacheV7MLock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV7MUnlock
** ��������: ����ָ�����͵� CACHE
** �䡡��  : cachetype     CACHE ����
**           pvAdrs        �����ַ
**           stBytes       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  armCacheV7MUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: armCacheV7MTextUpdate
** ��������: ���(��д�ڴ�) D CACHE ��Ч(���ʲ�����) I CACHE
** �䡡��  : pvAdrs                        �����ַ
**           stBytes                       ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : L2 cache Ϊͳһ CACHE ���� text update ����Ҫ���� L2 cache.
*********************************************************************************************************/
static INT  armCacheV7MTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    if (stBytes >= CACHE_LOOP_OP_MAX_SIZE) {
        SCB_CleanDCache();                                              /*  DCACHE ȫ����д             */

    } else {
        SCB_CleanDCache_by_Addr(CACHE_ALIGN_ADDR(pvAdrs),
                                CACHE_ALIGN_SIZE(pvAdrs, stBytes));
    }

    SCB_InvalidateICache();                                             /*  ICACHE ȫ����Ч             */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archCacheV7MInit
** ��������: ��ʼ�� CACHE
** �䡡��  : pcacheop       CACHE ����������
**           uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  armCacheV7MInit (LW_CACHE_OP *pcacheop,
                               CACHE_MODE   uiInstruction,
                               CACHE_MODE   uiData,
                               CPCHAR       pcMachineName)
{
    REGISTER UINT32  ccsidr;
    REGISTER UINT32  sets;
    REGISTER UINT32  linesize;

    if (lib_strcmp(pcMachineName, ARM_MACHINE_M7) != 0) {
        return;
    }

    pcacheop->CACHEOP_ulOption = 0ul;

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;

    ccsidr   = SCB->CCSIDR;
    sets     = (UINT32)(CCSIDR_SETS(ccsidr));
    linesize = (UINT32)(CCSIDR_LINESIZE(ccsidr));

    uiArmV7MCacheLineSize = 1 << (linesize + 4);                        /*  Log2(X) = (linesize + 4)    */

    pcacheop->CACHEOP_iICacheLine = uiArmV7MCacheLineSize;
    pcacheop->CACHEOP_iDCacheLine = uiArmV7MCacheLineSize;

    pcacheop->CACHEOP_iICacheWaySize = sets * uiArmV7MCacheLineSize;
    pcacheop->CACHEOP_iDCacheWaySize = pcacheop->CACHEOP_iICacheWaySize;

    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7-M I-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7-M D-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    pcacheop->CACHEOP_pfuncEnable  = armCacheV7MEnable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV7MDisable;

    pcacheop->CACHEOP_pfuncLock    = armCacheV7MLock;
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV7MUnlock;

    pcacheop->CACHEOP_pfuncFlush          = armCacheV7MFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = LW_NULL;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV7MInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = LW_NULL;
    pcacheop->CACHEOP_pfuncClear          = armCacheV7MClear;
    pcacheop->CACHEOP_pfuncClearPage      = LW_NULL;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV7MTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = LW_NULL;

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
    pcacheop->CACHEOP_pfuncDmaMalloc      = armMpuV7MDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = armMpuV7MDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = armMpuV7MDmaFree;
#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
}
/*********************************************************************************************************
** ��������: archCacheV7MReset
** ��������: ��λ CACHE
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  armCacheV7MReset (CPCHAR  pcMachineName)
{
    if (lib_strcmp(pcMachineName, ARM_MACHINE_M7) != 0) {
        return;
    }

    SCB_DisableICache();
    SCB_DisableDCache();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
