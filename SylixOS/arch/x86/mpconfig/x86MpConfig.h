/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: x86MpConfig.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 29 ��
**
** ��        ��: x86 ��ϵ���� MP configuration table ���ͷ�ļ�.
*********************************************************************************************************/

#ifndef __ARCH_X86MPCONFIG_H
#define __ARCH_X86MPCONFIG_H

/*********************************************************************************************************
  �ڲ����Ͷ���
*********************************************************************************************************/
/*********************************************************************************************************
  See MultiProcessor Specification Version 1.[14]
*********************************************************************************************************/
struct mp {
    UINT8       signature[4];                               /*  "_MP_"                                  */
    UINT32      physaddr;                                   /*  Phys addr of MP config table            */
    UINT8       length;                                     /*  1                                       */
    UINT8       specrev;                                    /*  [14]                                    */
    UINT8       checksum;                                   /*  All bytes must add up to 0              */
    UINT8       type;                                       /*  MP system config type                   */
    UINT8       imcrp;
    UINT8       reserved[3];
};
typedef struct mp           X86_MP, *PX86_MP;

struct mpconf {                                             /*  Configuration table header              */
    UINT8       signature[4];                               /*  "PCMP"                                  */
    UINT16      length;                                     /*  Total table length                      */
    UINT8       version;                                    /*  [14]                                    */
    UINT8       checksum;                                   /*  All bytes must add up to 0              */
    UINT8       product[20];                                /*  Product id                              */
    UINT32      oemtable;                                   /*  OEM table pointer                       */
    UINT16      oemlength;                                  /*  OEM table length                        */
    UINT16      entry;                                      /*  Entry count                             */
    UINT32      lapicaddr;                                  /*  Address of local APIC                   */
    UINT16      xlength;                                    /*  Extended table length                   */
    UINT8       xchecksum;                                  /*  Extended table checksum                 */
    UINT8       reserved;
};
typedef struct mpconf       X86_MP_CONFIG, *PX86_MP_CONFIG;

struct mpproc {                                             /*  Processor table entry                   */
    UINT8       type;                                       /*  Entry type (0)                          */
    UINT8       apicid;                                     /*  Local APIC id                           */
    UINT8       version;                                    /*  Local APIC verison                      */
    UINT8       flags;                                      /*  CPU flags                               */
#define MPBOOT      0x02                                    /*  This proc is the bootstrap processor.   */
    UINT8       signature[4];                               /*  CPU signature                           */
    UINT32      feature;                                    /*  Feature flags from CPUID instruction    */
    UINT8       reserved[8];
};
typedef struct mpproc       X86_MP_PROC, *PX86_MP_PROC;

struct mpioapic {                                           /*  I/O APIC table entry                    */
    UINT8       type;                                       /*  Entry type (2)                          */
    UINT8       apicno;                                     /*  I/O APIC id                             */
    UINT8       version;                                    /*  I/O APIC version                        */
    UINT8       flags;                                      /*  I/O APIC flags                          */
    UINT32      addr;                                       /*  I/O APIC address                        */
};
typedef struct mpioapic     X86_MP_IOAPIC, *PX86_MP_IOAPIC;

struct mpinterrupt {
    UINT8       type;
    UINT8       irqtype;
    UINT16      irqflag;
    UINT8       srcbus;
    UINT8       srcbusirq;
    UINT8       dstapic;
    UINT8       dstirq;
};
typedef struct mpinterrupt  X86_MP_INTERRUPT, *PX86_MP_INTERRUPT;

/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/

extern X86_MP_INTERRUPT     _G_x86MpInterrupt[];
extern INT                  _G_iX86MpInterruptNr;

#endif                                                                  /*  __ARCH_X86MPCONFIG_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
