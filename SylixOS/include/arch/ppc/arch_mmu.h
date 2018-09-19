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
  L4 ΢�ں������ MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  �Ƿ�ʹ�� L4 ����� MMU      */

/*********************************************************************************************************
  �Ƿ���Ҫ�ں˳��� 3 ��ҳ��֧��
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_4L_EN                 0                         /*  �Ƿ���Ҫ 4 ��ҳ��֧��       */

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
  �����ڴ��������
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_CFG_VMM_ZONE_NUM                   8                         /*  ���������                  */
#define LW_CFG_VMM_VIR_NUM                    8                         /*  ���������                  */

/*********************************************************************************************************
  MMU ת����Ŀ����
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

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
         * ����ֵ���� TLB MISS ʱ��װ�� MAS2 MAS3 MAS7 �Ĵ���
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

#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
        UINT        MAS7_uiReserved0: 28;                               /*  ����                        */
        UINT        MAS7_uiHigh4RPN :  4;                               /*  �� 4 λ����ҳ��             */
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/
    };                                                                  /*  E500 PTE                    */

    UINT32          MAS3_uiValue;                                       /*  MAS3 ֵ                     */
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
    UINT32          MAS7_uiValue;                                       /*  MAS7 ֵ                     */
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/
} LW_PTE_TRANSENTRY;                                                    /*  ҳ����Ŀ����                */

/*********************************************************************************************************
  E500 TLB1 ��Ŀӳ������
*********************************************************************************************************/

typedef struct {
    UINT64                   TLB1D_ui64PhyAddr;                         /*  �����ַ (ҳ�����ַ)       */
    ULONG                    TLB1D_ulVirMap;                            /*  ��Ҫ��ʼ����ӳ���ϵ        */
    ULONG                    TLB1D_stSize;                              /*  �����ڴ������� (ҳ���볤��) */
    ULONG                    TLB1D_ulFlag;                              /*  �����ڴ���������            */
#define E500_TLB1_FLAG_VALID               0x01                         /*  ӳ����Ч                    */
#define E500_TLB1_FLAG_UNVALID             0x00                         /*  ӳ����Ч                    */

#define E500_TLB1_FLAG_ACCESS              0x02                         /*  ���Է���                    */
#define E500_TLB1_FLAG_UNACCESS            0x00                         /*  ���ܷ���                    */

#define E500_TLB1_FLAG_WRITABLE            0x04                         /*  ����д����                  */
#define E500_TLB1_FLAG_UNWRITABLE          0x00                         /*  ������д����                */

#define E500_TLB1_FLAG_EXECABLE            0x08                         /*  ����ִ�д���                */
#define E500_TLB1_FLAG_UNEXECABLE          0x00                         /*  ������ִ�д���              */

#define E500_TLB1_FLAG_CACHEABLE           0x10                         /*  ���Ի���                    */
#define E500_TLB1_FLAG_UNCACHEABLE         0x00                         /*  �����Ի���                  */

#define E500_TLB1_FLAG_GUARDED             0x40                         /*  �����ϸ��Ȩ�޼��          */
#define E500_TLB1_FLAG_UNGUARDED           0x00                         /*  �������ϸ��Ȩ�޼��        */

#define E500_TLB1_FLAG_TEMP                0x80                         /*  ��ʱӳ��                    */

#define E500_TLB1_FLAG_MEM      (E500_TLB1_FLAG_VALID    | \
                                 E500_TLB1_FLAG_ACCESS   | \
                                 E500_TLB1_FLAG_WRITABLE | \
                                 E500_TLB1_FLAG_EXECABLE | \
                                 E500_TLB1_FLAG_CACHEABLE)              /*  ��ͨ�ڴ�                    */

#define E500_TLB1_FLAG_BOOTSFR  (E500_TLB1_FLAG_VALID      | \
                                 E500_TLB1_FLAG_GUARDED    | \
                                 E500_TLB1_FLAG_ACCESS     | \
                                 E500_TLB1_FLAG_WRITABLE   | \
                                 E500_TLB1_FLAG_UNEXECABLE | \
                                 E500_TLB1_FLAG_UNCACHEABLE)            /*  ���⹦�ܼĴ���              */
} E500_TLB1_MAP_DESC;
typedef E500_TLB1_MAP_DESC      *PE500_TLB1_MAP_DESC;

/*********************************************************************************************************
  E500 bsp ��Ҫ�������º�����ʼ�� TLB1
*********************************************************************************************************/

INT  archE500MmuTLB1GlobalMap(CPCHAR               pcMachineName,
                              PE500_TLB1_MAP_DESC  pdesc,
                              VOID                 (*pfuncPreRemoveTempMap)(VOID));

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __PPC_ARCH_MMU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
