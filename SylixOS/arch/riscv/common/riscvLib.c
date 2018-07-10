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
** 文   件   名: riscvLib.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 03 月 20 日
**
** 描        述: RISC-V 体系构架内部库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  位图表
*********************************************************************************************************/
static const UINT8  ucLsbBitmap[] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

static const UINT8  ucMsbBitmap[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};
/*********************************************************************************************************
** 函数名称: archLsb
** 功能描述: find least significant bit set 寻找 32 位数中最低的 1 位
** 输　入  : ui32      32 位数
** 输　出  : 最低位为 1 的位置
**           正确返回 [1 ~ 32], 如果参数为全 0 则返回 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archFindLsb (UINT32 ui32)
{
    UINT16  usMsw = (UINT16)(ui32 >> 16);
    UINT16  usLsw = (UINT16)(ui32 & 0xffff);
    UINT8   ucByte;

    if (ui32 == 0) {
        return (0);
    }

    if (usLsw) {
        ucByte = (UINT8)(usLsw & 0xff);
        if (ucByte) {                                                   /*  byte is bits [0:7]          */
            return (ucLsbBitmap[ucByte] + 1);
        
        } else {                                                        /*  byte is bits [8:15]         */
            return (ucLsbBitmap[(UINT8)(usLsw >> 8)] + 8 + 1);
        }
    
    } else {
        ucByte = (UINT8)(usMsw & 0xff);                                 /*  byte is bits [16:23]        */
        if (ucByte) {
            return (ucLsbBitmap[ucByte] + 16 + 1);
        
        } else {                                                        /*  byte is bits [24:31]        */
            return (ucLsbBitmap[(UINT8)(usMsw >> 8)] + 24 + 1);
        }
    }
}
/*********************************************************************************************************
** 函数名称: archMsb
** 功能描述: find most significant bit set 寻找 32 位数中最高的 1 位
** 输　入  : ui32      32 位数
** 输　出  : 最高位为 1 的位置
**           正确返回 [1 ~ 32], 如果参数为全 0 则返回 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archFindMsb (UINT32 ui32)
{
    UINT16  usMsw = (UINT16)(ui32 >> 16);
    UINT16  usLsw = (UINT16)(ui32 & 0xffff);
    UINT8   ucByte;
    
    if (ui32 == 0) {
        return  (0);
    }
    
    if (usMsw) {
        ucByte = (UINT8)(usMsw >> 8);                                   /*  byte is bits [24:31]        */
        if (ucByte) {
            return (ucMsbBitmap[ucByte] + 24 + 1);
        
        } else {
            return (ucMsbBitmap[(UINT8)usMsw] + 16 + 1);
        }
    
    } else {
        ucByte = (UINT8)(usLsw >> 8);                                   /*  byte is bits [8:15]         */
        if (ucByte) {
            return (ucMsbBitmap[ucByte] + 8 + 1);
        
        } else {
            return (ucMsbBitmap[(UINT8)usLsw] + 1);
        }
    }
}
/*********************************************************************************************************
** 函数名称: archPageCopy
** 功能描述: 拷贝一个页面
** 输　入  : pvTo      目标
**           pvFrom    源
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archPageCopy (PVOID  pvTo, PVOID  pvFrom)
{
    REGISTER INT      i;
    REGISTER UINT64  *pu64To   = (UINT64 *)pvTo;
    REGISTER UINT64  *pu64From = (UINT64 *)pvFrom;

    for (i = 0; i < (LW_CFG_VMM_PAGE_SIZE >> 3); i += 16) {             /*  4KB PAGE SIZE               */
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
        *pu64To++ = *pu64From++;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
