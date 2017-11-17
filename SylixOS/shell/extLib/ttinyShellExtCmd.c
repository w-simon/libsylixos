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
** ��   ��   ��: ttinyShellExtCmd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 11 �� 17 ��
**
** ��        ��: ϵͳ extension ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  Only for LITE version.
*********************************************************************************************************/
#if defined(__SYLIXOS_LITE)
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
/*********************************************************************************************************
** ��������: __tshellExtCmdTc
** ��������: ϵͳ���� "tc"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellExtCmdTc (INT  iArgC, PCHAR  ppcArgV[])
{
    INT                   iC;
    INT                   iPrio  = LW_PRIO_NORMAL;
    INT                   iStack = LW_CFG_THREAD_DEFAULT_STK_SIZE;
    ULONG                 ulArg  = 0ul;
    addr_t                addrT  = (addr_t)PX_ERROR;
    PCHAR                 pcName = LW_NULL;
    CHAR                  cName[LW_CFG_OBJECT_NAME_SIZE];
    
    LW_CLASS_THREADATTR   attr;
    LW_OBJECT_HANDLE      hThread;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (sscanf(ppcArgV[1], "%lx", &addrT) != 1) {
        fprintf(stderr, "address error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!ALIGNED(addrT, sizeof(FUNCPTR))) {
        fprintf(stderr, "address not aligned!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
#if defined(__SYLIXOS_ARM_ARCH_M__)
    addrT += 1;
#endif

    while ((iC = getopt(iArgC, ppcArgV, "n:p:s:a:")) != EOF) {
        switch (iC) {
        
        case 'p':
            iPrio = lib_atoi(optarg);
            break;
            
        case 's':
            iStack = lib_atoi(optarg);
            break;
            
        case 'a':
            ulArg = lib_strtoul(optarg, LW_NULL, 16);
            break;
            
        case 'n':
            pcName = optarg;
            break;
        }
    }
    
    getopt_free();
    
    if ((iPrio < LW_PRIO_HIGHEST) || (iPrio >= LW_PRIO_LOWEST)) {
        fprintf(stderr, "priority error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iStack < (ARCH_STK_MIN_WORD_SIZE * sizeof(PVOID))) {
        fprintf(stderr, "stack size error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (pcName) {
        if (lib_strnlen(pcName, LW_CFG_OBJECT_NAME_SIZE) >= LW_CFG_OBJECT_NAME_SIZE) {
            fprintf(stderr, "name too long!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
    
    } else {
        snprintf(cName, LW_CFG_OBJECT_NAME_SIZE, "tc_%lx", addrT);
        pcName = cName;
    }
    
    API_ThreadAttrBuild(&attr, (size_t)iStack, (UINT8)iPrio, 
                        LW_OPTION_OBJECT_LOCAL | LW_OPTION_THREAD_STK_CHK, 
                        (PVOID)ulArg);
                        
    hThread = API_ThreadCreate(pcName, (PTHREAD_START_ROUTINE)addrT, &attr, LW_NULL);
    if (!hThread) {
        fprintf(stderr, "can not create thread: %s!\n", lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellExtCmdInit
** ��������: ��ʼ�� Extension ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellExtCmdInit (VOID)
{
    API_TShellKeywordAdd("tc", __tshellExtCmdTc);
    API_TShellFormatAdd("tc", " address(Hex) [-n name] [-p priority(0~254)] [-s stacksize(Dec)] [-a argument(Hex)]");
    API_TShellHelpAdd("tc", "create a thread and run begin at specific address.\n"
                            "eg. tc 0x???\n"
                            "    tc 0x??? -n test -p 200 -s 4096\n"
                            "    tc 0x??? -n test -p 200 -s 4096\n"
                            "    tc 0x??? -n test -p 200 -s 4096 -a 0x00\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __SYLIXOS_LITE              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
