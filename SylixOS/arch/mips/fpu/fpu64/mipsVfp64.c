/**********************************************************************************************************
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
** ��   ��   ��: mipsVfp64.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 12 �� 01 ��
**
** ��        ��: MIPS ��ϵ�ܹ� VPU64 ֧��.
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
static MIPS_FPU_OP  _G_fpuopVfp64;
static INT          _G_iVfp64DNum;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern UINT32   mipsVfp64GetFIR(VOID);
extern VOID     mipsVfp64Enable(VOID);
extern VOID     mipsVfp64Disable(VOID);
extern BOOL     mipsVfp64IsEnable(VOID);
extern VOID     mipsVfp64Save16(PVOID pvFpuCtx);
extern VOID     mipsVfp64Restore16(PVOID pvFpuCtx);
extern VOID     mipsVfp64Save32(PVOID pvFpuCtx);
extern VOID     mipsVfp64Restore32(PVOID pvFpuCtx);
/*********************************************************************************************************
** ��������: mipsVfp64CtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsVfp64CtxShow (INT iFd, PVOID pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT             i;

    fdprintf(iFd, "FPCSR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpcsr);

    for (i = 0; i < _G_iVfp64DNum; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: mipsVfp64PrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PMIPS_FPU_OP  mipsVfp64PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    if (mipsVfp64GetFIR() == 0) {
        _G_iVfp64DNum = 32;
        _G_fpuopVfp64.MFPU_pfuncSave    = mipsVfp64Save16;
        _G_fpuopVfp64.MFPU_pfuncRestore = mipsVfp64Restore16;

    } else {
        _G_iVfp64DNum = 64;
        _G_fpuopVfp64.MFPU_pfuncSave    = mipsVfp64Save32;
        _G_fpuopVfp64.MFPU_pfuncRestore = mipsVfp64Restore32;
    }

    _G_fpuopVfp64.MFPU_pfuncEnable   = mipsVfp64Enable;
    _G_fpuopVfp64.MFPU_pfuncDisable  = mipsVfp64Disable;
    _G_fpuopVfp64.MFPU_pfuncIsEnable = mipsVfp64IsEnable;
    _G_fpuopVfp64.MFPU_pfuncCtxShow  = mipsVfp64CtxShow;

    return  (&_G_fpuopVfp64);
}
/*********************************************************************************************************
** ��������: mipsVfp64SecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsVfp64SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
