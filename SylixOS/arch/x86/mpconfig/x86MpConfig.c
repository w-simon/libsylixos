/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86MpConfig.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 MP configuration table.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86MpConfig.h"
#include "arch/x86/common/x86CpuId.h"
/*********************************************************************************************************
  Table entry types
*********************************************************************************************************/
#define MPPROC                  0x00                                /*  One per processor               */
#define MPBUS                   0x01                                /*  One per bus                     */
#define MPIOAPIC                0x02                                /*  One per I/O APIC                */
#define MPIOINTR                0x03                                /*  One per bus interrupt source    */
#define MPLINTR                 0x04                                /*  One per system interrupt source */

#define MP_MAX_INTERRUPT_NR     64
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
X86_MP_INTERRUPT            _G_x86MpInterrupt[MP_MAX_INTERRUPT_NR] = { { 0 }, };
INT                         _G_iX86MpInterruptNr = 0;
/*********************************************************************************************************
** 函数名称: x86MpSum
** 功能描述: 计算效验和
** 输　入  : pucAddr       地址
**           stLen         长度
** 输　出  : 效验和
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  x86MpSum (UINT8  *pucAddr, size_t  stLen)
{
    INT    i;
    UINT8  ucSum;

    for (ucSum = 0, i = 0; i < stLen; i++) {
        ucSum += pucAddr[i];
    }

    return  (ucSum);
}
/*********************************************************************************************************
** 函数名称: x86MpScanMpStruct
** 功能描述: 扫描 MP 结构
** 输　入  : ulAddr        地址
**           stLen         长度
** 输　出  : MP 结构地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PX86_MP  x86MpScanMpStruct (addr_t  ulAddr, size_t  stLen)
{
    UINT8  *pucEnd, *pucPos, *pucAddr;

    pucAddr = (UINT8 *)ulAddr;
    pucEnd  = pucAddr + stLen;

    for (pucPos = pucAddr; pucPos < pucEnd; pucPos += sizeof(X86_MP)) {
        if (lib_memcmp(pucPos, "_MP_", 4) == 0 && x86MpSum(pucPos, sizeof(X86_MP)) == 0) {
            return  ((PX86_MP)pucPos);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: x86MpFindMpStruct
** 功能描述: 查找 MP 结构
** 输　入  : ulAddr        地址
**           stLen         长度
** 输　出  : MP 结构地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PX86_MP  x86MpFindMpStruct (VOID)
{
    UINT8    *pucEBDA;
    addr_t    ulAddr;
    PX86_MP   pmp;

    /*
     * Search for the MP Floating Pointer Structure, which according to the
     * spec is in one of the following three locations:
     * 1) in the first KB of the EBDA;
     * 2) in the last KB of system base memory;
     * 3) in the BIOS ROM address space between 0F0000h and 0FFFFFh.
     */
    pucEBDA = (UINT8 *)0x400;

    if ((ulAddr = ((pucEBDA[0x0F] << 8) | pucEBDA[0x0E]) << 4)) {
        if ((pmp = x86MpScanMpStruct(ulAddr, 1024))) {
            return  (pmp);
        }

    } else {
        ulAddr = ((pucEBDA[0x14] << 8) | pucEBDA[0x13]) * 1024;
        if ((pmp = x86MpScanMpStruct(ulAddr - 1024, 1024))) {
            return  (pmp);
        }
    }

    return  (x86MpScanMpStruct(0xF0000, 0x10000));
}
/*********************************************************************************************************
** 函数名称: x86MpGetMpConfig
** 功能描述: 获得 MP 配置
** 输　入  : ppmp      MP 结构地址
** 输　出  : MP 配置
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PX86_MP_CONFIG  x86MpGetMpConfig (PX86_MP  *ppmp)
{
    PX86_MP_CONFIG  pmpconfig;
    PX86_MP         pmp;

    /*
     * Search for an MP configuration table.  For now,
     * don't accept the default configurations (physaddr == 0).
     * Check for correct signature, calculate the checksum and,
     * if correct, check the version.
     * To do: check extended table checksum.
     */

    if ((pmp = x86MpFindMpStruct()) == 0 || pmp->physaddr == 0) {
        return  (LW_NULL);
    }

    pmpconfig = (PX86_MP_CONFIG)pmp->physaddr;
    if (lib_memcmp(pmpconfig, "PCMP", 4) != 0) {
        return  (LW_NULL);
    }

    if (pmpconfig->version != 1 && pmpconfig->version != 4) {
        return  (LW_NULL);
    }

    if (x86MpSum((UINT8 *)pmpconfig, pmpconfig->length) != 0) {
        return  (LW_NULL);
    }

    *ppmp = pmp;

    return  (pmpconfig);
}
/*********************************************************************************************************
** 函数名称: x86MpInit
** 功能描述: 初始化 MP
** 输　入  : bHyperThreading       是否使用超线程技术
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86MpInit (BOOL  bHyperThreading)
{
    UINT8              *pucPos, *pucEnd;
    PX86_MP             pmp = LW_NULL;
    PX86_MP_CONFIG      pmpconfig;
    PX86_MP_PROC        pmpproc;
    PX86_MP_INTERRUPT   pmpinterrupt;
    static BOOL         bInited = LW_FALSE;

    if (bInited) {
        return  (PX_ERROR);
    }

    if ((pmpconfig = x86MpGetMpConfig(&pmp)) == 0) {
        goto __return;
    }

    if (!_G_bX86HasHTT) {
        bHyperThreading = LW_FALSE;
    }

    pucPos = (UINT8 *)(pmpconfig + 1);
    pucEnd = (UINT8 *)pmpconfig + pmpconfig->length;

    for (; pucPos < pucEnd; ) {
        switch (*pucPos) {

        case MPIOINTR:
            if (_G_iX86MpInterruptNr < MP_MAX_INTERRUPT_NR) {
                pmpinterrupt = (PX86_MP_INTERRUPT)pucPos;

                _G_x86MpInterrupt[_G_iX86MpInterruptNr] = *pmpinterrupt;

                _G_iX86MpInterruptNr++;
            }
            pucPos += sizeof(X86_MP_INTERRUPT);
            continue;

        case MPPROC:
            if (_G_iX86ProcNr < (2 * LW_CFG_MAX_PROCESSORS)) {
                pmpproc = (PX86_MP_PROC)pucPos;

                _G_x86ProcInfo[pmpproc->apicid].PROC_bPresent       = LW_TRUE;
                _G_x86ProcInfo[pmpproc->apicid].PROC_ulCPUId        = _G_iX86ProcNr;
                _G_x86ProcInfo[pmpproc->apicid].PROC_ucLocalApicId  = pmpproc->apicid;
                _G_x86ProcInfo[_G_iX86ProcNr].PROC_ucMapLocalApicId = pmpproc->apicid;
                _G_iX86ProcNr++;

                if (bHyperThreading) {
                    _G_x86ProcInfo[pmpproc->apicid + 1].PROC_bPresent      = LW_TRUE;
                    _G_x86ProcInfo[pmpproc->apicid + 1].PROC_ulCPUId       = _G_iX86ProcNr;
                    _G_x86ProcInfo[pmpproc->apicid + 1].PROC_ucLocalApicId = pmpproc->apicid + 1;
                    _G_x86ProcInfo[_G_iX86ProcNr].PROC_ucMapLocalApicId    = pmpproc->apicid + 1;
                    _G_iX86ProcNr++;
                }
            }
            pucPos += sizeof(X86_MP_PROC);
            continue;

        case MPIOAPIC:
            pucPos += sizeof(X86_MP_IOAPIC);
            continue;

        case MPBUS:
        case MPLINTR:
            pucPos += 8;
            continue;

        default:
            continue;
        }
    }

__return:
    if (_G_iX86ProcNr == 0) {
        _G_x86ProcInfo[0].PROC_bPresent         = LW_TRUE;
        _G_x86ProcInfo[0].PROC_ulCPUId          = 0;
        _G_x86ProcInfo[0].PROC_ucLocalApicId    = 0;
        _G_x86ProcInfo[0].PROC_ucMapLocalApicId = 0;
        _G_iX86ProcNr = 1;

    } else if (_G_iX86ProcNr > 1) {
        if (pmp && pmp->imcrp) {                                        /*  It run on real hardware     */
            out8(0x70, 0x22);                                           /*  Select IMCR                 */
            out8(in8(0x23) | 1, 0x23);                                  /*  Mask external interrupts.   */
        }
    }

    bInited = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
