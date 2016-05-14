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
** 文   件   名: ppcMmuE500Tlb1.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 06 日
**
** 描        述: PowerPC E500 体系构架 MMU TLB1 函数库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/ppcSprE500.h"
#include "./ppcMmuE500Reg.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT                 _G_uiTlbSize = 0;                           /*  TLB 数组大小                */
static BOOL                 _G_bMas2MBit = LW_FALSE;                    /*  多核一致性                  */
static BOOL                 _G_bHasMAS7  = LW_FALSE;                    /*  是否有 MAS7 寄存器          */
static BOOL                 _G_bHasHID1  = LW_FALSE;                    /*  是否有 HID1 寄存器          */
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define MMU_MAS2_M          _G_bMas2MBit                                /*  是否多核一致性              */
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID    __ppcE500MmuTLB1Invalidate(VOID);
extern VOID    __ppcE500MmuTLB1InvalidateEA(addr_t  ulAddr);
/*********************************************************************************************************
** 函数名称: ppcE500MmuTLB1Init
** 功能描述: MMU TLB1 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcE500MmuTLB1Init (CPCHAR  pcMachineName)
{
    INTREG      iregInterLevel;
    TLBCFG_REG  uiTLB1CFG;

    /*
     * 多核一致性须使能 HID1[ABE] 位
     */
    MMU_MAS2_M = (LW_NCPUS > 1) ? 1 : 0;                                /*  多核一致性位设置            */

    if (lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0 ||
        lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) {
        _G_bHasMAS7 = LW_TRUE;
    } else {
        _G_bHasMAS7 = LW_FALSE;
    }

    if (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) {
        _G_bHasHID1 = LW_FALSE;
    } else {
        _G_bHasHID1 = LW_TRUE;
    }

    /*
     * 获得 TLB1 条目数
     */
    uiTLB1CFG.TLBCFG_uiValue = ppcE500MmuGetTLB1CFG();
    _G_uiTlbSize = uiTLB1CFG.TLBCFG_usNENTRY;
    _DebugFormat(__LOGMESSAGE_LEVEL, "MMU TLB1 size = %d.\r\n", _G_uiTlbSize);

    /*
     * 无效 TLB
     */
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    __ppcE500MmuTLB1Invalidate();
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuTLB1GlobalMap
** 功能描述: MMU TLB1 全局映射
** 输　入  : pdesc         映射关系数组
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcE500MmuTLB1GlobalMap (PLW_MMU_GLOBAL_DESC  pdesc)
{
    INTREG      iregInterLevel;
    MAS0_REG    uiMAS0;
    MAS1_REG    uiMAS1;
    MAS2_REG    uiMAS2;
    MAS3_REG    uiMAS3;
    UINT        i;

    if (pdesc) {

        for (i = 0; (i < _G_uiTlbSize) && pdesc->stSize; i++) {

            if (pdesc->ulFlag & LW_VMM_FLAG_VALID) {
                /*
                 * MAS0
                 */
                uiMAS0.MAS0_uiValue = 0;

                uiMAS0.MAS0_bTLBSEL = 1;
                uiMAS0.MAS0_ucESEL  = i;
                uiMAS0.MAS0_ucNV    = 0;

                /*
                 * MAS1
                 */
                uiMAS1.MAS1_uiValue = 0;

                uiMAS1.MAS1_bVaild  = LW_TRUE;
                uiMAS1.MAS1_bIPROT  = LW_TRUE;
                uiMAS1.MAS1_ucTID   = 0;
                uiMAS1.MAS1_bTS     = 0;

                if (pdesc->stSize <= 4 * LW_CFG_KB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_4K;

                } else if (pdesc->stSize <= 16 * LW_CFG_KB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_16K;

                } else if (pdesc->stSize <= 64 * LW_CFG_KB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_64K;

                } else if (pdesc->stSize <= 256 * LW_CFG_KB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_256K;

                } else if (pdesc->stSize <= 1 * LW_CFG_MB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_1M;

                } else if (pdesc->stSize <= 4 * LW_CFG_MB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_4M;

                } else if (pdesc->stSize <= 16 * LW_CFG_MB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_16M;

                } else if (pdesc->stSize <= 64 * LW_CFG_MB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_64M;

                } else if (pdesc->stSize <= 256 * LW_CFG_MB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_256M;

                } else if (pdesc->stSize <= 1 * LW_CFG_GB_SIZE) {
                    uiMAS1.MAS1_ucTSIZE = MMU_TRANS_SZ_1G;
                } else {
                    _BugHandle(pdesc->stSize > 1 * LW_CFG_GB_SIZE, LW_TRUE, "map size to large!\r\n");
                }

                /*
                 * MAS2
                 */
                uiMAS2.MAS2_uiValue = 0;

                uiMAS2.MAS2_uiEPN   = pdesc->ulVirtualAddr >> LW_CFG_VMM_PAGE_SHIFT;

                if (!(pdesc->ulFlag & LW_VMM_FLAG_CACHEABLE)) {
                    uiMAS2.MAS2_bUnCache = LW_TRUE;                     /*  不可 Cache                  */
                }

                if ((pdesc->ulFlag & LW_VMM_FLAG_CACHEABLE) &&
                    !(pdesc->ulFlag & LW_VMM_FLAG_BUFFERABLE)) {
                    uiMAS2.MAS2_bWT = LW_TRUE;                          /*  写穿透                      */
                }

                if (MMU_MAS2_M) {
                    uiMAS2.MAS2_bMemCoh = LW_TRUE;                      /*  多核一致性                  */
                }

                uiMAS2.MAS2_bGuarded      = LW_FALSE;
                uiMAS2.MAS2_bLittleEndian = LW_FALSE;

                /*
                 * MAS3
                 */
                uiMAS3.MAS3_uiValue = 0;

                uiMAS3.MAS3_uiRPN   = pdesc->ulPhysicalAddr >> LW_CFG_VMM_PAGE_SHIFT;

                uiMAS3.MAS3_bSuperRead = LW_TRUE;                       /*  可读                        */

                if (pdesc->ulFlag & LW_VMM_FLAG_WRITABLE) {
                    uiMAS3.MAS3_bSuperWrite = LW_TRUE;                  /*  可写                        */
                }

                if (pdesc->ulFlag & LW_VMM_FLAG_EXECABLE) {
                    uiMAS3.MAS3_bSuperExec = LW_TRUE;                   /*  可执行                      */
                }

                /*
                 * TLBWE
                 */
                iregInterLevel = KN_INT_DISABLE();                      /*  关闭中断                    */

                PPC_EXEC_INS("SYNC");

                ppcE500MmuSetMAS0(uiMAS0.MAS0_uiValue);
                ppcE500MmuSetMAS1(uiMAS1.MAS1_uiValue);
                ppcE500MmuSetMAS2(uiMAS2.MAS2_uiValue);
                ppcE500MmuSetMAS3(uiMAS3.MAS3_uiValue);

                if (_G_bHasMAS7) {
                    ppcE500MmuSetMAS7(0);
                }

                PPC_EXEC_INS("ISYNC");
                PPC_EXEC_INS("SYNC");
                PPC_EXEC_INS("TLBWE");
                PPC_EXEC_INS("ISYNC");

                KN_INT_ENABLE(iregInterLevel);                          /*  打开中断                    */
            }

            pdesc++;
        }

        _BugHandle(i == _G_uiTlbSize, LW_TRUE, "to many map desc!\r\n");

        /*
         * 无效剩余的 TLB1
         */
        for (; i < _G_uiTlbSize; i++) {
            iregInterLevel = KN_INT_DISABLE();                          /*  关闭中断                    */

            PPC_EXEC_INS("SYNC");

            ppcE500MmuSetMAS0(0);
            ppcE500MmuSetMAS1(0);
            ppcE500MmuSetMAS2(0);
            ppcE500MmuSetMAS3(0);

            if (_G_bHasMAS7) {
                ppcE500MmuSetMAS7(0);
            }

            PPC_EXEC_INS("ISYNC");
            PPC_EXEC_INS("SYNC");
            PPC_EXEC_INS("TLBWE");
            PPC_EXEC_INS("ISYNC");

            KN_INT_ENABLE(iregInterLevel);                              /*  打开中断                    */
        }

        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
