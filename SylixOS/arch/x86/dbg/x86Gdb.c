/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86Gdb.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 04 日
**
** 描        述: x86 体系构架 GDB 调试接口.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "arch/arch_gdb.h"
#include "libudis86/extern.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml 回应包
*********************************************************************************************************/
static const CHAR   _G_cX86CoreXml[] = \
    "<?xml version=\"1.0\"?>"
    "<!-- Copyright (C) 2006-2016 SylixOS Group."
         "Copying and distribution of this file, with or without modification,"
         "are permitted in any medium without royalty provided the copyright"
         "notice and this notice are preserved.  -->"
    "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
    "<feature name=\"org.gnu.gdb.i386.core\">"
      "<flags id=\"i386_eflags\" size=\"4\">"
        "<field name=\"CF\" start=\"0\" end=\"0\"/>"
        "<field name=\"\" start=\"1\" end=\"1\"/>"
        "<field name=\"PF\" start=\"2\" end=\"2\"/>"
        "<field name=\"AF\" start=\"4\" end=\"4\"/>"
        "<field name=\"ZF\" start=\"6\" end=\"6\"/>"
        "<field name=\"SF\" start=\"7\" end=\"7\"/>"
        "<field name=\"TF\" start=\"8\" end=\"8\"/>"
        "<field name=\"IF\" start=\"9\" end=\"9\"/>"
        "<field name=\"DF\" start=\"10\" end=\"10\"/>"
        "<field name=\"OF\" start=\"11\" end=\"11\"/>"
        "<field name=\"NT\" start=\"14\" end=\"14\"/>"
        "<field name=\"RF\" start=\"16\" end=\"16\"/>"
        "<field name=\"VM\" start=\"17\" end=\"17\"/>"
        "<field name=\"AC\" start=\"18\" end=\"18\"/>"
        "<field name=\"VIF\" start=\"19\" end=\"19\"/>"
        "<field name=\"VIP\" start=\"20\" end=\"20\"/>"
        "<field name=\"ID\" start=\"21\" end=\"21\"/>"
      "</flags>"
      "<reg name=\"eax\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ecx\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"edx\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ebx\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"esp\" bitsize=\"32\" type=\"data_ptr\"/>"
      "<reg name=\"ebp\" bitsize=\"32\" type=\"data_ptr\"/>"
      "<reg name=\"esi\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"edi\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"eip\" bitsize=\"32\" type=\"code_ptr\"/>"
      "<reg name=\"eflags\" bitsize=\"32\" type=\"i386_eflags\"/>"
      "<reg name=\"cs\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ss\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"ds\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"es\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"fs\" bitsize=\"32\" type=\"int32\"/>"
      "<reg name=\"gs\" bitsize=\"32\" type=\"int32\"/>"
    "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml 回应包
*********************************************************************************************************/
static const CHAR   _G_cX86TargetXml[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
            "<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  寄存器在数组中的索引
*********************************************************************************************************/
#define GDB_X86_EAX_INDEX       0
#define GDB_X86_ECX_INDEX       1
#define GDB_X86_EDX_INDEX       2
#define GDB_X86_EBX_INDEX       3
#define GDB_X86_ESP_INDEX       4
#define GDB_X86_EBP_INDEX       5
#define GDB_X86_ESI_INDEX       6
#define GDB_X86_EDI_INDEX       7
#define GDB_X86_EIP_INDEX       8
#define GDB_X86_EFLAGS_INDEX    9
#define GDB_X86_CS_INDEX        10
#define GDB_X86_SS_INDEX        11
#define GDB_X86_DS_INDEX        12
#define GDB_X86_ES_INDEX        13
#define GDB_X86_FS_INDEX        14
#define GDB_X86_GS_INDEX        15
#define GDB_X86_REG_NR          16
/*********************************************************************************************************
** 函数名称: archGdbTargetXml
** 功能描述: 获得 Xfer:features:read:target.xml 回复 XML
** 输　入  : NONE
** 输　出  : 回复 XML
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  archGdbTargetXml (VOID)
{
    return  (_G_cX86TargetXml);
}
/*********************************************************************************************************
** 函数名称: archGdbCoreXml
** 功能描述: 获得 Xfer:features:read:arch-core.xml 回复 XML
** 输　入  : NONE
** 输　出  : 回复 XML
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (_G_cX86CoreXml);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsGet
** 功能描述: 获取寄存器值
** 输　入  : pvDtrace       dtrace 节点
**           ulThread       线程句柄
**           pregset        gdb 寄存器结构
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsGet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->GDBR_iRegCnt = GDB_X86_REG_NR;

    pregset->regArr[GDB_X86_EAX_INDEX].GDBRA_ulValue    = regctx.REG_uiEAX;
    pregset->regArr[GDB_X86_ECX_INDEX].GDBRA_ulValue    = regctx.REG_uiECX;
    pregset->regArr[GDB_X86_EDX_INDEX].GDBRA_ulValue    = regctx.REG_uiEDX;
    pregset->regArr[GDB_X86_EBX_INDEX].GDBRA_ulValue    = regctx.REG_uiEBX;
    pregset->regArr[GDB_X86_ESP_INDEX].GDBRA_ulValue    = regSp;
    pregset->regArr[GDB_X86_EBP_INDEX].GDBRA_ulValue    = regctx.REG_uiEBP;
    pregset->regArr[GDB_X86_ESI_INDEX].GDBRA_ulValue    = regctx.REG_uiESI;
    pregset->regArr[GDB_X86_EDI_INDEX].GDBRA_ulValue    = regctx.REG_uiEDI;
    pregset->regArr[GDB_X86_EIP_INDEX].GDBRA_ulValue    = regctx.REG_uiEIP;

    pregset->regArr[GDB_X86_EFLAGS_INDEX].GDBRA_ulValue = regctx.REG_uiEFLAGS;
    pregset->regArr[GDB_X86_CS_INDEX].GDBRA_ulValue     = regctx.REG_uiCS;
    pregset->regArr[GDB_X86_SS_INDEX].GDBRA_ulValue     = regctx.REG_usSS;
    pregset->regArr[GDB_X86_DS_INDEX].GDBRA_ulValue     = regctx.REG_usDS;
    pregset->regArr[GDB_X86_ES_INDEX].GDBRA_ulValue     = regctx.REG_usES;
    pregset->regArr[GDB_X86_FS_INDEX].GDBRA_ulValue     = regctx.REG_usFS;
    pregset->regArr[GDB_X86_GS_INDEX].GDBRA_ulValue     = regctx.REG_usGS;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegsSet
** 功能描述: 设置寄存器值
** 输　入  : pvDtrace       dtrace 节点
**           ulThread       线程句柄
**           pregset        gdb 寄存器结构
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegsSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pregset)
{
    ARCH_REG_CTX  regctx;

    lib_bzero(&regctx, sizeof(ARCH_REG_CTX));

    regctx.REG_uiEAX    = pregset->regArr[GDB_X86_EAX_INDEX].GDBRA_ulValue;
    regctx.REG_uiECX    = pregset->regArr[GDB_X86_ECX_INDEX].GDBRA_ulValue;
    regctx.REG_uiEDX    = pregset->regArr[GDB_X86_EDX_INDEX].GDBRA_ulValue;
    regctx.REG_uiEBX    = pregset->regArr[GDB_X86_EBX_INDEX].GDBRA_ulValue;

    regctx.REG_uiEBP    = pregset->regArr[GDB_X86_EBP_INDEX].GDBRA_ulValue;
    regctx.REG_uiESI    = pregset->regArr[GDB_X86_ESI_INDEX].GDBRA_ulValue;
    regctx.REG_uiEDI    = pregset->regArr[GDB_X86_EDI_INDEX].GDBRA_ulValue;
    regctx.REG_uiEIP    = pregset->regArr[GDB_X86_EIP_INDEX].GDBRA_ulValue;

    regctx.REG_uiEFLAGS = pregset->regArr[GDB_X86_EFLAGS_INDEX].GDBRA_ulValue;
    regctx.REG_uiCS     = pregset->regArr[GDB_X86_CS_INDEX].GDBRA_ulValue;
    regctx.REG_usSS     = pregset->regArr[GDB_X86_SS_INDEX].GDBRA_ulValue;
    regctx.REG_usDS     = pregset->regArr[GDB_X86_DS_INDEX].GDBRA_ulValue;
    regctx.REG_usES     = pregset->regArr[GDB_X86_ES_INDEX].GDBRA_ulValue;
    regctx.REG_usFS     = pregset->regArr[GDB_X86_FS_INDEX].GDBRA_ulValue;
    regctx.REG_usGS     = pregset->regArr[GDB_X86_GS_INDEX].GDBRA_ulValue;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegSetPc
** 功能描述: 设置 PC 寄存器值
** 输　入  : pvDtrace       dtrace 节点
**           ulThread       线程句柄
**           ulPC           PC 寄存器值
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archGdbRegSetPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, ULONG  ulPc)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    regctx.REG_uiEIP = ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archGdbRegGetPc
** 功能描述: 获取 PC 寄存器值
** 输　入  : pRegs       寄存器数组
** 输　出  : PC 寄存器值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG archGdbRegGetPc (GDB_REG_SET  *pRegs)
{
    return  (pRegs->regArr[GDB_X86_EIP_INDEX].GDBRA_ulValue);
}
/*********************************************************************************************************
** 函数名称: archGdbGetNextPc
** 功能描述: 获取下一条指令地址，含分支预测
** 输　入  : pvDtrace       dtrace 节点
**           ulThread       线程句柄
**           pRegs          寄存器数组
** 输　出  : 下一条指令地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  archGdbGetNextPc (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, GDB_REG_SET  *pRegs)
{
    struct ud       ud;
    UINT            uiLen;
    ULONG           ulNextPc;
    ULONG           ulJmpAddr = 0;
    ARCH_REG_T      uiEIP;
    ARCH_REG_T      uiEFLAGS;
    ARCH_REG_T      uiECX;
    ARCH_REG_T      uiESP;
    ARCH_REG_T      uiRegVal;
    CHAR            cInsnAsm[256];
    PCHAR           pcLast;
    PCHAR           pcInsn;
    PCHAR           pcJmpAddr;
    PCHAR           pcReg;
#define X86_ADDR_MODE_DIRECT    0                                       /*  跳转地址是立即数            */
#define X86_ADDR_MODE_REG       1                                       /*  跳转地址在寄存器            */
#define X86_ADDR_MODE_MEM       2                                       /*  跳转地址在内存              */
    UINT8           ucAddressMode = X86_ADDR_MODE_DIRECT;

    uiEIP    = pRegs->regArr[GDB_X86_EIP_INDEX].GDBRA_ulValue;
    uiEFLAGS = pRegs->regArr[GDB_X86_EFLAGS_INDEX].GDBRA_ulValue;
    uiECX    = pRegs->regArr[GDB_X86_ECX_INDEX].GDBRA_ulValue;
    uiESP    = pRegs->regArr[GDB_X86_ESP_INDEX].GDBRA_ulValue;

    ud_init(&ud);                                                       /*  初始化 UD 结构              */

    ud_set_mode(&ud, 32);                                               /*  设置为 32 位模式            */

    ud_set_syntax(&ud, UD_SYN_ATT);                                     /*  设置为 AT&T 语法            */

    ud_set_pc(&ud, (uint64_t)uiEIP);                                    /*  设置 PC                     */

    ud_set_input_buffer(&ud, (uint8_t *)uiEIP, 32);                     /*  设置输入缓冲                */

    uiLen = ud_disassemble(&ud);                                        /*  反汇编                      */

    lib_strcpy(cInsnAsm, ud_insn_asm(&ud));                             /*  拷贝到本地数组              */

    ulNextPc = uiEIP + uiLen;                                           /*  默认的下一条指令地址        */

    pcLast = LW_NULL;
    pcInsn = LW_NULL;

    if ((pcInsn = lib_strtok_r(cInsnAsm, " ", &pcLast)) != LW_NULL) {   /*  获得指令码                  */

        pcJmpAddr = lib_strtok_r(LW_NULL, " ", &pcLast);                /*  获得立即数                  */
        if (pcJmpAddr != LW_NULL) {

            ucAddressMode = X86_ADDR_MODE_DIRECT;                       /*  跳转地址是立即数            */

            ulJmpAddr = lib_strtoul(pcJmpAddr, LW_NULL, 16);

            if ((pcReg = lib_strchr(pcJmpAddr, '%'))) {                 /*  使用寄存器                  */

                ucAddressMode = X86_ADDR_MODE_REG;                      /*  跳转地址在寄存器            */

                if (lib_strchr(pcJmpAddr, '(')) {                       /*  使用内存                    */
                    ucAddressMode = X86_ADDR_MODE_MEM;                  /*  跳转地址在内存              */
                }

                /*
                 * 获得指定寄存器的值
                 */
                if (lib_strncmp("%ebx", pcReg, 4) == 0) {
                    uiRegVal = pRegs->regArr[GDB_X86_EBX_INDEX].GDBRA_ulValue;

                } else if (lib_strncmp("%eax", pcReg, 4) == 0) {
                    uiRegVal = pRegs->regArr[GDB_X86_EAX_INDEX].GDBRA_ulValue;

                } else if (lib_strncmp("%ecx", pcReg, 4) == 0) {
                    uiRegVal = uiECX;

                } else if (lib_strncmp("%edx", pcReg, 4) == 0) {
                    uiRegVal = pRegs->regArr[GDB_X86_EDX_INDEX].GDBRA_ulValue;

                } else if (lib_strncmp("%esp", pcReg, 4) == 0) {
                    uiRegVal = uiESP;

                } else if (lib_strncmp("%ebp", pcReg, 4) == 0) {
                    uiRegVal = pRegs->regArr[GDB_X86_EBP_INDEX].GDBRA_ulValue;

                } else if (lib_strncmp("%esi", pcReg, 4) == 0) {
                    uiRegVal = pRegs->regArr[GDB_X86_ESI_INDEX].GDBRA_ulValue;

                } else if (lib_strncmp("%edi", pcReg, 4) == 0) {
                    uiRegVal = pRegs->regArr[GDB_X86_EDI_INDEX].GDBRA_ulValue;

                } else {
                    return  (PX_ERROR);
                }
            }
        }

        if (lib_strcmp(pcInsn, "call") == 0) {
            if (ucAddressMode == X86_ADDR_MODE_DIRECT) {
                ulNextPc = ulJmpAddr;

            } else if (ucAddressMode == X86_ADDR_MODE_MEM) {
                ulNextPc = *(ARCH_REG_T *)(uiRegVal + ulJmpAddr);

            } else {
                ulNextPc = uiRegVal;
            }

#if LW_CFG_VMM_EN > 0
            if (!API_VmmVirtualIsInside(ulNextPc)) {
                ulNextPc = uiEIP + uiLen;
            }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
        } else if (lib_strcmp(pcInsn, "jmp") == 0) {
            if (ucAddressMode == X86_ADDR_MODE_DIRECT) {
                /*
                 * 反汇编后, JMP 不用再相对 EIP 做转移
                 */
                ulNextPc = ulJmpAddr;

            } else if (ucAddressMode == X86_ADDR_MODE_MEM) {
                ulNextPc = *(ARCH_REG_T *)(uiRegVal + ulJmpAddr);

            } else {
                ulNextPc = uiRegVal;
            }

#if LW_CFG_VMM_EN > 0

/*********************************************************************************************************

00000190 <puts@plt-0x10>:
 190:   ff b3 04 00 00 00       pushl  0x4(%ebx)
 196:   ff a3 08 00 00 00       jmp    *0x8(%ebx)
 19c:   00 00                   add    %al,(%eax)
    ...

000001a0 <puts@plt>:
 1a0:   ff a3 0c 00 00 00       jmp    *0xc(%ebx)   这里其实是跳转到内核的 puts 函数, 为避免进入,
                                                    ulNextPc = *(ARCH_REG_T *)uiESP; 因为是通过 call 来到
                                                    这里的, call 把返回地址压栈了
 1a6:   68 00 00 00 00          push   $0x0
 1ab:   e9 e0 ff ff ff          jmp    190 <puts@plt-0x10>
    ...

000001b0 <main>:
    ...
 1ea:   e8 b1 ff ff ff          call   1a0 <puts@plt>
    ...
*********************************************************************************************************/

            if (!API_VmmVirtualIsInside(ulNextPc)) {
                ulNextPc = *(ARCH_REG_T *)uiESP;
            }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

        } else if (lib_strcmp(pcInsn, "ret") == 0) {
            /*
             * x86的堆栈是满递减堆栈
             */
            ulNextPc = *(ARCH_REG_T *)uiESP;

        } else if (lib_strcmp(pcInsn, "ja") == 0) {
            /*
             * cf = 0 且 zf = 0 转移
             */
            if (((uiEFLAGS & X86_EFLAGS_CF) == 0) &&
                ((uiEFLAGS & X86_EFLAGS_ZF) == 0)) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jae") == 0) {
            /*
             * cf = 0 转移
             */
            if ((uiEFLAGS & X86_EFLAGS_CF) == 0) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jb") == 0) {
            /*
             * cf = 1 转移
             */
            if (uiEFLAGS & X86_EFLAGS_CF) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jbe") == 0) {
            /*
             * cf = 1 或 zf =1 转移
             */
            if ((uiEFLAGS & X86_EFLAGS_CF) || (uiEFLAGS & X86_EFLAGS_ZF)) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jcxz") == 0) {
            /*
             * 若 cx = 0 转移
             */
            if ((uiECX & 0x0000FFFF) == 0) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jecxz") == 0) {
            /*
             * 若 ecx = 0 转移
             */
            if (uiECX == 0) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jg") == 0) {
            /*
             * sf = of, 且 zf = 0 转移
             */
            if ((((uiEFLAGS >> X86_EFLAGS_SF_BIT) & 0x1) == ((uiEFLAGS >> X86_EFLAGS_OF_BIT) & 0x1)) &&
                 ((uiEFLAGS &  X86_EFLAGS_ZF) == 0)) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jge") == 0) {
            /*
             * sf = of 转移
             */
            if (((uiEFLAGS >> X86_EFLAGS_SF_BIT) & 0x1) == ((uiEFLAGS >> X86_EFLAGS_OF_BIT) & 0x1)) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jl") == 0) {
            /*
             * sf ≠ of 转移
             */
            if (((uiEFLAGS >> X86_EFLAGS_SF_BIT) & 0x1) != ((uiEFLAGS >> X86_EFLAGS_OF_BIT) & 0x1)) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jle") == 0) {
            /*
             * sf ≠ of 或 zf = 1 转移
             */
            if ((((uiEFLAGS >> X86_EFLAGS_SF_BIT) & 0x1) != ((uiEFLAGS >> X86_EFLAGS_OF_BIT) & 0x1)) ||
                  (uiEFLAGS  & X86_EFLAGS_ZF)) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jno") == 0) {
            /*
             * of = 0 转移
             */
            if ((uiEFLAGS & X86_EFLAGS_OF) == 0) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jnp") == 0) {
            /*
             * pf = 0 转移
             */
            if ((uiEFLAGS & X86_EFLAGS_PF) == 0) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jns") == 0) {
            /*
             * sf = 0 转移
             */
            if ((uiEFLAGS & X86_EFLAGS_SF) == 0) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jnz") == 0) {
            /*
             * zf = 0 转移
             */
            if ((uiEFLAGS & X86_EFLAGS_ZF) == 0) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jo") == 0) {
            /*
             * of = 1 转移
             */
            if (uiEFLAGS & X86_EFLAGS_OF) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jp") == 0) {
            /*
             * pf = 1 转移
             */
            if (uiEFLAGS & X86_EFLAGS_PF) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jrcxz") == 0) {
            /*
             * 若 rcx = 0 转移(x64)
             */
            if (uiECX == 0) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "js") == 0) {
            /*
             * sf = 1 转移
             */
            if (uiEFLAGS & X86_EFLAGS_SF) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "jz") == 0) {
            /*
             * zf = 1 转移
             */
            if (uiEFLAGS & X86_EFLAGS_ZF) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "loop") == 0) {
            /*
             * (cx/ecx) – 1 → cx/ecx, 若 (cx/ecx) ≠ 0 则转移
             */
            if (--uiECX) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "loope") == 0) {
            /*
             * (cx/ecx) – 1 → cx/ecx, 若 (cx/ecx) ≠ 0 且 zf = 1 则转移
             */
            if ((--uiECX) && (uiEFLAGS & X86_EFLAGS_ZF)) {
                ulNextPc = ulJmpAddr;
            }

        } else if (lib_strcmp(pcInsn, "loopne") == 0) {
            /*
             * (cx/ecx) – 1 → cx/ecx, 若 (cx/ecx) ≠ 0 且 zf = 0 则转移
             */
            if ((--uiECX) && ((uiEFLAGS & X86_EFLAGS_ZF) == 0)) {
                ulNextPc = ulJmpAddr;
            }
        }
    }

    return  (ulNextPc);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
