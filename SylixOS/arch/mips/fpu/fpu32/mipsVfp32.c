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
static INT          _G_iVfp32DNum;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern UINT32   mipsVfp32GetFIR(VOID);
extern VOID     mipsVfp32Enable(VOID);
extern VOID     mipsVfp32Disable(VOID);
extern BOOL     mipsVfp32IsEnable(VOID);
extern VOID     mipsVfp32Save16(PVOID pvFpuCtx);
extern VOID     mipsVfp32Restore16(PVOID pvFpuCtx);
extern VOID     mipsVfp32Save32(PVOID pvFpuCtx);
extern VOID     mipsVfp32Restore32(PVOID pvFpuCtx);
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

    fdprintf(iFd, "FPCSR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpcsr);

    for (i = 0; i < _G_iVfp32DNum; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
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
    if (mipsVfp32GetFIR() == 0) {
        _G_iVfp32DNum = 32;
        _G_fpuopVfp32.MFPU_pfuncSave    = mipsVfp32Save16;
        _G_fpuopVfp32.MFPU_pfuncRestore = mipsVfp32Restore16;

    } else {
        _G_iVfp32DNum = 64;
        _G_fpuopVfp32.MFPU_pfuncSave    = mipsVfp32Save32;
        _G_fpuopVfp32.MFPU_pfuncRestore = mipsVfp32Restore32;
    }

    _G_fpuopVfp32.MFPU_pfuncEnable   = mipsVfp32Enable;
    _G_fpuopVfp32.MFPU_pfuncDisable  = mipsVfp32Disable;
    _G_fpuopVfp32.MFPU_pfuncIsEnable = mipsVfp32IsEnable;
    _G_fpuopVfp32.MFPU_pfuncCtxShow  = mipsVfp32CtxShow;

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
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
