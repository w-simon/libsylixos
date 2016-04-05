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
** ��   ��   ��: ppcVfp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 21 ��
**
** ��        ��: PowerPC ��ϵ�ܹ� VPU ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../ppcFpu.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static PPC_FPU_OP   _G_fpuopVfp;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID     ppcVfpEnable(VOID);
extern VOID     ppcVfpDisable(VOID);
extern BOOL     ppcVfpIsEnable(VOID);
extern VOID     ppcVfpSave(PVOID pvFpuCtx);
extern VOID     ppcVfpRestore(PVOID pvFpuCtx);
/*********************************************************************************************************
** ��������: ppcVfpCtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcVfpCtxShow (INT iFd, PVOID pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT             i;

    fdprintf(iFd, "FPSCR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpscr);

    for (i = 0; i < FP_DREG_NR; i += 2) {
        fdprintf(iFd,
                 "FPR%02d = %lf, FPR%02d = %lf\n",
                 i,
                 pcpufpuCtx->FPUCTX_dfDreg[i],
                 i + 1,
                 pcpufpuCtx->FPUCTX_dfDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: ppcVfpPrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PPPC_FPU_OP  ppcVfpPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _G_fpuopVfp.PFPU_pfuncSave     = ppcVfpSave;
    _G_fpuopVfp.PFPU_pfuncRestore  = ppcVfpRestore;
    _G_fpuopVfp.PFPU_pfuncEnable   = ppcVfpEnable;
    _G_fpuopVfp.PFPU_pfuncDisable  = ppcVfpDisable;
    _G_fpuopVfp.PFPU_pfuncIsEnable = ppcVfpIsEnable;
    _G_fpuopVfp.PFPU_pfuncCtxShow  = ppcVfpCtxShow;

    return  (&_G_fpuopVfp);
}
/*********************************************************************************************************
** ��������: ppcVfpSecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcVfpSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
