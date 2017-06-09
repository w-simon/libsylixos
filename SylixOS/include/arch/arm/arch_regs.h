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
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 05 �� 04 ��
**
** ��        ��: ARM �Ĵ������.
**
** BUG:
2015.08.19 �� CPU �ֳ������ƶ�������.
*********************************************************************************************************/

#ifndef __ARM_ARCH_REGS_H
#define __ARM_ARCH_REGS_H

#include "asm/archprob.h"

/*********************************************************************************************************
  arm cpsr
*********************************************************************************************************/

#define ARCH_ARM_USR32MODE          0x10                                /*  �û�ģʽ                    */
#define ARCH_ARM_FIQ32MODE          0x11                                /*  �����ж�ģʽ                */
#define ARCH_ARM_IRQ32MODE          0x12                                /*  �ж�ģʽ                    */
#define ARCH_ARM_SVC32MODE          0x13                                /*  ����ģʽ                    */
#define ARCH_ARM_ABT32MODE          0x17                                /*  ��ֹģʽ                    */
#define ARCH_ARM_UND32MODE          0x1b                                /*  δ����ģʽ                  */
#define ARCH_ARM_SYS32MODE          0x1f                                /*  ϵͳģʽ                    */
#define ARCH_ARM_MASKMODE           0x1f                                /*  ģʽ����                    */
#define ARCH_ARM_DIS_FIQ            0x40                                /*  �ر� FIQ �ж�               */
#define ARCH_ARM_DIS_IRQ            0x80                                /*  �ر� IRQ �ж�               */

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/

typedef UINT        ARCH_REG_T;

typedef struct {
#if defined(__SYLIXOS_ARM_ARCH_M__)
    ARCH_REG_T      REG_uiBASEPRI;
    ARCH_REG_T      REG_uiR4;
    ARCH_REG_T      REG_uiR5;
    ARCH_REG_T      REG_uiR6;
    ARCH_REG_T      REG_uiR7;
    ARCH_REG_T      REG_uiR8;
    ARCH_REG_T      REG_uiR9;
    ARCH_REG_T      REG_uiR10;
    ARCH_REG_T      REG_uiR11;
    ARCH_REG_T      REG_uiExcRet;

    ARCH_REG_T      REG_uiR0;
    ARCH_REG_T      REG_uiR1;
    ARCH_REG_T      REG_uiR2;
    ARCH_REG_T      REG_uiR3;
    ARCH_REG_T      REG_uiR12;
    ARCH_REG_T      REG_uiR14;
    ARCH_REG_T      REG_uiR15;
    ARCH_REG_T      REG_uiXpsr;

#define REG_uiFp            REG_uiR11
#define REG_uiIp            REG_uiR12
#define REG_uiLr            REG_uiR14
#define REG_uiPc            REG_uiR15
#define REG_uiCpsr          REG_uiXpsr

#else
    ARCH_REG_T      REG_uiCpsr;
    ARCH_REG_T      REG_uiR0;
    ARCH_REG_T      REG_uiR1;
    ARCH_REG_T      REG_uiR2;
    ARCH_REG_T      REG_uiR3;
    ARCH_REG_T      REG_uiR4;
    ARCH_REG_T      REG_uiR5;
    ARCH_REG_T      REG_uiR6;
    ARCH_REG_T      REG_uiR7;
    ARCH_REG_T      REG_uiR8;
    ARCH_REG_T      REG_uiR9;
    ARCH_REG_T      REG_uiR10;
    ARCH_REG_T      REG_uiFp;
    ARCH_REG_T      REG_uiIp;
    ARCH_REG_T      REG_uiLr;
    ARCH_REG_T      REG_uiPc;
#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
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

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiPc)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiFp)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  ��׼ȷ, ��Ϊ�˼��������    */

/*********************************************************************************************************
  ��ջ�еļĴ�����Ϣ��ռ��С
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  18
#define ARCH_STK_MIN_WORD_SIZE  128

/*********************************************************************************************************
  CPU �ֳ�
*********************************************************************************************************/

#define LW_CFG_CPU_WORD_LENGHT  32                                      /*  CPU �ֳ�                    */

#endif                                                                  /*  __ARM_ARCH_REGS_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
