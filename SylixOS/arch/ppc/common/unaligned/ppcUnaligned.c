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
** ��   ��   ��: ppcUnaligned.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 08 �� 27 ��
**
** ��        ��: PowerPC ��ϵ���ܷǶ��봦��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/ppc/param/ppcParam.h"
#include <linux/compat.h>
#include "porting.h"
#include "sstep.h"
#include "disassemble.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
int fix_alignment(ARCH_REG_CTX *regs, enum instruction_type  *inst_type);
/*********************************************************************************************************
** ��������: ppcUnalignedHandle
** ��������: PowerPC �Ƕ��봦��
** �䡡��  : pregctx           �Ĵ���������
** �䡡��  : ��ֹ��Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_VMM_ABORT  ppcUnalignedHandle (ARCH_REG_CTX  *pregctx)
{
    PPC_PARAM             *param = archKernelParamGet();
    INT                    iFixed;
    enum instruction_type  type;
    LW_VMM_ABORT           abtInfo;

    if (pregctx->REG_uiDar == pregctx->REG_uiPc) {
        goto sigbus;
    }

    if (param->PP_bUnalign == LW_FALSE) {                               /*  Unsupport unalign access    */
        goto sigbus;
    }

    iFixed = fix_alignment(pregctx, &type);
    if (iFixed == 1) {
        pregctx->REG_uiPc += 4;                                         /*  Skip over emulated inst     */
        abtInfo.VMABT_uiType = 0;
        return  (abtInfo);

    } else if (iFixed == -EFAULT) {                                     /*  Operand address was bad     */
        switch (type) {

        case STORE:
        case STORE_MULTI:
        case STORE_FP:
        case STORE_VMX:
        case STORE_VSX:
        case STCX:
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
            break;

        case LOAD:
        case LOAD_MULTI:
        case LOAD_FP:
        case LOAD_VMX:
        case LOAD_VSX:
        case LARX:
        default:
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
            break;
        }

        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_MAP;
        return  (abtInfo);
    }

sigbus:
    abtInfo.VMABT_uiMethod = BUS_ADRALN;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    return  (abtInfo);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
