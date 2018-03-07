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
** ��   ��   ��: ppcVfpAltivec.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 04 ��
**
** ��        ��: PowerPC ��ϵ�ܹ� ALTIVEC ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../ppcFpu.h"
#define  __SYLIXOS_PPC_HAVE_ALTIVEC 1
#include "arch/ppc/arch_604.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static PPC_FPU_OP   _G_fpuopVfpAltivec;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID     ppcVfpAltivecEnable(VOID);
extern VOID     ppcVfpAltivecDisable(VOID);
extern BOOL     ppcVfpAltivecIsEnable(VOID);
extern VOID     ppcVfpAltivecSave(PVOID  pvFpuCtx);
extern VOID     ppcVfpAltivecRestore(PVOID  pvFpuCtx);
/*********************************************************************************************************
  VSCR ��ض���
*********************************************************************************************************/
#define ALTIVEC_VSCR_CONFIG_WORD    3                                   /*  VSCR word with below bits   */
#define ALTIVEC_VSCR_NJ             0x00010000                          /*  Non-Java mode               */
#define ALTIVEC_VSCR_SAT            0x00000001                          /*  Vector Saturation           */
/*********************************************************************************************************
** ��������: ppcVfpAltivecCtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcVfpAltivecCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    UINT32         *puiValue;
    INT             i;

    fdprintf(iFd, "VRSAVE = 0x%08x VSCR = 0x%08x\n",
             pcpufpuCtx->VECCTX_uiVrsave,
             pcpufpuCtx->VECCTX_uiVscr[ALTIVEC_VSCR_CONFIG_WORD]);

    for (i = 0; i < ALTIVEC_REG_NR; i++) {
        puiValue = (UINT32 *)&pcpufpuCtx->VECCTX_regs[i];
        fdprintf(iFd, "AR%02d: 0x%08x_0x%08x_0x%08x_0x%08x\n",
                 i,
                 puiValue[0], puiValue[1], puiValue[2], puiValue[3]);
    }
#endif
}
/*********************************************************************************************************
** ��������: ppcVfpAltivecEnableTask
** ��������: ϵͳ���� undef �쳣ʱ, ʹ������� VFP
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcVfpAltivecEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_FPU_CTX  *pfpuctx;

    pregctx = &ptcbCur->TCB_archRegCtx;
    pregctx->REG_uiSrr1 |= ARCH_PPC_MSR_VEC;

    pfpuctx = &ptcbCur->TCB_fpuctxContext.FPUCTX_fpuctxContext;
    pfpuctx->VECCTX_uiVrsave = 0ul;
    pfpuctx->VECCTX_uiVscr[ALTIVEC_VSCR_CONFIG_WORD] = 0ul;
}
/*********************************************************************************************************
** ��������: ppcVfpAltivecPrimaryInit
** ��������: ��ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PPPC_FPU_OP  ppcVfpAltivecPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _G_fpuopVfpAltivec.PFPU_pfuncEnable     = ppcVfpAltivecEnable;
    _G_fpuopVfpAltivec.PFPU_pfuncDisable    = ppcVfpAltivecDisable;
    _G_fpuopVfpAltivec.PFPU_pfuncIsEnable   = ppcVfpAltivecIsEnable;
    _G_fpuopVfpAltivec.PFPU_pfuncSave       = ppcVfpAltivecSave;
    _G_fpuopVfpAltivec.PFPU_pfuncRestore    = ppcVfpAltivecRestore;
    _G_fpuopVfpAltivec.PFPU_pfuncCtxShow    = ppcVfpAltivecCtxShow;
    _G_fpuopVfpAltivec.PFPU_pfuncEnableTask = ppcVfpAltivecEnableTask;

    return  (&_G_fpuopVfpAltivec);
}
/*********************************************************************************************************
** ��������: ppcVfpAltivecSecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcVfpAltivecSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
