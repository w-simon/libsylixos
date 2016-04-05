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
** ��   ��   ��: mipsGdb.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ���� GDB ���Խӿ�.
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#include "../mips_gdb.h"
/*********************************************************************************************************
  Xfer:features:read:arch-core.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   cMipsCore[] = \
        "l<?xml version=\"1.0\"?>"
        "<!-- Copyright (C) 2006-2016 SylixOS Group."
             "Copying and distribution of this file, with or without modification,"
             "are permitted in any medium without royalty provided the copyright"
             "notice and this notice are preserved.  -->"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<feature name=\"org.gnu.gdb.mips.cpu\">"
          "<reg name=\"zero\" bitsize=\"32\" type=\"uint32\" regnum=\"0\"/>"
          "<reg name=\"at\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"v0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"v1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"a0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"a1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"a2\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"a3\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t2\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t3\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t4\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t5\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t6\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t7\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s2\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s3\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s4\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s5\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s6\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s7\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t8\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"t9\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"k0\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"k1\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"gp\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"sp\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"s8\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"ra\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"sr\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"lo\" bitsize=\"32\" type=\"uint32\" regnum=\"33\"/>"
          "<reg name=\"hi\" bitsize=\"32\" type=\"uint32\" regnum=\"34\"/>"
          "<reg name=\"bad\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"cause\" bitsize=\"32\" type=\"uint32\"/>"
          "<reg name=\"pc\" bitsize=\"32\" type=\"uint32\" regnum=\"37\"/>"
        "</feature>";
/*********************************************************************************************************
  Xfer:features:read:target.xml ��Ӧ��
*********************************************************************************************************/
static const CHAR   cTargetSystem[] = \
        "l<?xml version=\"1.0\"?>"
        "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
        "<target>"
            "<xi:include href=\"arch-core.xml\"/>"
        "</target>";
/*********************************************************************************************************
  PC �Ĵ����� GDB_REG_SET �ṹ�е�����
*********************************************************************************************************/
#define MIPS_REG_INDEX_PC   37
/*********************************************************************************************************
  ����״̬�Ĵ�������Ϊ��fpcsr �Ĵ�����λΪ 1 ��ʶ��������
*********************************************************************************************************/
#define FP_COND             0x800000
/*********************************************************************************************************
** ��������: archGdbTargetXml
** ��������: ��� Xfer:features:read:target.xml �ظ� XML
** �䡡��  : NONE
** �䡡��  : �ظ� XML
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CPCHAR  archGdbTargetXml (VOID)
{
    return  (cTargetSystem);
}
/*********************************************************************************************************
** ��������: archGdbCoreXml
** ��������: ��� Xfer:features:read:mips-core.xml �ظ� XML
** �䡡��  : NONE
** �䡡��  : �ظ� XML
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CPCHAR  archGdbCoreXml (VOID)
{
    return  (cMipsCore);
}
/*********************************************************************************************************
** ��������: archGdbRegsGet
** ��������: ��ȡ�Ĵ���ֵ
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
** �䡡��  : pregset        gdb �Ĵ����ṹ
**           ����ֵ         �ɹ�-- ERROR_NONE��ʧ��-- PX_ERROR.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archGdbRegsGet (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pregset)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;
    INT           iIndex = 0;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    lib_bzero(pregset, sizeof(GDB_REG_SET));

    pregset->regArr[iIndex++].GDBRA_ulValue = 0;
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_AT];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_V0];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_V1];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_A0];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_A1];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_A2];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_A3];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T0];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T1];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T2];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T3];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T4];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T5];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T6];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T7];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_S0];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_S1];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_S2];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_S3];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_S4];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_S5];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_S6];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_S7];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T8];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_T9];
    pregset->regArr[iIndex++].GDBRA_ulValue = 0;                        /*  k0, k1                      */
    pregset->regArr[iIndex++].GDBRA_ulValue = 0;
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_GP];
    pregset->regArr[iIndex++].GDBRA_ulValue = regSp;
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_FP];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiReg[REG_RA];
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiCP0Status;
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiCP0DataLO;
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiCP0DataHI;
    pregset->regArr[iIndex++].GDBRA_ulValue = 0;                        /*  bad							*/
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiCP0Cause;
    pregset->regArr[iIndex++].GDBRA_ulValue = regctx.REG_uiCP0EPC;
    iIndex += 34;

    /*
     * ��� Cause �Ĵ��� BD λ��Ϊ 1����˵�������жϵ�Ϊ��֧��ʱ��ָ�PC �Ĵ���ֵ�����
     */
    if (regctx.REG_uiCP0Cause & M_CauseBD) {
        pregset->regArr[MIPS_REG_INDEX_PC].GDBRA_ulValue += sizeof(ULONG);
    }

    pregset->GDBR_iRegCnt = iIndex;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archGdbRegsSet
** ��������: ���üĴ���ֵ
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
**           pregset        gdb �Ĵ����ṹ
** �䡡��  : �ɹ�-- ERROR_NONE��ʧ��-- PX_ERROR.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archGdbRegsSet (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pregset)
{
    ARCH_REG_CTX  regctx;
    INT           iIndex = 0;

    iIndex++;
    regctx.REG_uiReg[REG_AT] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_V0] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_V1] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_A0] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_A1] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_A2] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_A3] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T0] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T1] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T2] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T3] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T4] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T5] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T6] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T7] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_S0] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_S1] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_S2] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_S3] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_S4] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_S5] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_S6] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_S7] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T8] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_T9] = pregset->regArr[iIndex++].GDBRA_ulValue;
    iIndex++;
    iIndex++;
    regctx.REG_uiReg[REG_GP] = pregset->regArr[iIndex++].GDBRA_ulValue;
    iIndex++;
    regctx.REG_uiReg[REG_FP] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiReg[REG_RA] = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiCP0Status   = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiCP0DataLO   = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiCP0DataHI   = pregset->regArr[iIndex++].GDBRA_ulValue;
    iIndex++;
    regctx.REG_uiCP0Cause    = pregset->regArr[iIndex++].GDBRA_ulValue;
    regctx.REG_uiCP0EPC      = pregset->regArr[iIndex++].GDBRA_ulValue;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archGdbRegSetPc
** ��������: ���� pc �Ĵ���ֵ
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
**           ulPC           pc �Ĵ���ֵ
** �䡡��  : �ɹ�-- ERROR_NONE��ʧ��-- PX_ERROR.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archGdbRegSetPc (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, ULONG ulPc)
{
    ARCH_REG_CTX  regctx;
    ARCH_REG_T    regSp;

    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);

    regctx.REG_uiCP0EPC = (ARCH_REG_T)ulPc;

    API_DtraceSetRegs(pvDtrace, ulThread, &regctx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archGdbRegGetPc
** ��������: ��ȡ pc �Ĵ���ֵ
** �䡡��  : pRegs       �Ĵ�������
** �䡡��  : PC�Ĵ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG archGdbRegGetPc (GDB_REG_SET *pRegs)
{
    return  (pRegs->regArr[MIPS_REG_INDEX_PC].GDBRA_ulValue);
}
/*********************************************************************************************************
** ��������: archGdbGetNextPc
** ��������: ��ȡ��һ��ָ���ַ������֧Ԥ��
** �䡡��  : pvDtrace       dtrace ���
**           ulThread       �������߳�
**           pRegs          �Ĵ�������
** �䡡��  : ��һ��ָ���ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  archGdbGetNextPc (PVOID pvDtrace, LW_OBJECT_HANDLE ulThread, GDB_REG_SET *pRegs)
{
    INT             rsVal;
    INT             rtVal;
    INT             disp;
    ULONG           machInstr;
    ULONG           npc;
    ULONG           pc;
    ULONG           ulFpcsr;
    ARCH_REG_CTX    regctx;
    ARCH_REG_T      regSp;
    ARCH_FPU_CTX    fpuctx;

    API_DtraceGetFpuRegs(pvDtrace, ulThread, &fpuctx);
    ulFpcsr = fpuctx.FPUCTX_uiFpcsr;

    pc = (ULONG) pRegs->regArr[MIPS_REG_INDEX_PC].GDBRA_ulValue;        /*  ��ǰ PC ָ��                */
    /*
     * ��� PC Ϊ��֧��ʱ��ָ��������Ϊ branch ָ��
     */
    API_DtraceGetRegs(pvDtrace, ulThread, &regctx, &regSp);
    if (regctx.REG_uiCP0Cause & M_CauseBD) {
        pc -= sizeof(ULONG);
    }
    machInstr = *(ULONG *) pc;                                          /*  ��ǰָ��                    */

    npc = pc + 4;                                                       /*  Ĭ��ֱ��ȡ��һ��ָ��        */

    /*
     * ��ǰָ��Ϊ��תָ��ʱ���������������������ת��������������һ��ָ��ΪĿ���ֵַ
     * ������һ��ָ��Ϊ pc+8����Ϊ��תָ��ͽ������ķ�֧��ʱ��Ϊһ�����壬
     * ������Ϊһ��ָ���
     */
    rsVal = pRegs->regArr[(machInstr >> 21) & 0x1f].GDBRA_ulValue;
    rtVal = pRegs->regArr[(machInstr >> 16) & 0x1f].GDBRA_ulValue;
    disp = ((INT) ((machInstr & 0x0000ffff) << 16)) >> 14;
    if ((machInstr & 0xf3ff0000) == 0x41020000) {                       /*  BCzFL                       */
        INT copId = (machInstr >> 26) & 0x03;
        npc = pc + 8;
        switch (copId) {
        case 1:
#ifndef SOFT_FLOAT
            if ((ulFpcsr & FP_COND) != FP_COND) {                       /*  ������תָ�����жϸ�����  */
                npc = disp + pc + 4;
            }
#endif
            break;
        }
    } else if ((machInstr & 0xf3ff0000) == 0x41030000) {                /*  BCzTL                       */
        INT copId = (machInstr >> 26) & 0x03;
        npc = pc + 8;
        switch (copId) {
        case 1:
#ifndef SOFT_FLOAT
            if ((ulFpcsr & FP_COND) == FP_COND) {
                npc = disp + pc + 4;
            }
#endif
            break;
        }
    } else if (((machInstr & 0xfc1f0000) == 0x04130000)                 /*  BGEZALL                     */
               || ((machInstr & 0xfc1f0000) == 0x04030000)) {           /*  BGEZL                       */
        if (rsVal >= 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if ((machInstr & 0xfc1f0000) == 0x5c000000) {                /*  BGTZL                       */
        if (rsVal > 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if ((machInstr & 0xfc1f0000) == 0x58000000) {                /*  BLEZL                       */
        if (rsVal <= 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if (((machInstr & 0xfc1f0000) == 0x04120000)                 /*  BLTZALL                     */
         || ((machInstr & 0xfc1f0000) == 0x04020000)) {                 /*  BLTZL                       */
        if (rsVal < 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if ((machInstr & 0xfc000000) == 0x50000000) {                /*  BEQL                        */
        if (rsVal == rtVal) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if ((machInstr & 0xfc000000) == 0x54000000) {                /*  BNEL                        */
        if (rsVal != rtVal) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if (((machInstr & 0xfc000000) == 0x08000000)                 /*  J                           */
               || ((machInstr & 0xfc000000) == 0x0c000000)) {           /*  JAL                         */
        npc = ((machInstr & 0x03ffffff) << 2) | (pc & 0xf0000000);
    } else if (((machInstr & 0xfc1f07ff) == 0x00000009)                 /*  JALR                        */
               || ((machInstr & 0xfc1fffff) == 0x00000008)) {           /*  JR                          */
        npc = pRegs->regArr[(machInstr >> 21) & 0x1f].GDBRA_ulValue;
    } else if ((machInstr & 0xf3ff0000) == 0x41000000) {                /*  BCzF                        */
        INT copId = (machInstr >> 26) & 0x03;
        npc = pc + 8;
        switch (copId){
        case 1:
#ifndef SOFT_FLOAT
            if ((ulFpcsr & FP_COND) != FP_COND) {
                npc = disp + pc + 4;
            }
#endif
            break;
        }
    } else if ((machInstr & 0xf3ff0000) == 0x41010000) {                /*  BCzT                        */
        INT copId = (machInstr >> 26) & 0x03;
        npc = pc + 8;
        switch (copId) {
        case 1:
#ifndef SOFT_FLOAT
            if ((ulFpcsr & FP_COND) == FP_COND) {
                npc = disp + pc + 4;
            }
#endif
            break;
        }
    } else if ((machInstr & 0xfc000000) == 0x10000000) {                /*  BEQ                         */
        if (rsVal == rtVal) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    }
    else if (((machInstr & 0xfc1f0000) == 0x04010000)                   /*  BGEZ                        */
         || ((machInstr & 0xfc1f0000) == 0x04110000)) {                 /*  BGEZAL                      */
        if (rsVal >= 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if ((machInstr & 0xfc1f0000) == 0x1c000000) {                /*  BGTZ                        */
        if (rsVal > 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if ((machInstr & 0xfc1f0000) == 0x18000000) {                /*  BLEZ                        */
        if (rsVal <= 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if (((machInstr & 0xfc1f0000) == 0x04000000)                 /*  BLTZ                        */
               || ((machInstr & 0xfc1f0000) == 0x04100000)) {           /*  BLTZAL                      */
        if (rsVal < 0) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    } else if ((machInstr & 0xfc000000) == 0x14000000) {                /*  BNE                         */
        if (rsVal != rtVal) {
            npc = disp + pc + 4;
        } else {
            npc = pc + 8;
        }
    }
#ifdef MIPSXX_cav_cn3xxx
    else if (((machInstr & CAVIUM_JUMP_MASK) >> 26) == 0x32) {
        INT     offset;
        INT     bitnr;
        INT     regnr;
        LONG    reg;
        ULONG   op;

        offset = (machInstr & 0xffff) << 2;
        bitnr  = (machInstr & (0x1f << 16)) >> 16;
        regnr  = (machInstr & (0x1f << 21)) >> 21;
        reg    = pRegs->regArr[regnr].GDBRA_ulValue;

        op = (machInstr >> 26);

        switch (op) {
        case BBIT0:                                                     /*  BBIT0                       */
            if (!(reg & (0x1 << bitnr))) {
                npc = pc + offset;
            }
            break;
        case BBIT032:                                                   /*  BBIT032                     */
            if (!(reg & (0x1 << (bitnr + 32)))) {
                npc = pc + offset;
            }
            break;
        case BBIT1:                                                     /*  BBIT1                       */
            if (reg & (0x1 << bitnr)) {
                npc = pc + offset;
            }
            break;
        case BBIT132:                                                   /*  BBIT132                     */
            if (reg & (0x1 << (bitnr + 32))) {
                npc = pc + offset;
            }
            break;
        default:
            break;
        }
    }
#endif
    else {
                                                                        /*  ��ָͨ��                    */
    }
    return  (npc);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
