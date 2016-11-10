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
** ��   ��   ��: mipsVfp32.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 11 �� 17 ��
**
** ��        ��: MIPS ��ϵ�ܹ� VPU32 ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../mipsFpu.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static MIPS_FPU_OP  _G_fpuopVfp32;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID    mipsVfp32Init(VOID);
extern UINT32  mipsVfp32GetFIR(VOID);
extern VOID    mipsVfp32Enable(VOID);
extern VOID    mipsVfp32Disable(VOID);
extern BOOL    mipsVfp32IsEnable(VOID);
extern VOID    mipsVfp32Save(PVOID  pvFpuCtx);
extern VOID    mipsVfp32Restore(PVOID  pvFpuCtx);
/*********************************************************************************************************
** ��������: mipsVfp32CtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsVfp32CtxShow (INT iFd, PVOID pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT             i;

    fdprintf(iFd, "FCSR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpcsr);

    for (i = 0; i < (FP_NUM_DREGS * 2); i += 2) {
        fdprintf(iFd, "FP%d = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiReg[i]);
        fdprintf(iFd, "FP%d = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiReg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: mipsVfp32PrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PMIPS_FPU_OP  mipsVfp32PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    mipsVfp32Init();

    _G_fpuopVfp32.MFPU_pfuncEnable   = mipsVfp32Enable;
    _G_fpuopVfp32.MFPU_pfuncDisable  = mipsVfp32Disable;
    _G_fpuopVfp32.MFPU_pfuncIsEnable = mipsVfp32IsEnable;
    _G_fpuopVfp32.MFPU_pfuncCtxShow  = mipsVfp32CtxShow;
    _G_fpuopVfp32.MFPU_pfuncSave     = mipsVfp32Save;
    _G_fpuopVfp32.MFPU_pfuncRestore  = mipsVfp32Restore;

    return  (&_G_fpuopVfp32);
}
/*********************************************************************************************************
** ��������: mipsVfp32SecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsVfp32SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    mipsVfp32Init();
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
