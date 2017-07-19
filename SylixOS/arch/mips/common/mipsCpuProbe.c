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
** ��   ��   ��: mipsCpuProbe.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 07 �� 18 ��
**
** ��        ��: MIPS ��ϵ���� CPU ̽��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
UINT    _G_uiMipsProcessorId = 0;                                       /*  ������ ID                   */
UINT    _G_uiMipsCpuType     = PRID_IMP_UNKNOWN;                        /*  CPU ����                    */
/*********************************************************************************************************
** ��������: mispCpuProbeLegacy
** ��������: ̽�� Legacy CPU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mispCpuProbeLegacy (VOID)
{
    switch (_G_uiMipsProcessorId & PRID_IMP_MASK) {

    case PRID_IMP_LOONGSON_64:                                          /*  Loongson-2/3                */
        switch (_G_uiMipsProcessorId & PRID_REV_MASK) {

        case PRID_REV_LOONGSON2E:
        case PRID_REV_LOONGSON2F:
            _G_uiMipsCpuType = CPU_LOONGSON2;
            break;

        case PRID_REV_LOONGSON3A_R1:
        case PRID_REV_LOONGSON3A_R2:
        case PRID_REV_LOONGSON3A_R3:
        case PRID_REV_LOONGSON3B_R1:
        case PRID_REV_LOONGSON3B_R2:
            _G_uiMipsCpuType = CPU_LOONGSON3;
            break;
        }
        break;

    case PRID_IMP_LOONGSON_32:                                          /*  Loongson-1                  */
        _G_uiMipsCpuType = CPU_LOONGSON1;
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: mispCpuProbeIngenic
** ��������: ̽����� CPU
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mispCpuProbeIngenic (VOID)
{
    switch (_G_uiMipsProcessorId & PRID_IMP_MASK) {

    case PRID_IMP_JZRISC:
        _G_uiMipsCpuType = CPU_JZRISC;
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: mipsCpuProbe
** ��������: mips CPU ̽��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsCpuProbe (VOID)
{
    _G_uiMipsProcessorId = mipsCp0PRIdRead();
    switch (_G_uiMipsProcessorId & PRID_COMP_MASK) {

    case PRID_COMP_LEGACY:
        mispCpuProbeLegacy();
        break;

    case PRID_COMP_INGENIC:
    case PRID_COMP_INGENIC_4780:
        mispCpuProbeIngenic();
        break;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
