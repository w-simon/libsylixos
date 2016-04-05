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
** ��   ��   ��: ppcFpu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "ppcFpu.h"
#include "vfpnone/ppcVfpNone.h"
#include "vfp/ppcVfp.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit;
static PPPC_FPU_OP      _G_pfpuop;
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
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s FPU pri-core initialization.\r\n", 
                 pcMachineName, pcFpuName);

    if (lib_strcmp(pcFpuName, PPC_FPU_NONE) == 0) {                     /*  ѡ�� VFP �ܹ�               */
        _G_pfpuop = ppcVfpNonePrimaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, PPC_FPU_VFP) == 0) {
        _G_pfpuop = ppcVfpPrimaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }
    
    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
    
    PPC_VFP_ENABLE(_G_pfpuop);
    
    PPC_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);
    
    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpscr = 0x00000000;     /*  Do not enable FPU           */

    PPC_VFP_DISABLE(_G_pfpuop);
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
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s FPU sec-core initialization.\r\n", 
                 pcMachineName, pcFpuName);

    if (lib_strcmp(pcFpuName, PPC_FPU_NONE) == 0) {                     /*  ѡ�� VFP �ܹ�               */
        ppcVfpNoneSecondaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, PPC_FPU_VFP) == 0) {
        ppcVfpSecondaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
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
    PPC_VFP_ENABLE(_G_pfpuop);
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
    PPC_VFP_DISABLE(_G_pfpuop);
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
    PPC_VFP_SAVE(_G_pfpuop, pvFpuCtx);
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
    PPC_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
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
    PPC_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
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
    ARCH_FPU_CTX  *pfpuctx;
    ARCH_REG_T     regSp;

    if (PPC_VFP_ISENABLE(_G_pfpuop)) {                                  /*  �����ǰ������ FPU ʹ��     */
        return  (PX_ERROR);                                             /*  ��δ����ָ���� FPU �޹�     */
    }
    
    pregctx = archTaskRegsGet(ptcbCur->TCB_pstkStackNow, &regSp);
    pregctx->REG_uiSrr1 |= ARCH_PPC_MSR_FP;

    pfpuctx = &ptcbCur->TCB_fpuctxContext.FPUCTX_fpuctxContext;
    pfpuctx->FPUCTX_uiFpscr     = ARCH_PPC_FPSCR_INIT;
    pfpuctx->FPUCTX_uiFpscrCopy = ARCH_PPC_FPSCR_INIT;

    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;
    PPC_VFP_ENABLE(_G_pfpuop);                                          /*  ʹ�� FPU                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
