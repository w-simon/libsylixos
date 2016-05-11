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
** ��   ��   ��: arch_mmu.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 11 �� 26 ��
**
** ��        ��: PowerPC �ڴ�������.
*********************************************************************************************************/

#ifndef __PPC_ARCH_MMU_H
#define __PPC_ARCH_MMU_H

/*********************************************************************************************************
  �����ڴ�ҳ���������
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_SHIFT                 12                        /*  2^12 = 4096                 */
#define LW_CFG_VMM_PAGE_SIZE                  (1ul << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PMD_SHIFT                  20                        /*  NO PMD same as PGD          */
#define LW_CFG_VMM_PMD_SIZE                   (1ul << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (~(LW_CFG_VMM_PMD_SIZE - 1))

#define LW_CFG_VMM_PGD_SHIFT                  20                        /*  2^20 = 1MB                  */
#define LW_CFG_VMM_PGD_SIZE                   (1ul << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (~(LW_CFG_VMM_PGD_SIZE - 1))

/*********************************************************************************************************
  ת����Ŀ���, ÿһ���������ı������
*********************************************************************************************************/

#define LW_CFG_VMM_PTRS_PER_PGD               256                       /*  PGD ӵ�е�ת����Ŀ��        */
#define LW_CFG_VMM_PTRS_PER_PMD               1                         /*  PMD ӵ�е�ת����Ŀ��        */
#define LW_CFG_VMM_PTRS_PER_PTE               1024                      /*  PTE ӵ�е�ת����Ŀ��        */

/*********************************************************************************************************
  MMU ת����Ŀ����
*********************************************************************************************************/

typedef UINT32  LW_PGD_TRANSENTRY;                                      /*  ҳĿ¼����                  */
typedef UINT32  LW_PMD_TRANSENTRY;                                      /*  �м�ҳĿ¼����              */

typedef union {
    struct {
        UINT        PTE_uiRPN       : 20;                               /*  ����ҳ��                    */
        UINT        PTE_ucReserved1 :  3;                               /*  ����                        */
        UINT        PTE_bRef        :  1;                               /*  ����λ                      */
        UINT        PTE_bChange     :  1;                               /*  �޸�λ                      */
        UINT        PTE_ucWIMG      :  4;                               /*  �ڴ�� CACHE ����λ         */
        UINT        PTE_bReserved2  :  1;                               /*  ����                        */
        UINT        PTE_ucPP        :  2;                               /*  ҳ����Ȩ��λ                */
    };                                                                  /*  ͨ�õ� PPC32 PTE            */
    UINT32          PTE_uiValue;                                        /*  ֵ                          */

    struct {
        /*
         * ����ֵ���� TLB MISS ʱ��װ�� MAS2 MAS3 �Ĵ���
         */
        UINT        MAS3_uiRPN      : 20;                               /*  ����ҳ��                    */

        UINT        MAS3_ucReserved0:  2;                               /*  ����                        */

        /*
         * �����û��������� TLB MISS ʱ��װ�� MAS2 �Ĵ���
         *
         * MAS2 �Ĵ������� X0 X1 G E λ, G �� E λ�̶�Ϊ 0
         * X0 X1 λ�� MAS4 �Ĵ����� X0D X1D �Զ���װ
         */
#define MAS3_bValid      MAS3_bUserAttr0                                /*  �Ƿ���Ч                    */
#define MAS3_bWT         MAS3_bUserAttr1                                /*  �Ƿ�д��͸                  */
#define MAS3_bUnCache    MAS3_bUserAttr2                                /*  �Ƿ񲻿� Cache              */
#define MAS3_bMemCoh     MAS3_bUserAttr3                                /*  �Ƿ����ڴ�һ����          */

        UINT        MAS3_bUserAttr0 :  1;                               /*  �û����� 0                  */
        UINT        MAS3_bUserAttr1 :  1;                               /*  �û����� 1                  */
        UINT        MAS3_bUserAttr2 :  1;                               /*  �û����� 2                  */
        UINT        MAS3_bUserAttr3 :  1;                               /*  �û����� 3                  */

        UINT        MAS3_bUserExec  :  1;                               /*  �û���ִ��Ȩ��              */
        UINT        MAS3_bSuperExec :  1;                               /*  ����Ա��ִ��Ȩ��            */

        UINT        MAS3_bUserWrite :  1;                               /*  �û���дȨ��                */
        UINT        MAS3_bSuperWrite:  1;                               /*  ����Ա��дȨ��              */

        UINT        MAS3_bUserRead  :  1;                               /*  �û��ɶ�Ȩ��                */
        UINT        MAS3_bSuperRead :  1;                               /*  ����Ա�ɶ�Ȩ��              */
    };                                                                  /*  E500 PTE                    */
    UINT32          MAS3_uiValue;                                       /*  ֵ                          */
} LW_PTE_TRANSENTRY;                                                    /*  ҳ����Ŀ����                */

#endif                                                                  /*  __PPC_ARCH_MMU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
