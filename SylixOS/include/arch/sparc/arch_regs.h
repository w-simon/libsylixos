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
** ��   ��   ��: Xu.Guizhou (�����)
**
** �ļ���������: 2017 �� 05 �� 15 ��
**
** ��        ��: SPARC �Ĵ������.
**
*********************************************************************************************************/

#ifndef __SPARC_ARCH_REGS_H
#define __SPARC_ARCH_REGS_H

/*********************************************************************************************************
  �Ĵ�����
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT        ARCH_REG_T;

typedef struct {
#define REG_uiFp    REG_uiInput[6]
#define REG_uiSp    REG_uiOutput[6]
    ARCH_REG_T      REG_uiGlobal[8];                                    /*  Global regs                 */
    ARCH_REG_T      REG_uiOutput[8];                                    /*  Output regs                 */
    ARCH_REG_T      REG_uiLocal[8];                                     /*  Local regs                  */
    ARCH_REG_T      REG_uiInput[8];                                     /*  Input regs                  */
    ARCH_REG_T      REG_uiPsr;                                          /*  Psr reg                     */
    ARCH_REG_T      REG_uiTbr;                                          /*  Tbr reg                     */
    ARCH_REG_T      REG_uiPc;                                           /*  Pc reg                      */
    ARCH_REG_T      REG_uiNPc;                                          /*  NPc reg                     */
    ARCH_REG_T      REG_uiWim;                                          /*  wim reg                     */
    ARCH_REG_T      REG_uiY;                                            /*  Y reg                       */
} ARCH_REG_CTX;

/*********************************************************************************************************
  ���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T      FP_uiLocal[8];
    ARCH_REG_T      FP_uiInput[6];
    ARCH_REG_T      FP_uiFp;
    ARCH_REG_T      FP_uiRetAddr;
    ARCH_REG_T      FP_uiStructPtr;
    ARCH_REG_T      FP_uiXArgs[6];
    ARCH_REG_T      FP_uiXXArgs[1];
} ARCH_FP_CTX;

#endif                                                                  /*  !defined __ASSEMBLY__       */
/*********************************************************************************************************
  context stack frame offset
*********************************************************************************************************/

#define REG_GLOBAL(x)           (((x) + 0)  * 4)                        /*  Byte offset in ARCH_REG_CTX */
#define REG_OUTPUT(x)           (((x) + 8)  * 4)
#define REG_LOCAL(x)            (((x) + 16) * 4)
#define REG_INPUT(x)            (((x) + 24) * 4)
#define REG_PSR                 (32 * 4)
#define REG_TBR                 (REG_PSR + 4)
#define REG_PC                  (REG_TBR + 4)
#define REG_NPC                 (REG_PC  + 4)
#define REG_WIM                 (REG_NPC + 4)
#define REG_Y                   (REG_WIM + 4)

/*********************************************************************************************************
  window stack frame offset
*********************************************************************************************************/

#define SF_LOCAL(x)             (((x) + 0) * 4)
#define SF_INPUT(x)             (((x) + 8) * 4)

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiPc)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiFp)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  ��׼ȷ, ��Ϊ�˼��������    */

/*********************************************************************************************************
  ��ջ�еļĴ�����Ϣ��ռ��С
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE      ((sizeof(ARCH_REG_CTX) + sizeof(ARCH_FP_CTX)) / sizeof(ARCH_REG_T))
#define ARCH_STK_MIN_WORD_SIZE      96

#define ASM_STACK_FRAME_SIZE        ARCH_STK_MIN_WORD_SIZE
#define ASM_REG_STACK_FRAME_SIZE    ((38 + 2) * 4)
#define ASM_CTX_STACK_FRAME_SIZE    (ASM_REG_STACK_FRAME_SIZE + ASM_STACK_FRAME_SIZE)

/*********************************************************************************************************
  ��ջ����Ҫ��
*********************************************************************************************************/

#define ARCH_STK_ALIGN_SIZE         8

#endif                                                                  /*  __SPARC_ARCH_REGS_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
