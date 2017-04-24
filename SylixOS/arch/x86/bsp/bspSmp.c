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
** ��   ��   ��: bspSmp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 7 �� 31 ��
**
** ��        ��: ��������ҪΪ SylixOS �ṩ�Ĺ���֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86Topology.h"
#include "arch/x86/mpcore/x86MpCore.h"
/*********************************************************************************************************
  �����ؽӿ�
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define AP_BOOT_ENTRY_ADDR          0x7000                              /*  AP ������ַ(ʵģʽ��ַ)   */
#define AP_BOOT_STACK_SIZE          (32 * LW_CFG_KB_SIZE)               /*  AP ��ʼ�����̶�ջ��С       */
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static const UINT8  _G_ucX86ApEntryCode[] = {                           /*  AP ��������               */
    0xfa, 0x31, 0xc0, 0x8e, 0xd8, 0x8e,
    0xc0, 0x8e, 0xd0, 0x0f, 0x01, 0x16,
    0x68, 0x70, 0x0f, 0x20, 0xc0, 0x66,
    0x83, 0xc8, 0x01, 0x0f, 0x22, 0xc0,
    0x66, 0xea, 0x20, 0x70, 0x00, 0x00,
    0x08, 0x00, 0x66, 0xb8, 0x10, 0x00,
    0x8e, 0xd8, 0x8e, 0xc0, 0x8e, 0xd0,
    0x66, 0xb8, 0x00, 0x00, 0x8e, 0xe0,
    0x8e, 0xe8, 0x8b, 0x25, 0xfc, 0x6f,
    0x00, 0x00, 0xff, 0x15, 0xf8, 0x6f,
    0x00, 0x00, 0x66, 0xb8, 0x00, 0x8a,
    0x66, 0x89, 0xc2, 0x66, 0xef, 0x66,
    0xb8, 0xe0, 0x8a, 0x66, 0xef, 0xeb,
    0xfe, 0x90, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x92,
    0xcf, 0x00, 0x17, 0x00, 0x50, 0x70,
    0x00, 0x00, 0x00
};

static SPINLOCKTYPE _G_slX86ApLock = 0;                                 /*  AP ����������               */
/*********************************************************************************************************
** ��������: x86CpuIpiStartSecondary
** ��������: ���� AP Processor
** �䡡��  : ucLocalApicID     Local APIC ID
**           uiEntryAddr       ʵģʽ������ַ
**           uiTimes           �ط�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  x86CpuIpiStartSecondary (UINT8  ucLocalApicId, addr_t  ulEntryAddr, UINT32  uiTimes)
{
    LOAPIC_IPI_INFO  ipiInfo;
    INT              i;

    /*
     * BSP sends AP an INIT IPI
     */

    /*
     * INIT Level De-assert not supported in the Pentium 4, Xeon processors
     */

    /*
     * Do an INIT IPI: assert RESET
     */
    ipiInfo.ICR.ICR_uiApicId     = ucLocalApicId;
    ipiInfo.ICR.ICR_uiShortHand  = 0;
    ipiInfo.ICR.ICR_uiTrigger    = 0;
    ipiInfo.ICR.ICR_uiLevel      = 1;
    ipiInfo.ICR.ICR_uiDestMode   = 0;
    ipiInfo.ICR.ICR_uiDeliMode   = 5;
    ipiInfo.ICR.ICR_uiVectorNo   = 0;

    if (x86LocalApicSendIpiRaw(ipiInfo.IPI_uiAll) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * BSP delay 10msec
     */
    for (i = 0; i < 15000; i++) {                                       /*  15000*720 ~= 10.8 msec      */
        bspDelay720Ns();                                                /*  720ns                       */
    }

    while (uiTimes-- > 0) {
        /*
         * BSP sends AP a STARTUP IPI again
         */
        ipiInfo.ICR.ICR_uiApicId     = ucLocalApicId;
        ipiInfo.ICR.ICR_uiShortHand  = 0;
        ipiInfo.ICR.ICR_uiTrigger    = 0;
        ipiInfo.ICR.ICR_uiLevel      = 0;
        ipiInfo.ICR.ICR_uiDestMode   = 0;
        ipiInfo.ICR.ICR_uiDeliMode   = 6;
        ipiInfo.ICR.ICR_uiVectorNo   = (UINT8)(ulEntryAddr >> 12);

        if (x86LocalApicSendIpiRaw(ipiInfo.IPI_uiAll) != ERROR_NONE) {
            return  (PX_ERROR);
        }

        /*
         * BSP delays 200usec again
         */
        for (i = 0; i < 300; i++) {                                     /*  300*720 ~= 216usec          */
            bspDelay720Ns();                                            /*  720ns                       */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: bspCpuUp
** ��������: ����һ�� CPU
** ��  ��  : ulCPUId      Ŀ�� CPU
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID   bspCpuUp (ULONG  ulCPUId)
{
    UINT32     *puiEntryAddr;
    PLW_STACK   pstkStack;
    UINT32      uiOldResetVector;                                       /*  Warm reset vector           */
    UINT8       uiOldShutdownCode;                                      /*  CMOS shutdown code          */

    x86SpinLock(&_G_slX86ApLock);

    pstkStack = sys_malloc(AP_BOOT_STACK_SIZE);
    if (pstkStack == LW_NULL) {
        _PrintFormat("Can't start CPU#%d, system low memory!\r\n", ulCPUId);
        return;
    }
    puiEntryAddr = (UINT32 *)AP_BOOT_ENTRY_ADDR;

    lib_memcpy(puiEntryAddr, _G_ucX86ApEntryCode, (size_t)sizeof(_G_ucX86ApEntryCode));

    *(puiEntryAddr - 1) = ((UINT32)pstkStack) + AP_BOOT_STACK_SIZE - 4;
    *(puiEntryAddr - 2) = (UINT32)bspSecondaryInit;

    /*
     * Set the AP entry point address in WARM_REST_VECTOR
     */
    uiOldResetVector = *(volatile UINT32 *)WARM_RESET_VECTOR;

    /*
     * Selects Shutdown Status Register
     */
    out8(BIOS_SHUTDOWN_STATUS, RTC_INDEX);
    /*
     * Get BIOS Shutdown code
     */
    uiOldShutdownCode = in8(RTC_DATA);

    *(volatile UINT16 *)(WARM_RESET_VECTOR + 0) = 0;
    *(volatile UINT16 *)(WARM_RESET_VECTOR + 2) = (((addr_t)puiEntryAddr) >> 4);

    /*
     * Initialze the BIOS shutdown code to be 0xA
     */
    /*
     * Selects Shutdown Status Register
     */
    out8(BIOS_SHUTDOWN_STATUS, RTC_INDEX);
    /*
     * Set BIOS Shutdown code to 0xa
     */
    out8(0xa, RTC_DATA);

    X86_WBINVD();

    x86CpuIpiStartSecondary(X86_LOGICID_TO_APICID(ulCPUId), (addr_t)puiEntryAddr, 2);

    /*
     * Restore the WARM_REST_VECTOR and the BIOS shutdown code
     */
    *(volatile UINT32 *)WARM_RESET_VECTOR = uiOldResetVector;
    out8(BIOS_SHUTDOWN_STATUS, RTC_INDEX);                              /*  Shutdown Status Reg         */
    out8(uiOldShutdownCode,    RTC_DATA);                               /*  Set BIOS Shutdown code      */

    X86_WBINVD();
}
/*********************************************************************************************************
** ��������: bspCpuUpDone
** ��������: һ�� CPU �������
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspCpuUpDone (VOID)
{
    x86SpinUnlock(&_G_slX86ApLock);
}
/*********************************************************************************************************
** ��������: bspSecondaryCpusUp
** ��������: �������е� Secondary CPU
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspSecondaryCpusUp (VOID)
{
    UINT8  ucLocalApicId;
    ULONG  ulCPUId;

    if (LW_NCPUS <= 1) {
        return;
    }

    if (X86_HAS_SMT) {                                                  /*  ���ڳ��߳�                  */
        LW_CPU_FOREACH_EXCEPT(ulCPUId, 0) {
            ucLocalApicId = X86_LOGICID_TO_APICID(ulCPUId);
            if (X86_APICID_PRESEND(ucLocalApicId)) {
                if (X86_APICID_IS_HT(ucLocalApicId)) {                  /*  ���������������            */
                    continue;
                }
                API_CpuUp(ulCPUId);
            }
        }

        LW_CPU_FOREACH_EXCEPT(ulCPUId, 0) {
            ucLocalApicId = X86_LOGICID_TO_APICID(ulCPUId);
            if (X86_APICID_PRESEND(ucLocalApicId)) {
                if (X86_APICID_IS_HT(ucLocalApicId)) {                  /*  �����������߼���            */
                    API_CpuUp(ulCPUId);
                }
            }
        }

    } else {                                                            /*  �����ڳ��߳�                */
        LW_CPU_FOREACH_EXCEPT(ulCPUId, 0) {
            ucLocalApicId = X86_LOGICID_TO_APICID(ulCPUId);
            if (X86_APICID_PRESEND(ucLocalApicId)) {
                API_CpuUp(ulCPUId);
            }
        }
    }
}
/*********************************************************************************************************
** ��������: bspCpuLogic2Physical
** ��������: ����߼� CPU ������ ID
** ��  ��  : ulCPUId      Ŀ�� CPU
** ��  ��  : ���� ID
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CPU_ARCH_SMT > 0

LW_WEAK ULONG  bspCpuLogic2Physical (ULONG  ulCPUId)
{
    return  (X86_LOGICID_TO_PHYID(ulCPUId));
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
/*********************************************************************************************************
** ��������: bspCpuDown
** ��������: ֹͣһ�� CPU
** ��  ��  : ulCPUId      Ŀ�� CPU
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

LW_WEAK VOID   bspCpuDown (ULONG  ulCPUId)
{
    KN_INT_DISABLE();

    X86_WBINVD();

    while (1) {
        X86_HLT();
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: bspCpuIpiVectorInstall
** ��������: ��װ IPI ����
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspCpuIpiVectorInstall (VOID)
{
    ULONG  ulCPUId;

    if (LW_NCPUS <= 1) {
        return;
    }

    ulCPUId = LW_CPU_GET_CUR_ID();
    API_InterVectorIpi(ulCPUId, X86_IPI_VECTOR(ulCPUId));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
