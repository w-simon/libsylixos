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
** ��   ��   ��: x86_io.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 06 �� 25 ��
**
** ��        ��: x86 ��ϵ���� I/O ���ʽӿ�.
*********************************************************************************************************/

#ifndef __ARCH_X86_IO_H
#define __ARCH_X86_IO_H

#include "endian.h"

/*********************************************************************************************************
  x86 ������ I/O ���� (Non-Cache ���� SylixOS δʹ�õ�Ч�ʵ�ǿ����ʽ, ����������Ҫ�����ڴ�����)
*********************************************************************************************************/

#define KN_IO_MB()      KN_SMP_MB()
#define KN_IO_RMB()     KN_SMP_RMB()
#define KN_IO_WMB()     KN_SMP_WMB()

/*********************************************************************************************************
  x86 ������ I/O �ڴ��
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
  x86 ������ I/O �ڴ�� (��С�����)
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
  x86 ������ I/O �ڴ�д
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
  x86 ������ I/O �ڴ�д (��С�����)
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
  x86 ������ I/O �ڴ������� (�������Ե�����ַ)
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
  x86 ������ I/O �ڴ�����д (����д�뵥����ַ)
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
  x86 ������ I/O �˿ڲ���
*********************************************************************************************************/

/*********************************************************************************************************
  x86 ������ I/O �˿ڶ�
*********************************************************************************************************/

UINT8  in8( addr_t  ulAddr);
UINT16 in16(addr_t  ulAddr);
UINT32 in32(addr_t  ulAddr);
UINT64 in64(addr_t  ulAddr);

/*********************************************************************************************************
  x86 ������ I/O �˿ڶ� (��С�����)
*********************************************************************************************************/

#define in8_le(a)        in8(a)
#define in16_le(a)       le16toh(in16(a))
#define in32_le(a)       le32toh(in32(a))
#define in64_le(a)       le64toh(in64(a))

#define in8_be(a)        in8(a)
#define in16_be(a)       be16toh(in16(a))
#define in32_be(a)       be32toh(in32(a))
#define in64_be(a)       be64toh(in64(a))

/*********************************************************************************************************
  x86 ������ I/O �˿�д
*********************************************************************************************************/

VOID out8( UINT8   ucData,  addr_t  ulAddr);
VOID out16(UINT16  usData,  addr_t  ulAddr);
VOID out32(UINT32  uiData,  addr_t  ulAddr);
VOID out64(UINT64  u64Data, addr_t  ulAddr);

/*********************************************************************************************************
  x86 ������ I/O �˿�д (��С�����)
*********************************************************************************************************/

#define out8_le(v, a)     out8(v, a)
#define out16_le(v, a)    out16(htole16(v), a)
#define out32_le(v, a)    out32(htole32(v), a)
#define out64_le(v, a)    out64(htole64(v), a)

#define out8_be(v, a)     out8(v, a)
#define out16_be(v, a)    out16(htobe16(v), a)
#define out32_be(v, a)    out32(htobe32(v), a)
#define out64_be(v, a)    out64(htobe64(v), a)

/*********************************************************************************************************
  x86 ������ I/O �˿������� (�������Ե�����ַ)
*********************************************************************************************************/

VOID ins8( addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID ins16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID ins32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID ins64(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);

/*********************************************************************************************************
  x86 ������ I/O �˿�����д (����д�뵥����ַ)
*********************************************************************************************************/

VOID outs8( addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID outs16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID outs32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
VOID outs64(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);

#endif                                                                  /*  __ARCH_X86_IO_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
