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
** ��   ��   ��: ppcMmuE500Tlb1.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 06 ��
**
** ��        ��: PowerPC E500 ��ϵ���� MMU TLB1 ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/ppcSprE500.h"
#include "./ppcMmuE500Reg.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static UINT                 _G_uiTlbSize = 0;                           /*  TLB �����С                */
static BOOL                 _G_bMas2MBit = LW_FALSE;                    /*  ���һ����                  */
static BOOL                 _G_bHasMAS7  = LW_FALSE;                    /*  �Ƿ��� MAS7 �Ĵ���          */
static BOOL                 _G_bHasHID1  = LW_FALSE;                    /*  �Ƿ��� HID1 �Ĵ���          */
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define MMU_MAS2_M          _G_bMas2MBit                                /*  �Ƿ���һ����              */
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID    __ppcE500MmuTLB1Invalidate(VOID);
extern VOID    __ppcE500MmuTLB1InvalidateEA(addr_t  ulAddr);
/*********************************************************************************************************
** ��������: ppcE500MmuTLB1Init
** ��������: MMU TLB1 ��ʼ��
** �䡡��  : pcMachineName     ʹ�õĻ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ppcE500MmuTLB1Init (CPCHAR  pcMachineName)
{
    INTREG      iregInterLevel;
    TLBCFG_REG  uiTLB1CFG;

    /*
     * ���һ������ʹ�� HID1[ABE] λ
     */
    MMU_MAS2_M = (LW_NCPUS > 1) ? 1 : 0;                                /*  ���һ����λ����            */

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
     * ��� TLB1 ��Ŀ��
     */
    uiTLB1CFG.TLBCFG_uiValue = ppcE500MmuGetTLB1CFG();
    _G_uiTlbSize = uiTLB1CFG.TLBCFG_usNENTRY;
    _DebugFormat(__LOGMESSAGE_LEVEL, "MMU TLB1 size = %d.\r\n", _G_uiTlbSize);

    /*
     * ��Ч TLB
     */
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    __ppcE500MmuTLB1Invalidate();
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ppcE500MmuTLB1GlobalMap
** ��������: MMU TLB1 ȫ��ӳ��
** �䡡��  : pdesc         ӳ���ϵ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
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
                    uiMAS2.MAS2_bUnCache = LW_TRUE;                     /*  ���� Cache                  */
                }

                if ((pdesc->ulFlag & LW_VMM_FLAG_CACHEABLE) &&
                    !(pdesc->ulFlag & LW_VMM_FLAG_BUFFERABLE)) {
                    uiMAS2.MAS2_bWT = LW_TRUE;                          /*  д��͸                      */
                }

                if (MMU_MAS2_M) {
                    uiMAS2.MAS2_bMemCoh = LW_TRUE;                      /*  ���һ����                  */
                }

                uiMAS2.MAS2_bGuarded      = LW_FALSE;
                uiMAS2.MAS2_bLittleEndian = LW_FALSE;

                /*
                 * MAS3
                 */
                uiMAS3.MAS3_uiValue = 0;

                uiMAS3.MAS3_uiRPN   = pdesc->ulPhysicalAddr >> LW_CFG_VMM_PAGE_SHIFT;

                uiMAS3.MAS3_bSuperRead = LW_TRUE;                       /*  �ɶ�                        */

                if (pdesc->ulFlag & LW_VMM_FLAG_WRITABLE) {
                    uiMAS3.MAS3_bSuperWrite = LW_TRUE;                  /*  ��д                        */
                }

                if (pdesc->ulFlag & LW_VMM_FLAG_EXECABLE) {
                    uiMAS3.MAS3_bSuperExec = LW_TRUE;                   /*  ��ִ��                      */
                }

                /*
                 * TLBWE
                 */
                iregInterLevel = KN_INT_DISABLE();                      /*  �ر��ж�                    */

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

                KN_INT_ENABLE(iregInterLevel);                          /*  ���ж�                    */
            }

            pdesc++;
        }

        _BugHandle(i == _G_uiTlbSize, LW_TRUE, "to many map desc!\r\n");

        /*
         * ��Чʣ��� TLB1
         */
        for (; i < _G_uiTlbSize; i++) {
            iregInterLevel = KN_INT_DISABLE();                          /*  �ر��ж�                    */

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

            KN_INT_ENABLE(iregInterLevel);                              /*  ���ж�                    */
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
