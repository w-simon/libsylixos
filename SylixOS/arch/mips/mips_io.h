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
** ��   ��   ��: mips_io.h
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ���� I/O ���ʽӿ�.
*********************************************************************************************************/
#ifndef __ARCH_MIPS_IO_H
#define __ARCH_MIPS_IO_H

#include "endian.h"

/*********************************************************************************************************
  MIPS ������ I/O ���� (Non-Cache ���� SylixOS δʹ�õ�Ч�ʵ�ǿ����ʽ, ����������Ҫ�����ڴ�����)
*********************************************************************************************************/

#define KN_IO_MB()      KN_SMP_MB()
#define KN_IO_RMB()     KN_SMP_RMB()
#define KN_IO_WMB()     KN_SMP_WMB()

/*********************************************************************************************************
  MIPS ������ I/O �ڴ��
*********************************************************************************************************/

static LW_INLINE UINT8 read8 (addr_t  ulAddr)
{
    UINT8   ucVal = *(volatile UINT8 *)ulAddr;
    KN_IO_RMB();
    return  (ucVal);
}

static LW_INLINE UINT16 read16 (addr_t  ulAddr)
{
    UINT16  usVal = *(volatile UINT16 *)ulAddr;
    KN_IO_RMB();
    return  (usVal);
}

static LW_INLINE UINT32 read32 (addr_t  ulAddr)
{
    UINT32  uiVal = *(volatile UINT32 *)ulAddr;
    KN_IO_RMB();
    return  (uiVal);
}

static LW_INLINE UINT64 read64 (addr_t  ulAddr)
{
    UINT64  u64Val = *(volatile UINT64 *)ulAddr;
    KN_IO_RMB();
    return  (u64Val);
}

/*********************************************************************************************************
  MIPS ������ I/O �ڴ�� (��С�����)
*********************************************************************************************************/

#define read8_le(a)        read8(a)
#define read16_le(a)       le16toh(read16(a))
#define read32_le(a)       le32toh(read32(a))
#define read64_le(a)       le64toh(read64(a))

#define read8_be(a)        read8(a)
#define read16_be(a)       be16toh(read16(a))
#define read32_be(a)       be32toh(read32(a))
#define read64_be(a)       be64toh(read64(a))

/*********************************************************************************************************
  MIPS ������ I/O �ڴ�д
*********************************************************************************************************/

static LW_INLINE VOID write8 (UINT8  ucData, addr_t  ulAddr)
{
    KN_IO_WMB();
    *(volatile UINT8 *)ulAddr = ucData;
}

static LW_INLINE VOID write16 (UINT16  usData, addr_t  ulAddr)
{
    KN_IO_WMB();
    *(volatile UINT16 *)ulAddr = usData;
}

static LW_INLINE VOID write32 (UINT32  uiData, addr_t  ulAddr)
{
    KN_IO_WMB();
    *(volatile UINT32 *)ulAddr = uiData;
}

static LW_INLINE VOID write64 (UINT64  u64Data, addr_t  ulAddr)
{
    KN_IO_WMB();
    *(volatile UINT64 *)ulAddr = u64Data;
}

/*********************************************************************************************************
  MIPS ������ I/O �ڴ�д (��С�����)
*********************************************************************************************************/

#define write8_le(v, a)     write8(v, a)
#define write16_le(v, a)    write16(htole16(v), a)
#define write32_le(v, a)    write32(htole32(v), a)
#define write64_le(v, a)    write64(htole64(v), a)

#define write8_be(v, a)     write8(v, a)
#define write16_be(v, a)    write16(htobe16(v), a)
#define write32_be(v, a)    write32(htobe32(v), a)
#define write64_be(v, a)    write64(htobe64(v), a)

/*********************************************************************************************************
  MIPS ������ I/O �ڴ������� (�������Ե�����ַ)
*********************************************************************************************************/

static LW_INLINE VOID reads8 (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT8  *pucBuffer = (UINT8 *)pvBuffer;

    while (stCount > 0) {
        *pucBuffer++ = *(volatile UINT8 *)ulAddr;
        stCount--;
        KN_IO_RMB();
    }
}

static LW_INLINE VOID reads16 (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT16  *pusBuffer = (UINT16 *)pvBuffer;

    while (stCount > 0) {
        *pusBuffer++ = *(volatile UINT16 *)ulAddr;
        stCount--;
        KN_IO_RMB();
    }
}

static LW_INLINE VOID reads32 (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT32  *puiBuffer = (UINT32 *)pvBuffer;

    while (stCount > 0) {
        *puiBuffer++ = *(volatile UINT32 *)ulAddr;
        stCount--;
        KN_IO_RMB();
    }
}

static LW_INLINE VOID reads64 (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT64  *pu64Buffer = (UINT64 *)pvBuffer;

    while (stCount > 0) {
        *pu64Buffer++ = *(volatile UINT64 *)ulAddr;
        stCount--;
        KN_IO_RMB();
    }
}

/*********************************************************************************************************
  MIPS ������ I/O �ڴ�����д (����д�뵥����ַ)
*********************************************************************************************************/

static LW_INLINE VOID writes8 (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT8  *pucBuffer = (UINT8 *)pvBuffer;

    while (stCount > 0) {
        KN_IO_WMB();
        *(volatile UINT8 *)ulAddr = *pucBuffer++;
        stCount--;
    }
}

static LW_INLINE VOID writes16 (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT16  *pusBuffer = (UINT16 *)pvBuffer;

    while (stCount > 0) {
        KN_IO_WMB();
        *(volatile UINT16 *)ulAddr = *pusBuffer++;
        stCount--;
    }
}

static LW_INLINE VOID writes32 (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT32  *puiBuffer = (UINT32 *)pvBuffer;

    while (stCount > 0) {
        KN_IO_WMB();
        *(volatile UINT32 *)ulAddr = *puiBuffer++;
        stCount--;
    }
}

static LW_INLINE VOID writes64 (addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount)
{
    REGISTER UINT64  *pu64Buffer = (UINT64 *)pvBuffer;

    while (stCount > 0) {
        KN_IO_WMB();
        *(volatile UINT64 *)ulAddr = *pu64Buffer++;
        stCount--;
    }
}

/*********************************************************************************************************
  MIPS ������ I/O �˿ڲ��� (MIPS ������û�ж��� I/O �˿�, �������� I/O �ڴ���ͬ)
*********************************************************************************************************/

#define in8         read8
#define in16        read16
#define in32        read32
#define in64        read64

#define in8_le      read8_le
#define in16_le     read16_le
#define in32_le     read32_le
#define in64_le     read64_le

#define in8_be      read8_be
#define in16_be     read16_be
#define in32_be     read32_be
#define in64_be     read64_be

#define out8        write8
#define out16       write16
#define out32       write32
#define out64       write64

#define out8_le     write8_le
#define out16_le    write16_le
#define out32_le    write32_le
#define out64_le    write64_le

#define out8_be     write8_be
#define out16_be    write16_be
#define out32_be    write32_be
#define out64_be    write64_be

#define ins8        reads8
#define ins16       reads16
#define ins32       reads32
#define ins64       reads64

#define outs8       writes8
#define outs16      writes16
#define outs32      writes32
#define outs64      writes64

#endif                                                                  /*  __ARCH_MIPS_IO_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
