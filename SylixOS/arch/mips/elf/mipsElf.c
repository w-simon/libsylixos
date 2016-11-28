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
** ��   ��   ��: mipsElf.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: ʵ�� MIPS ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef   LW_CFG_CPU_ARCH_MIPS                                            /*  MIPS ��ϵ�ṹ              */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
#include "../SylixOS/loader/include/loader_symbol.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define JMP_TABLE_ITEMLEN       8                                       /*  ��ת����Ŀ����              */
#define LOW16_VALUE(val)        ((val) & 0x0000FFFF)                    /*  Get LOW 16 Bit              */
#define SIGN_LOW16_VALUE(val)   (((LOW16_VALUE(val)^0x8000) - 0x8000))  /*  Get Sign LOW 16 Bit         */
/*********************************************************************************************************
** ��������: mipsElfRelocateRela
** ��������: �ض�λ RELA ���͵��ض�λ��
** ��  ��  : prela        RELA ����
**           addrSymVal   �ض�λ���ŵ�ֵ
**           pcTargetSec  �ض�λĿĿ�����
**           pcBuffer     ��ת����ʼ��ַ
**           stBuffLen    ��ת����
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfRelocateRela (PVOID       pmodule,
                          Elf_Rela   *prela,
                          Elf_Addr    addrSymVal,
                          PCHAR       pcTargetSec,
                          PCHAR       pcBuffer,
                          size_t      stBuffLen)
{
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mipsElfHI16RelocateRel
** ��������: �ض�λ R_MIPS_HI16���͵��ض�λ��
** ��  ��  : pmodule      Module������
**           pRelocAdrs   �ض�λ��ַ
**           addrSymVal   �ض�λ���ŵ�ֵ
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT mipsElfHI16RelocateRel (LW_LD_EXEC_MODULE  *pmodule,
                                   Elf_Addr           *pRelocAdrs,
                                   Elf_Addr            addrSymVal)
{
    PMIPS_HI16_RELOC_INFO   pHi16Info;

    pHi16Info = LW_LD_SAFEMALLOC(sizeof(MIPS_HI16_RELOC_INFO));
    if (!pHi16Info) {
        return  (PX_ERROR);
    }

    pHi16Info->HI16_pAddr    	= (Elf_Addr *)pRelocAdrs;
    pHi16Info->HI16_valAddr  	= addrSymVal;
    pHi16Info->HI16_pNext    	= pmodule->EMOD_pMIPSHi16List;        	/*  ����һ�� List �ڵ�          */
    pmodule->EMOD_pMIPSHi16List = pHi16Info;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsElfFreeHI16Relocatelist
** ��������: �ض�λ R_MIPS_LO16���͵��ض�λ��
** ��  ��  : pHi16Info   HI16_RELOC_INFO������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID mipsElfFreeHI16Relocatelist (PMIPS_HI16_RELOC_INFO  pHi16Info)
{
    PMIPS_HI16_RELOC_INFO  pNext;

    while (pHi16Info != NULL) {
        pNext = pHi16Info->HI16_pNext;
        LW_LD_SAFEFREE(pHi16Info);
        pHi16Info = pNext;
    }
}
/*********************************************************************************************************
** ��������: mipsElfLO16RelocateRel
** ��������: �ض�λ R_MIPS_LO16���͵��ض�λ��
** ��  ��  : pmodule      Module������
**           pRelocAdrs   �ض�λ��ַ
**           addrSymVal   �ض�λ���ŵ�ֵ
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT mipsElfLO16RelocateRel (LW_LD_EXEC_MODULE  *pmodule,
                                   Elf_Addr           *pRelocAdrs,
                                   Elf_Addr            addrSymVal)
{
    PMIPS_HI16_RELOC_INFO  pHi16Info;
    Elf_Addr               addrVal, addrValLO;
    Elf_Addr               addrInsnLO = *pRelocAdrs;

    addrValLO = SIGN_LOW16_VALUE(addrInsnLO);

    if (pmodule->EMOD_pMIPSHi16List != NULL) {
        pHi16Info = pmodule->EMOD_pMIPSHi16List;
        while (pHi16Info != NULL) {
            PMIPS_HI16_RELOC_INFO  pNext;
            Elf_Addr               addrInsn;

            /*
             * ��� HI16 �� LO16 ���ض�λ offset �Ƿ�һ��
             */
            if (addrSymVal != pHi16Info->HI16_valAddr) {
                mipsElfFreeHI16Relocatelist(pHi16Info);
                pmodule->EMOD_pMIPSHi16List = NULL;
                return  (PX_ERROR);
            }

            /*
             * �����ܵĵ�ַ
             */
            addrInsn  = *pHi16Info->HI16_pAddr;
            addrVal   = (LOW16_VALUE(addrInsn) << 16) + addrValLO;
            addrVal  += addrSymVal;

            /*
             * ��� BIT15 �ķ���ֵ(sign extension)
             */
            addrVal   = ((addrVal >> 16) + ((addrVal & 0x8000) != 0)) & 0xFFFF;

            addrInsn  = (addrInsn & ~0xFFFF) | addrVal;
            *pHi16Info->HI16_pAddr = addrInsn;

            pNext     = pHi16Info->HI16_pNext;
            LW_LD_SAFEFREE(pHi16Info);
            pHi16Info = pNext;
        }

        pmodule->EMOD_pMIPSHi16List = NULL;
    }

    addrVal     = addrSymVal + addrValLO;
    addrInsnLO  = (addrInsnLO & ~0xFFFF) | LOW16_VALUE(addrVal);
    *pRelocAdrs = addrInsnLO;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mipsElfREL32RelocateRel
** ��������: �ض�λ R_MIPS_REL32���͵��ض�λ��
** ��  ��  : pmodule      Module������
**           pRelocAdrs   �ض�λ��ַ
**           SymIndex     �ض�λ��������ֵ
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT mipsElfREL32RelocateRel (LW_LD_EXEC_MODULE  *pmodule,
                                    Elf_Addr           *pRelocAdrs,
                                    INT                 symIndex)
{
    ELF_DYN_DIR *pdyndir = (ELF_DYN_DIR *)(pmodule->EMOD_pvFormatInfo);

    if (symIndex) {
        if (symIndex < pdyndir->ulMIPSGotSymIdx) {
            *pRelocAdrs += (Elf_Addr)pmodule->EMOD_pvBaseAddr +
                            pdyndir->psymTable[symIndex].st_value;
        } else {
            *pRelocAdrs += pdyndir->ulPltGotAddr[symIndex + pdyndir->ulMIPSLocalGotNumIdx -
                                                 pdyndir->ulMIPSGotSymIdx];
        }
    } else {
        *pRelocAdrs += (Elf_Addr)pmodule->EMOD_pvBaseAddr;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfRelocateRel
** ��������: �ض�λ REL ���͵��ض�λ��
** ��  ��  : prel         REL ����
**           addrSymVal   �ض�λ���ŵ�ֵ
**           pcTargetSec  �ض�λĿĿ�����
**           pcBuffer     ��ת����ʼ��ַ
**           stBuffLen    ��ת����
** ��  ��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfRelocateRel (PVOID        pmodule,
                         Elf_Rel     *prel,
                         Elf_Addr     addrSymVal,
                         PCHAR        pcTargetSec,
                         PCHAR        pcBuffer,
                         size_t       stBuffLen)
{
    Elf_Addr  *paddrWhere;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prel->r_offset);    /*  �����ض�λĿ���ַ          */

    switch (ELF_R_TYPE(prel->r_info)) {

    case R_MIPS_NONE:
        break;

    case R_MIPS_32:
        *paddrWhere += (Elf_Addr)addrSymVal;
        break;

    case R_MIPS_REL32:
        mipsElfREL32RelocateRel((LW_LD_EXEC_MODULE *)pmodule,
                                paddrWhere,
                                ELF_R_SYM(prel->r_info));
        break;

    case R_MIPS_26:
        if (addrSymVal & 0x03) {
            return  (PX_ERROR);
        }
        if ((addrSymVal & 0xF0000000) != (((Elf_Addr)paddrWhere + 4) & 0xF0000000)) {
            return  (PX_ERROR);
        }
        *paddrWhere = (*paddrWhere & ~0x03FFFFFF) |
                      ((*paddrWhere + (addrSymVal >> 2)) & 0x03FFFFFF);
        break;

    case R_MIPS_HI16:
        mipsElfHI16RelocateRel((LW_LD_EXEC_MODULE *)pmodule,
                               paddrWhere,
                               addrSymVal);
        break;

    case R_MIPS_LO16:
        mipsElfLO16RelocateRel((LW_LD_EXEC_MODULE *)pmodule,
                               paddrWhere,
                               addrSymVal);
        break;

    case R_MIPS_JUMP_SLOT:
        *paddrWhere = (Elf_Addr)addrSymVal;
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prel->r_info));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfGotInit
** ��������: MIPS GOT �ض�λ
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT archElfGotInit (PVOID pmodule)
{
    Elf_Addr            *pMipsGotEntry;
    Elf_Sym             *pMipsSym;
    CHAR                *pchStrTab;
    Elf_Addr             addrSymVal;
    ULONG                ulTemp = 0;
    INT                  iIndex = 2;
    LW_LD_EXEC_MODULE   *pprivmodule = (LW_LD_EXEC_MODULE *)pmodule;
    ELF_DYN_DIR         *pprivdyndir = (ELF_DYN_DIR *)pprivmodule->EMOD_pvFormatInfo;

    pMipsGotEntry = pprivdyndir->ulPltGotAddr + iIndex;
    for (; iIndex < pprivdyndir->ulMIPSLocalGotNumIdx; iIndex++, pMipsGotEntry++) {
        *pMipsGotEntry += (Elf_Addr)pprivmodule->EMOD_pvBaseAddr;
    }

    pMipsGotEntry = pprivdyndir->ulPltGotAddr + pprivdyndir->ulMIPSLocalGotNumIdx;
    pMipsSym      = (Elf_Sym *)pprivdyndir->psymTable + pprivdyndir->ulMIPSGotSymIdx;
    pchStrTab     = (CHAR *)pprivdyndir->pcStrTable;
    ulTemp        = pprivdyndir->ulMIPSSymNumIdx - pprivdyndir->ulMIPSGotSymIdx;

    while (ulTemp--) {
        if (pMipsSym->st_shndx == SHN_UNDEF || pMipsSym->st_shndx == SHN_COMMON) {
            BOOL    bWeak = (STB_WEAK == ELF_ST_BIND(pMipsSym->st_info));

            if (__moduleSymGetValue(pprivmodule,
                                    bWeak,
                                    pchStrTab + pMipsSym->st_name,
                                    &addrSymVal,
                                    LW_LD_SYM_ANY) < 0) {
                return  (PX_ERROR);
            }
            *pMipsGotEntry = addrSymVal;

        } else if (ELF_ST_TYPE(pMipsSym->st_info) == STT_SECTION) {
            if (pMipsSym->st_other == 0) {
                *pMipsGotEntry += (Elf_Addr)pprivmodule->EMOD_pvBaseAddr;
            }
        } else {
            addrSymVal     = LW_LD_V2PADDR(pprivdyndir->addrMin,
                                           pprivmodule->EMOD_pvBaseAddr,
                                           pMipsSym->st_value);
            *pMipsGotEntry = addrSymVal;
        }

        pMipsGotEntry++;
        pMipsSym++;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: archElfRGetJmpBuffItemLen
** ��������: ������ת�����
** ��  ��  :
** ��  ��  : ��ת�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archElfRGetJmpBuffItemLen (PVOID  pmodule)
{
    return  (JMP_TABLE_ITEMLEN);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
