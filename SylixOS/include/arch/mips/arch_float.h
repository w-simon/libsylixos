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
** ��   ��   ��: arch_float.h
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS �������.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_FLOAT_H
#define __MIPS_ARCH_FLOAT_H

/*********************************************************************************************************
  Number of fp registers on coprocessor
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define FP_NUM_DREGS            32

#define FP0_OFFSET              0x0                                     /*  OFFSET(FPX[0])              */
#define FP1_OFFSET              0x8                                     /*  OFFSET(FPX[1])              */
#define FP2_OFFSET              0x10                                    /*  OFFSET(FPX[2])              */
#define FP3_OFFSET              0x18                                    /*  OFFSET(FPX[3])              */
#define FP4_OFFSET              0x20                                    /*  OFFSET(FPX[4])              */
#define FP5_OFFSET              0x28                                    /*  OFFSET(FPX[5])              */
#define FP6_OFFSET              0x30                                    /*  OFFSET(FPX[6])              */
#define FP7_OFFSET              0x38                                    /*  OFFSET(FPX[7])              */
#define FP8_OFFSET              0x40                                    /*  OFFSET(FPX[8])              */
#define FP9_OFFSET              0x48                                    /*  OFFSET(FPX[9])              */
#define FP10_OFFSET             0x50                                    /*  OFFSET(FPX[10])             */
#define FP11_OFFSET             0x58                                    /*  OFFSET(FPX[11])             */
#define FP12_OFFSET             0x60                                    /*  OFFSET(FPX[12])             */
#define FP13_OFFSET             0x68                                    /*  OFFSET(FPX[13])             */
#define FP14_OFFSET             0x70                                    /*  OFFSET(FPX[14])             */
#define FP15_OFFSET             0x78                                    /*  OFFSET(FPX[15])             */
#define FP16_OFFSET             0x80                                    /*  OFFSET(FPX[16])             */
#define FP17_OFFSET             0x88                                    /*  OFFSET(FPX[17])             */
#define FP18_OFFSET             0x90                                    /*  OFFSET(FPX[18])             */
#define FP19_OFFSET             0x98                                    /*  OFFSET(FPX[19])             */
#define FP20_OFFSET             0xa0                                    /*  OFFSET(FPX[20])             */
#define FP21_OFFSET             0xa8                                    /*  OFFSET(FPX[21])             */
#define FP22_OFFSET             0xb0                                    /*  OFFSET(FPX[22])             */
#define FP23_OFFSET             0xb8                                    /*  OFFSET(FPX[23])             */
#define FP24_OFFSET             0xc0                                    /*  OFFSET(FPX[24])             */
#define FP25_OFFSET             0xc8                                    /*  OFFSET(FPX[25])             */
#define FP26_OFFSET             0xd0                                    /*  OFFSET(FPX[26])             */
#define FP27_OFFSET             0xd8                                    /*  OFFSET(FPX[27])             */
#define FP28_OFFSET             0xe0                                    /*  OFFSET(FPX[28])             */
#define FP29_OFFSET             0xe8                                    /*  OFFSET(FPX[29])             */
#define FP30_OFFSET             0xf0                                    /*  OFFSET(FPX[30])             */
#define FP31_OFFSET             0xf8                                    /*  OFFSET(FPX[31])             */
#define FPCSR_OFFSET            0x100                                   /*  OFFSET(ARCH_FPU_CTX, fpcsr) */

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if (!defined(__ASSEMBLY__)) && (!defined(ASSEMBLY))

typedef struct arch_fpu_ctx {                                           /* FP_CONTEXT ������            */
    UINT32              FPUCTX_uiReg[FP_NUM_DREGS * 2];                 /* 32 ��˫���ȸ��� 64 λ�Ĵ���  */
    UINT32              FPUCTX_uiFpcsr;                                 /* status and control register  */
    UINT32              FPUCTX_uiPad;
} ARCH_FPU_CTX;

#define ARCH_FPU_CTX_ALIGN      8                                       /* FPU CTX align size           */

/*********************************************************************************************************
  float ��ʽ (ʹ�� union ������Ϊ�м�ת��, ���� GCC 3.x.x strict aliasing warning)
*********************************************************************************************************/

#define __ARCH_FLOAT_EXP_NAN           255                              /*  NaN ���������� Exp ֵ     */

typedef struct __cpu_float_field {
    unsigned int        frac : 23;
    unsigned int        exp  :  8;
    unsigned int        sig  :  1;
} __CPU_FLOAT_FIELD;

typedef union __cpu_float {
    __CPU_FLOAT_FIELD   fltfield;                                       /*  float λ���ֶ�              */
    float               flt;                                            /*  float ռλ                  */
} __CPU_FLOAT;

static LW_INLINE INT  __ARCH_FLOAT_ISNAN (float  x)
{
    __CPU_FLOAT     cpuflt;

    cpuflt.flt = x;

    return  ((cpuflt.fltfield.exp == __ARCH_FLOAT_EXP_NAN) && (cpuflt.fltfield.frac != 0));
}

static LW_INLINE INT  __ARCH_FLOAT_ISINF (float  x)
{
    __CPU_FLOAT     cpuflt;

    cpuflt.flt = x;

    return  ((cpuflt.fltfield.exp == __ARCH_FLOAT_EXP_NAN) && (cpuflt.fltfield.frac == 0));
}

/*********************************************************************************************************
  double ��ʽ
*********************************************************************************************************/

#define __ARCH_DOUBLE_EXP_NAN           2047                            /*  NaN ���������� Exp ֵ     */
#define __ARCH_DOUBLE_INC_FLOAT_H          0                            /*  �Ƿ����ñ����� float.h �ļ� */

/*********************************************************************************************************
  mips-sylixos-elf-gcc ... GNU
*********************************************************************************************************/

#if LW_CFG_DOUBLE_MIX_ENDIAN > 0
typedef struct __cpu_double_field {                                     /*  old mixed-endian            */
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;

    unsigned int        fracl : 32;                                     /*  �� 32 λ����ߵ�ַ          */
} __CPU_DOUBLE_FIELD;
#else
typedef struct __cpu_double_field {                                     /*  native-endian               */
    unsigned int        fracl : 32;                                     /*  �� 32 λ����͵�ַ          */

    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
} __CPU_DOUBLE_FIELD;
#endif                                                                  /*  __ARCH_DOUBLE_MIX_ENDIAN    */

typedef union __cpu_double {
    __CPU_DOUBLE_FIELD  dblfield;                                       /*  float λ���ֶ�              */
    double              dbl;                                            /*  float ռλ                  */
} __CPU_DOUBLE;

static LW_INLINE INT  __ARCH_DOUBLE_ISNAN (double  x)
{
    __CPU_DOUBLE     dblflt;

    dblflt.dbl = x;

    return  ((dblflt.dblfield.exp == __ARCH_DOUBLE_EXP_NAN) &&
             ((dblflt.dblfield.fracl != 0) &&
              (dblflt.dblfield.frach != 0)));
}

static LW_INLINE INT  __ARCH_DOUBLE_ISINF (double  x)
{
    __CPU_DOUBLE     dblflt;

    dblflt.dbl = x;

    return  ((dblflt.dblfield.exp == __ARCH_DOUBLE_EXP_NAN) &&
             ((dblflt.dblfield.fracl == 0) ||
              (dblflt.dblfield.frach == 0)));
}

#endif                                                                  /*  __ASSEMBLY__                */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __MIPS_ARCH_FLOAT_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
