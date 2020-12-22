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
** ��   ��   ��: clockGate.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 27 ��
**
** ��        ��: ��ѡһʱ�����������Ƹ�ʱ��ѡ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "clock.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static UINT8  __clockMuxParentGet(PLW_CLOCK  pclk);
static INT    __clockMuxParentSet(PLW_CLOCK  pclk, UINT8  ucIndex);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsMux = {
    .clockParentGet = __clockMuxParentGet,
    .clockParentSet = __clockMuxParentSet,
};
/*********************************************************************************************************
** ��������: __clockMuxParentGet
** ��������: ��õ�ǰѡ�еĸ�ʱ�����
** �䡡��  : pclk           ʱ���豸
** �䡡��  : ��ǰѡ�еĸ�ʱ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8  __clockMuxParentGet (PLW_CLOCK  pclk)
{
    PLW_CLOCK_MUX        pclkmux      = __CLK_TO_CLK_MUX(pclk);
    PLW_CLOCK_MUX_TABLE  pclkmuxtable = pclkmux->CLKM_pclktable;
    UINT                 uiNumParents = pclk->CLK_uiNumParents;
    UINT                 uiVal        = pclkmux->CLKM_pfuncValGet();
    UINT                 i;

    uiVal &= pclkmux->CLKM_uiMask;
    uiVal  = uiVal >> pclkmux->CLKM_uiShift;

    for (i = 0; i < uiNumParents; i++) {
        if (pclkmuxtable[i].CLKMT_uiVal == uiVal) {
            return  (pclkmuxtable[i].CLKMT_uiSource);
        }
    }

    return  (0);
}
/*********************************************************************************************************
** ��������: __clockMuxParentSet
** ��������: ���õ�ǰ�ĸ�ʱ�����
** �䡡��  : pclk             ʱ���豸
**           ucIndex          ��Ӧ�ĸ�ʱ�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __clockMuxParentSet (PLW_CLOCK  pclk, UINT8  ucIndex)
{
    PLW_CLOCK_MUX        pclkmux      = __CLK_TO_CLK_MUX(pclk);
    PLW_CLOCK_MUX_TABLE  pclkmuxtable = pclkmux->CLKM_pclktable;
    UINT                 uiNumParents = pclk->CLK_uiNumParents;
    UINT                 uiVal        = pclkmux->CLKM_pfuncValGet();
    UINT                 uiRegVal;
    UINT                 i;

    for (i = 0; i < uiNumParents; i++) {
        if (pclkmuxtable[i].CLKMT_uiSource == ucIndex) {
            uiVal = pclkmuxtable[i].CLKMT_uiVal;
        }
    }

    uiVal     = uiVal << pclkmux->CLKM_uiShift;
    uiRegVal  = pclkmux->CLKM_pfuncValGet();
    uiRegVal &= ~pclkmux->CLKM_uiMask;
    uiRegVal |= uiVal;

    return  (pclkmux->CLKM_pfuncValSet(uiVal));
}
/*********************************************************************************************************
** ��������: API_ClockDividerRegister
** ��������: ע��ɱ��Ƶ��ʱ��
** �䡡��  : pcName            ʱ������
**           pcParentName      ��ʱ������
**           ulFlags           ��ʼ����ʶ
**           pclkmuxtable      ��Ƶ��
**           uiMask            ����
**           uiShift           ��λ
**           pfuncValGet       �Ĵ���ֵ��ȡ����
**           pfuncValSet       �Ĵ���ֵ���ú���
** �䡡��  : �ɱ��Ƶ��ʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockMuxRegister (CPCHAR               pcName,
                                 CHAR               **pcParentName,
                                 ULONG                ulFlags,
                                 UINT                 uiParentNum,
                                 PLW_CLOCK_MUX_TABLE  pclkmuxtable,
                                 UINT                 uiMask,
                                 UINT                 uiShift,
                                 UINTFUNCPTR          pfuncValGet,
                                 FUNCPTR              pfuncValSet)
{
    PLW_CLOCK_MUX       pclkmux;
    PLW_CLOCK           pclk;
    INT                 iRet;

    pclkmux = __SHEAP_ZALLOC(sizeof(LW_CLOCK_MUX));
    if (!pclkmux) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkmux->CLKM_clk,
                                pcName,
                                &_G_clkopsMux,
                                ulFlags,
                                pcParentName,
                                uiParentNum);
    if (iRet) {
        __SHEAP_FREE(pclkmux);
        return  (LW_NULL);
    }

    pclk = &pclkmux->CLKM_clk;
    pclkmux->CLKM_pclktable   = pclkmuxtable;
    pclkmux->CLKM_uiMask      = uiMask;
    pclkmux->CLKM_uiShift     = uiShift;
    pclkmux->CLKM_pfuncValGet = pfuncValGet;
    pclkmux->CLKM_pfuncValSet = pfuncValSet;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkmux);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
