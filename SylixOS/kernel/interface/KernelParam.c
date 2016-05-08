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
** ��   ��   ��: KernelParam.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 08 �� 08 ��
**
** ��        ��: ����ϵͳ�ں��������������ļ���
**
** BUG:
2014/09.09  ncpus ȡֵ��ΧΪ [1 ~ LW_CFG_MAX_PROCESSORS].
*********************************************************************************************************/
#define  __KERNEL_NCPUS_SET
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "../SylixOS/kernel/vmm/virPage.h"
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  �ں˲���
*********************************************************************************************************/
static CHAR     _K_cKernelStartParam[256];
/*********************************************************************************************************
** ��������: API_KernelStartParam
** ��������: ϵͳ�ں���������
** �䡡��  : pcParam       ��������, ���Կո�ֿ���һ���ַ����б�ͨ������������ʽ:
                           ncpus=1      CPU ����
                           dlog=no      DEBUG LOG ��Ϣ��ӡ
                           derror=yes   DEBUG ERROR ��Ϣ��ӡ
                           kfpu=no      �ں�̬�Ը���֧�� (�Ƽ�Ϊ no)
                           heapchk=yes  ��ջԽ����
                           varea=*      * ��ʾ�����ڴ���ʼ��, Ĭ��Ϊ 0xC000_0000
                           vsize=*      * ��ʾ�����ڴ��С, Ĭ��Ϊ 1GB
                           hz=100       ϵͳ tick Ƶ��, Ĭ��Ϊ 100 (�Ƽ� 100 ~ 10000 �м�)
                           hhz=100      ���ٶ�ʱ��Ƶ��, Ĭ���� hz ��ͬ (�� BSP ֧��)
                           irate=5      Ӧ�ö�ʱ���ֱ���, Ĭ��Ϊ 5 �� tick. (�Ƽ� 1 ~ 10 �м�)
                           hpsec=1      �Ȳ��ѭ�������ʱ��, ��λ: �� (�Ƽ� 1 ~ 5 ��)
                           bugreboot=no �ں�̽�⵽ bug ʱ�Ƿ��Զ�����.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                       API ����
*********************************************************************************************************/
LW_API
ULONG  API_KernelStartParam (CPCHAR  pcParam)
{
    CHAR        cParamBuffer[256];                                      /*  �������Ȳ��ó��� 256 �ֽ�   */
    PCHAR       pcDelim = " ";
    PCHAR       pcLast;
    PCHAR       pcTok;
    
#if LW_CFG_VMM_EN > 0
    PLW_MMU_VIRTUAL_DESC    pvirdesc = __vmmVirtualDesc();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    
    if (LW_SYS_STATUS_IS_RUNNING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel is already start.\r\n");
        _ErrorHandle(ERROR_KERNEL_RUNNING);
        return  (ERROR_KERNEL_RUNNING);
    }
    
    lib_strlcpy(cParamBuffer,         pcParam, sizeof(cParamBuffer));
    lib_strlcpy(_K_cKernelStartParam, pcParam, sizeof(_K_cKernelStartParam));

    pcTok = lib_strtok_r(cParamBuffer, pcDelim, &pcLast);
    while (pcTok) {
        if (lib_strncmp(pcTok, "ncpus=", 6) == 0) {                     /*  CPU ����                    */
            INT     iCpus = lib_atoi(&pcTok[6]);
            if (iCpus > 0 && iCpus <= LW_CFG_MAX_PROCESSORS) {
                _K_ulNCpus = (ULONG)iCpus;
            }
            
#if LW_CFG_LOGMESSAGE_EN > 0
        } else if (lib_strncmp(pcTok, "kdlog=", 6) == 0) {              /*  �Ƿ�ʹ���ں� log ��ӡ       */
            if (pcTok[6] == 'n') {
                _K_pfuncKernelDebugLog = LW_NULL;
            } else {
                _K_pfuncKernelDebugLog = bspDebugMsg;
            }
#endif                                                                  /*  LW_CFG_LOGMESSAGE_EN > 0    */

#if LW_CFG_ERRORMESSAGE_EN > 0
        } else if (lib_strncmp(pcTok, "kderror=", 8) == 0) {            /*  �Ƿ�ʹ���ں˴����ӡ        */
            if (pcTok[8] == 'n') {
                _K_pfuncKernelDebugError = LW_NULL;
            } else {
                _K_pfuncKernelDebugError = bspDebugMsg;
            }
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN > 0  */
        
        } else if (lib_strncmp(pcTok, "kfpu=", 5) == 0) {               /*  �Ƿ�ʹ���ں˸���֧��        */
            if (pcTok[5] == 'n') {
                LW_KERN_FPU_EN_SET(LW_FALSE);
            } else {
                LW_KERN_FPU_EN_SET(LW_TRUE);
            }
        
        } else if (lib_strncmp(pcTok, "bugreboot=", 5) == 0) {          /*  ̽�⵽ bug ʱ�Ƿ��Զ�����   */
            if (pcTok[10] == 'n') {
                LW_KERN_BUG_REBOOT_EN_SET(LW_FALSE);
            } else {
                LW_KERN_BUG_REBOOT_EN_SET(LW_TRUE);
            }
            
        } else if (lib_strncmp(pcTok, "heapchk=", 8) == 0) {            /*  �Ƿ���ж��ڴ�Խ����      */
            if (pcTok[8] == 'n') {
                _K_bHeapCrossBorderEn = LW_FALSE;
            } else {
                _K_bHeapCrossBorderEn = LW_TRUE;
            }
        
        } else if (lib_strncmp(pcTok, "hz=", 3) == 0) {                 /*  tick Ƶ��                   */
            ULONG   ulHz = (ULONG)lib_atol(&pcTok[3]);
            if (ulHz >= 100 && ulHz <= 10000) {                         /*  10ms ~ 100us                */
                LW_TICK_HZ = ulHz;
                LW_NSEC_PER_TICK = __TIMEVAL_NSEC_MAX / ulHz;
            }
        
        } else if (lib_strncmp(pcTok, "hhz=", 4) == 0) {                /*  �߶ȶ�ʱ��Ƶ��              */
            ULONG   ulHz = (ULONG)lib_atol(&pcTok[4]);
            if (ulHz >= 100 && ulHz <= 100000) {                        /*  10ms ~ 10us                 */
                LW_HTIMER_HZ = ulHz;
            }
        
        } else if (lib_strncmp(pcTok, "irate=", 6) == 0) {              /*  Ӧ�ö�ʱ���ֱ���            */
            ULONG   ulRate = (ULONG)lib_atol(&pcTok[6]);
            if (ulRate >= 1 && ulRate <= 10) {                          /*  1 ~ 10 ticks                */
                LW_ITIMER_RATE = ulRate;
            }
        
        } else if (lib_strncmp(pcTok, "hpsec=", 6) == 0) {              /*  �Ȳ��ѭ���������          */
            ULONG   ulSec = (ULONG)lib_atol(&pcTok[6]);
            if (ulSec >= 1 && ulSec <= 10) {                            /*  1 ~ 10 ticks                */
                LW_HOTPLUG_SEC = ulSec;
            }
        }
        
#if LW_CFG_VMM_EN > 0
          else if (lib_strncmp(pcTok, "varea=", 6) == 0) {              /*  �����ڴ���ʼ��              */
            pvirdesc->ulVirtualSwitch = lib_strtoul(&pcTok[6], LW_NULL, 16);
            pvirdesc->ulVirtualStart  = pvirdesc->ulVirtualSwitch 
                                      + LW_CFG_VMM_PAGE_SIZE;
          
        } else if (lib_strncmp(pcTok, "vsize=", 6) == 0) {              /*  �����ڴ��С                */
            pvirdesc->stSize = (size_t)lib_strtoul(&pcTok[6], LW_NULL, 16);
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
          
#ifdef __ARCH_KERNEL_PARAM
          else {
            __ARCH_KERNEL_PARAM(pcTok);                                 /*  ��ϵ�ṹ��ز���            */
        }
#endif                                                                  /*  __ARCH_KERNEL_PARAM         */
          
        pcTok = lib_strtok_r(LW_NULL, pcDelim, &pcLast);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_KernelStartParamGet
** ��������: ���ϵͳ�ں���������
** �䡡��  : pcParam       ��������
**           stLen         ����������
** �䡡��  : ʵ�ʳ���
** ȫ�ֱ���: 
** ����ģ��: 
                                       API ����
*********************************************************************************************************/
LW_API
ssize_t  API_KernelStartParamGet (PCHAR  pcParam, size_t  stLen)
{
    if (!pcParam || !stLen) {
        _ErrorHandle(ERROR_KERNEL_BUFFER_NULL);
        return  (0);
    }

    return  ((ssize_t)lib_strlcpy(pcParam, _K_cKernelStartParam, stLen));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
