/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: vmmIo.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 05 月 21 日
**
** 描        述: 平台无关虚拟内存管理, 设备内存映射.
*********************************************************************************************************/

#ifndef __VMMIO_H
#define __VMMIO_H

/*********************************************************************************************************
  加入裁剪支持 (推荐使用第二套接口)
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

LW_API PVOID    API_VmmIoRemap(PVOID  pvPhysicalAddr, size_t stSize);
LW_API PVOID    API_VmmIoRemapEx(PVOID  pvPhysicalAddr, size_t stSize, ULONG  ulFlags);
LW_API PVOID    API_VmmIoRemapNocache(PVOID  pvPhysicalAddr, size_t stSize);
LW_API VOID     API_VmmIoUnmap(PVOID  pvVirtualAddr);

LW_API addr_t   API_VmmIoRemap2(phys_addr_t  paPhysicalAddr, size_t stSize);
LW_API addr_t   API_VmmIoRemapEx2(phys_addr_t  paPhysicalAddr, size_t stSize, ULONG  ulFlags);
LW_API addr_t   API_VmmIoRemapNocache2(phys_addr_t  paPhysicalAddr, size_t stSize);
LW_API VOID     API_VmmIoUnmap2(addr_t  ulVirtualAddr);

/*********************************************************************************************************
  无 VMM 支持
*********************************************************************************************************/
#else

static LW_INLINE PVOID  API_VmmIoRemap (PVOID  pvPhysicalAddr, size_t stSize)
{
    return  (pvPhysicalAddr);
}

static LW_INLINE PVOID  API_VmmIoRemapEx (PVOID  pvPhysicalAddr, size_t stSize, ULONG  ulFlags)
{
    return  (pvPhysicalAddr);
}

static LW_INLINE PVOID  API_VmmIoRemapNocache (PVOID  pvPhysicalAddr, size_t stSize)
{
    return  (pvPhysicalAddr);
}

static LW_INLINE VOID  API_VmmIoUnmap (PVOID  pvVirtualAddr)
{
}

static LW_INLINE addr_t  API_VmmIoRemap2 (phys_addr_t  paPhysicalAddr, size_t stSize)
{
    return  ((addr_t)paPhysicalAddr);
}

static LW_INLINE addr_t  API_VmmIoRemapEx2 (phys_addr_t  paPhysicalAddr, size_t stSize, ULONG  ulFlags)
{
    return  ((addr_t)paPhysicalAddr);
}

static LW_INLINE addr_t  API_VmmIoRemapNocache2 (phys_addr_t  paPhysicalAddr, size_t stSize)
{
    return  ((addr_t)paPhysicalAddr);
}

static LW_INLINE VOID  API_VmmIoUnmap2 (addr_t  ulVirtualAddr)
{
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#define vmmIoRemap              API_VmmIoRemap
#define vmmIoRemapEx            API_VmmIoRemapEx
#define vmmIoUnmap              API_VmmIoUnmap
#define vmmIoRemapNocache       API_VmmIoRemapNocache

#define vmmIoRemap2             API_VmmIoRemap2
#define vmmIoRemapEx2           API_VmmIoRemapEx2
#define vmmIoUnmap2             API_VmmIoUnmap2
#define vmmIoRemapNocache2      API_VmmIoRemapNocache2

#endif                                                                  /*  __VMMIO_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
