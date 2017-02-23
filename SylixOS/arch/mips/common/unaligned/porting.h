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
** ��   ��   ��: porting.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 02 �� 15 ��
**
** ��        ��: MIPS �Ƕ��봦����ֲͷ�ļ�.
*********************************************************************************************************/

#ifndef __ARCH_MIPSUNALIGNEDPORT_H
#define __ARCH_MIPSUNALIGNEDPORT_H

#ifdef __GNUC__
#define PTR         .word
#else
#define PTR         TODO
#endif

enum {
    VERIFY_READ,
    VERIFY_WRITE,
};

/*
 * ע��: ʼ����Ϊ���Է��ʣ������ַ���Ϸ������ܻ�����쳣Ƕ�ף�����ͨ���쳣Ƕ��ʱ��ӡ�ĵ���ջ����Ӧ��
 * ��������Ĵ���
 */
#define access_ok(type, va, len)     1

#define IS_ENABLED(config)      MIPS_##config

#define get_fs()                0
#define get_ds()                1
#define segment_eq(s1, s2)      0

extern ARCH_FPU_CTX   _G_mipsFpuCtx[LW_CFG_MAX_PROCESSORS];
extern int fpu_emulator_cop1Handler(ARCH_REG_CTX *xcp, ARCH_FPU_CTX *ctx,
                                    int has_fpu, void *__user *fault_addr);

#endif                                                                  /*  __ARCH_MIPSUNALIGNEDPORT_H  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
