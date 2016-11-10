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
** ��   ��   ��: mips64.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 11 �� 02 ��
**
** ��        ��: MIPS64 ��ϵ������غ�����.
*********************************************************************************************************/

#ifndef __MIPS_MIPS64_H
#define __MIPS_MIPS64_H

PCHAR   mips64MemDup(UINT64  ui64Addr, size_t  stLen);
VOID    mips64MemFree(PCHAR  pcBuffer);

UINT8   mips64Read8( UINT64  ui64Addr);
UINT16  mips64Read16(UINT64  ui64Addr);
UINT32  mips64Read32(UINT64  ui64Addr);
UINT64  mips64Read64(UINT64  ui64Addr);

VOID    mips64Write8( UINT8   ucData,   UINT64  ui64Addr);
VOID    mips64Write16(UINT16  usData,   UINT64  ui64Addr);
VOID    mips64Write32(UINT32  uiData,   UINT64  ui64Addr);
VOID    mips64Write64(UINT64  ui64Data, UINT64  ui64Addr);

#endif                                                                  /*  __MIPS_MIPS64_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
