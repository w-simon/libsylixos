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
** ��   ��   ��: arch_regs.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 11 �� 26 ��
**
** ��        ��: PowerPC �Ĵ������.
*********************************************************************************************************/

#ifndef __PPC_ARCH_REGS_H
#define __PPC_ARCH_REGS_H

/*********************************************************************************************************
  �Ĵ�����ջ֡�е�ƫ����
*********************************************************************************************************/

#define XR1         (0)
#define XBLK        (XR1   + 4)
#define XR2         (XBLK  + 4)
#define XR3         (XR2   + 4)
#define XR4         (XR3   + 4)
#define XR5         (XR4   + 4)
#define XR6         (XR5   + 4)
#define XR7         (XR6   + 4)
#define XR8         (XR7   + 4)
#define XR9         (XR8   + 4)
#define XR10        (XR9   + 4)
#define XR11        (XR10  + 4)
#define XR12        (XR11  + 4)
#define XR13        (XR12  + 4)
#define XR14        (XR13  + 4)
#define XR15        (XR14  + 4)
#define XR16        (XR15  + 4)
#define XR17        (XR16  + 4)
#define XR18        (XR17  + 4)
#define XR19        (XR18  + 4)
#define XR20        (XR19  + 4)
#define XR21        (XR20  + 4)
#define XR22        (XR21  + 4)
#define XR23        (XR22  + 4)
#define XR24        (XR23  + 4)
#define XR25        (XR24  + 4)
#define XR26        (XR25  + 4)
#define XR27        (XR26  + 4)
#define XR28        (XR27  + 4)
#define XR29        (XR28  + 4)
#define XR30        (XR29  + 4)
#define XR31        (XR30  + 4)
#define XR0         (XR31  + 4)
#define XSRR0       (XR0   + 4)
#define XSRR1       (XSRR0 + 4)
#define XCTR        (XSRR1 + 4)
#define XXER        (XCTR  + 4)
#define XCR         (XXER  + 4)
#define XLR         (XCR   + 4)
#define XPAD2       (XLR   + 4)
#define XPAD3       (XPAD2 + 4)
#define XMSR        (XPAD3 + 4)

/*********************************************************************************************************
  ջ֡�Ĵ�С
*********************************************************************************************************/

#define STACK_FRAME_SIZE   (XMSR + 4)

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if (!defined(__ASSEMBLY__)) && (!defined(ASSEMBLY))

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/

typedef UINT32      ARCH_REG_T;

typedef struct {
    ARCH_REG_T      REG_uiSp;
    ARCH_REG_T      REG_uiBlank;
    ARCH_REG_T      REG_uiR2;
    ARCH_REG_T      REG_uiR3;
    ARCH_REG_T      REG_uiR4;
    ARCH_REG_T      REG_uiR5;
    ARCH_REG_T      REG_uiR6;
    ARCH_REG_T      REG_uiR7;
    ARCH_REG_T      REG_uiR8;
    ARCH_REG_T      REG_uiR9;
    ARCH_REG_T      REG_uiR10;
    ARCH_REG_T      REG_uiR11;
    ARCH_REG_T      REG_uiR12;
    ARCH_REG_T      REG_uiR13;
    ARCH_REG_T      REG_uiR14;
    ARCH_REG_T      REG_uiR15;
    ARCH_REG_T      REG_uiR16;
    ARCH_REG_T      REG_uiR17;
    ARCH_REG_T      REG_uiR18;
    ARCH_REG_T      REG_uiR19;
    ARCH_REG_T      REG_uiR20;
    ARCH_REG_T      REG_uiR21;
    ARCH_REG_T      REG_uiR22;
    ARCH_REG_T      REG_uiR23;
    ARCH_REG_T      REG_uiR24;
    ARCH_REG_T      REG_uiR25;
    ARCH_REG_T      REG_uiR26;
    ARCH_REG_T      REG_uiR27;
    ARCH_REG_T      REG_uiR28;
    ARCH_REG_T      REG_uiR29;
    ARCH_REG_T      REG_uiR30;
    ARCH_REG_T      REG_uiFp;
    ARCH_REG_T      REG_uiR0;
    ARCH_REG_T      REG_uiSrr0;
    ARCH_REG_T      REG_uiSrr1;
    ARCH_REG_T      REG_uiCtr;
    ARCH_REG_T      REG_uiXer;
    ARCH_REG_T      REG_uiCr;
    ARCH_REG_T      REG_uiLr;
    ARCH_REG_T      REG_uiPad2;
    ARCH_REG_T      REG_uiPad3;
    ARCH_REG_T      REG_uiMsr;
} ARCH_REG_CTX;

/*********************************************************************************************************
  EABI ��׼���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T      FP_uiFp;
    ARCH_REG_T      FP_uiLr;
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiSrr0)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiFp)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  ��׼ȷ, ��Ϊ�˼��������    */

/*********************************************************************************************************
  ��ջ�еļĴ�����Ϣ��ռ��С
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  ((sizeof(ARCH_REG_CTX) + sizeof(ARCH_FP_CTX)) / sizeof(ARCH_REG_T))
#define ARCH_STK_MIN_WORD_SIZE  128

/*********************************************************************************************************
  CPU �ֳ�
*********************************************************************************************************/

#define LW_CFG_CPU_WORD_LENGHT  32                                      /*  CPU �ֳ�                    */

#endif

#endif                                                                  /*  __PPC_ARCH_REGS_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
