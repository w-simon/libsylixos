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
** ��   ��   ��: mipsFpu.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 11 �� 17 ��
**
** ��        ��: MIPS ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "mipsFpu.h"
#include "fpu32/mipsVfp32.h"
#include "vfpnone/mipsVfpNone.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit;
static PMIPS_FPU_OP     _G_pfpuop;
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern UINT32  mipsVfp32GetFIR(VOID);
/*********************************************************************************************************
** ��������: archFpuPrimaryInit
** ��������: ���� Fpu ��������ʼ��
** �䡡��  : pcMachineName ��������
**           pcFpuName     fpu ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    UINT32  uiConfig1;

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s FPU pri-core initialization.\r\n",
                 pcMachineName, pcFpuName);

    uiConfig1 = mipsCp0Config1Read();
    if (uiConfig1 & M_Config1FP) {
        if (lib_strcmp(pcFpuName, MIPS_FPU_NONE) == 0) {                /*  ѡ�� VFP �ܹ�               */
            _G_pfpuop = mipsVfpNonePrimaryInit(pcMachineName, pcFpuName);

        } else if (lib_strcmp(pcFpuName, MIPS_FPU_VFP32) == 0) {
            _G_pfpuop = mipsVfp32PrimaryInit(pcMachineName, pcFpuName);

        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
            return;
        }
    } else {
        _G_pfpuop = mipsVfpNonePrimaryInit(pcMachineName, MIPS_FPU_NONE);
    }

    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));

    MIPS_VFP_ENABLE(_G_pfpuop);

    MIPS_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);

    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpcsr = 0x00000000;     /*  Set FZ bit in VFP           */

    MIPS_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** ��������: archFpuSecondaryInit
** ��������: �Ӻ� Fpu ��������ʼ��
** �䡡��  : pcMachineName ��������
**           pcFpuName     fpu ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  archFpuSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    UINT32  uiConfig1;

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s FPU sec-core initialization.\r\n",
                 pcMachineName, pcFpuName);

    uiConfig1 = mipsCp0Config1Read();
    if (uiConfig1 & M_Config1FP) {
        if (lib_strcmp(pcFpuName, MIPS_FPU_NONE) == 0) {                /*  ѡ�� VFP �ܹ�               */
            mipsVfpNoneSecondaryInit(pcMachineName, pcFpuName);

        } else if (lib_strcmp(pcFpuName, MIPS_FPU_VFP32) == 0) {
            mipsVfp32SecondaryInit(pcMachineName, pcFpuName);

        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
            return;
        }
    } else {
        mipsVfpNoneSecondaryInit(pcMachineName, MIPS_FPU_NONE);
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: OSFpuInitCtx
** ��������: ��ʼ��һ�� Fpu �����Ŀ��ƿ� (���ﲢû��ʹ�� FPU)
** �䡡��  : pvFpuCtx   FPU ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuCtxInit (PVOID pvFpuCtx)
{
    lib_memcpy(pvFpuCtx, &_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
}
/*********************************************************************************************************
** ��������: archFpuEnable
** ��������: ʹ�� FPU.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuEnable (VOID)
{
    MIPS_VFP_ENABLE(_G_pfpuop);
}
/*********************************************************************************************************
** ��������: archFpuDisable
** ��������: ���� FPU.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuDisable (VOID)
{
    MIPS_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** ��������: archFpuSave
** ��������: ���� FPU ������.
** �䡡��  : pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuSave (PVOID pvFpuCtx)
{
    MIPS_VFP_SAVE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuRestore
** ��������: �ظ� FPU ������.
** �䡡��  : pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuRestore (PVOID pvFpuCtx)
{
    MIPS_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuCtxShow
** ��������: ��ʾ FPU ������.
** �䡡��  : iFd       �ļ�������
**           pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuCtxShow (INT  iFd, PVOID pvFpuCtx)
{
    MIPS_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuUndHandle
** ��������: ϵͳ���� undef �쳣ʱ, ���ô˺���.
**           ֻ��ĳ����������ж�, ����ʹ�ø�������ʱ (�����е���������ָ������쳣)
**           ��ʱ�ſ��Դ򿪸��������.
** �䡡��  : ptcbCur   ��ǰ���� TCB
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archFpuUndHandle (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_REG_T     regSp;
    UINT32         uiConfig1;

    if (MIPS_VFP_ISENABLE(_G_pfpuop)) {                                 /*  �����ǰ������ FPU ʹ��     */
        return  (PX_ERROR);                                             /*  ��δ����ָ���� FPU �޹�     */
    }

    uiConfig1 = mipsCp0Config1Read();
    if (uiConfig1 & M_Config1FP) {                                      /*  �� FPU                      */
        pregctx = archTaskRegsGet(ptcbCur->TCB_pstkStackNow, &regSp);
        pregctx->REG_uiCP0Status |= M_StatusCU1;
    
	} else {
        return  (PX_ERROR);                                             /*  û�� FPU                    */
    }

    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;
    MIPS_VFP_ENABLE(_G_pfpuop);                                         /*  ʹ�� FPU                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
