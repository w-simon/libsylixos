/**
 * @file
 * ACPI enum.
 */

/*
 * Copyright (c) 2006-2016 SylixOS Group.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Jiao.JinXing <jiaojinxing@acoinfo.com>
 *
 */

#include "acpi.h"
#include "accommon.h"
#include "stdio.h"
#include <linux/compat.h>
#include <assert.h>
#include "acpi_interface.h"
#include "arch/x86/mpconfig/x86MpConfig.h"
#include "arch/x86/common/x86CpuId.h"
#include "arch/x86/param/x86Param.h"

/*
 * Globals
 */
static List_t *GlbAcpiNodes = NULL;
static int GlbAcpiAvailable = ACPI_NOT_AVAILABLE;

/*
 * Static Acpica
 */
static ACPI_TABLE_DESC TableArray[ACPI_MAX_INIT_TABLES];

/*
 * Enumerate MADT Entries
 */
void AcpiEnumarateMADT(void *MadtStart, void *MadtEnd)
{
    ACPI_SUBTABLE_HEADER   *MadtEntry;
    X86_PARAM              *pparam;
    BOOL                    bHyperThreading = LW_FALSE;
    DataKey_t               Key;

    if (!_G_bX86HasMpConfig) {                                          /*  没有 MP 配置                */
        pparam = archKernelParamGet();                                  /*  获得启动参数结构            */
        bHyperThreading = pparam->X86_bHyperThreading;

        if (!_G_bX86HasHTT) {                                           /*  如果识别无超线程技术        */
            bHyperThreading = LW_FALSE;                                 /*  强制不使用超线程技术        */
        }

        _G_iX86LProcNr = 0;                                             /*  重置逻辑 Processor 数目     */
        _G_iX86PProcNr = 0;                                             /*  重置物理 Processor 数目     */
    }

    for (MadtEntry = (ACPI_SUBTABLE_HEADER*)MadtStart;
        (void *)MadtEntry < MadtEnd;) {
        /*
         * Avoid an infinite loop if we hit a bogus entry.
         */
        if (MadtEntry->Length < sizeof(ACPI_SUBTABLE_HEADER))
            return;

        switch (MadtEntry->Type) {
        /*
         * Processor Core
         */
        case ACPI_MADT_TYPE_LOCAL_APIC: {
            /* Allocate a new structure */
            ACPI_MADT_LOCAL_APIC *CpuNode =
                (ACPI_MADT_LOCAL_APIC*)AcpiOsAllocate(sizeof(ACPI_MADT_LOCAL_APIC));

            /* Cast to correct structure */
            ACPI_MADT_LOCAL_APIC *AcpiCpu = (ACPI_MADT_LOCAL_APIC*)MadtEntry;

            /* Now we have it allocated aswell, copy info */
            memcpy(CpuNode, AcpiCpu, sizeof(ACPI_MADT_LOCAL_APIC));

            /* Insert it into list */
            Key.Value = ACPI_MADT_TYPE_LOCAL_APIC;
            ListAppend(GlbAcpiNodes, ListCreateNode(Key, Key, CpuNode));

            LogInformation("MADT", "Found CPU: %u (%s)",
                AcpiCpu->Id, (AcpiCpu->LapicFlags & 0x1) ? "Active" : "Inactive");

            if (!_G_bX86HasMpConfig) {                                  /*  没有 MP 配置                */
                _G_x86Apic2LInfo[AcpiCpu->Id].APIC2L_bPresent    = LW_TRUE;
                _G_x86Apic2LInfo[AcpiCpu->Id].APIC2L_bIsHt       = LW_FALSE;
                _G_x86Apic2LInfo[AcpiCpu->Id].APIC2L_ulCPUId     = _G_iX86LProcNr;
                _G_x86L2ApicInfo[_G_iX86LProcNr].L2APIC_ucApicId = AcpiCpu->Id;
                _G_iX86LProcNr++;
                _G_iX86PProcNr++;

                if (bHyperThreading) {                                  /*  超线程支持                  */
                    _G_x86Apic2LInfo[AcpiCpu->Id + 1].APIC2L_bPresent = LW_TRUE;
                    _G_x86Apic2LInfo[AcpiCpu->Id + 1].APIC2L_bIsHt    = LW_TRUE;
                    _G_x86Apic2LInfo[AcpiCpu->Id + 1].APIC2L_ulCPUId  = _G_iX86LProcNr;
                    _G_x86L2ApicInfo[_G_iX86LProcNr].L2APIC_ucApicId  = AcpiCpu->Id + 1;
                    _G_iX86LProcNr++;
                }
            }
        } break;

        /*
         * IO Apic
         */
        case ACPI_MADT_TYPE_IO_APIC: {
            /* Alocate a new structure */
            ACPI_MADT_IO_APIC *IoNode =
                (ACPI_MADT_IO_APIC*)AcpiOsAllocate(sizeof(ACPI_MADT_IO_APIC));

            /* Cast to correct structure */
            ACPI_MADT_IO_APIC *AcpiIoApic = (ACPI_MADT_IO_APIC*)MadtEntry;

            /* Now we have it allocated aswell, copy info */
            memcpy(IoNode, AcpiIoApic, sizeof(ACPI_MADT_IO_APIC));

            /* Insert it into list */
            Key.Value = ACPI_MADT_TYPE_IO_APIC;
            ListAppend(GlbAcpiNodes, ListCreateNode(Key, Key, IoNode));

            /* Debug */
            LogInformation("MADT", "Found IO-APIC: %u", AcpiIoApic->Id);
        } break;

        /*
         * Interrupt Overrides
         */
        case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE: {
            /* Allocate a new structure */
            ACPI_MADT_INTERRUPT_OVERRIDE *OverrideNode =
                (ACPI_MADT_INTERRUPT_OVERRIDE*)AcpiOsAllocate(sizeof(ACPI_MADT_INTERRUPT_OVERRIDE));

            /* Cast to correct structure */
            ACPI_MADT_INTERRUPT_OVERRIDE *AcpiOverrideNode =
                (ACPI_MADT_INTERRUPT_OVERRIDE*)MadtEntry;

            /* Now we have it allocated aswell, copy info */
            memcpy(OverrideNode, AcpiOverrideNode, sizeof(ACPI_MADT_INTERRUPT_OVERRIDE));

            /* Insert it into list */
            Key.Value = ACPI_MADT_TYPE_INTERRUPT_OVERRIDE;
            ListAppend(GlbAcpiNodes, ListCreateNode(Key, Key, OverrideNode));
        } break;

        /*
         * Local APIC NMI Configuration
         */
        case ACPI_MADT_TYPE_LOCAL_APIC_NMI: {
            /* Allocate a new structure */
            ACPI_MADT_LOCAL_APIC_NMI *NmiNode =
                (ACPI_MADT_LOCAL_APIC_NMI*)AcpiOsAllocate(sizeof(ACPI_MADT_LOCAL_APIC_NMI));

            /* Cast to correct structure */
            ACPI_MADT_LOCAL_APIC_NMI *ApinNmi = (ACPI_MADT_LOCAL_APIC_NMI*)MadtEntry;

            /* Now we have it allocated aswell, copy info */
            memcpy(NmiNode, ApinNmi, sizeof(ACPI_MADT_LOCAL_APIC_NMI));

            /* Insert it into list */
            Key.Value = ACPI_MADT_TYPE_LOCAL_APIC_NMI;
            ListAppend(GlbAcpiNodes, ListCreateNode(Key, Key, NmiNode));
        } break;

        default:
            LogDebug("MADT", "Found Type %u", MadtEntry->Type);
            break;
        }

        MadtEntry = (ACPI_SUBTABLE_HEADER*)
            ACPI_ADD_PTR(ACPI_SUBTABLE_HEADER, MadtEntry, MadtEntry->Length);
    }
}

/*
 * Enumerate SRAT Entries
 */
void AcpiEnumerateSRAT(void *SratStart, void *SratEnd)
{
    UNUSED(SratStart);
    UNUSED(SratEnd);
}

/*
 * Initializes Early Access
 * and enumerates the APIC
 */
int AcpiEnumerate(void)
{
    /*
     * Variables needed for initial
     * ACPI access
     */
    ACPI_TABLE_HEADER *Header = NULL;
    ACPI_STATUS Status = 0;

    /*
     * Early Table Access
     */
    Status = AcpiInitializeTables((ACPI_TABLE_DESC*)&TableArray, ACPI_MAX_INIT_TABLES, TRUE);

    /*
     * If this fails there is no ACPI on the system
     */
    if (ACPI_FAILURE(Status)) {
        LogFatal("ACPI", "Failed to initialize early ACPI access,"
            "probable no ACPI available (%u)", Status);
        GlbAcpiAvailable = ACPI_NOT_AVAILABLE;
        return -1;
    } else
        GlbAcpiAvailable = ACPI_AVAILABLE;

    /*
     * Create the acpi lists
     */

    /*
     * Enumerate MADT
     */
    if (ACPI_SUCCESS(AcpiGetTable(ACPI_SIG_MADT, 0, &Header))) {
        ACPI_TABLE_MADT *MadtTable = NULL;

        LogInformation("ACPI", "Enumerating the MADT Table");

        MadtTable = (ACPI_TABLE_MADT*)Header;

        AcpiEnumarateMADT((void*)((addr_t)MadtTable + sizeof(ACPI_TABLE_MADT)),
            (void*)((addr_t)MadtTable + MadtTable->Header.Length));
    }

    /*
     * Enumerate SRAT
     */
    if (ACPI_SUCCESS(AcpiGetTable(ACPI_SIG_SRAT, 0, &Header))) {
        ACPI_TABLE_SRAT *SratTable = NULL;

        LogInformation("ACPI", "Enumerating the SRAT Table");

        SratTable = (ACPI_TABLE_SRAT*)Header;

        AcpiEnumerateSRAT((void*)((addr_t)SratTable + sizeof(ACPI_TABLE_MADT)),
            (void*)((addr_t)SratTable + SratTable->Header.Length));
    }

    /*
     * Enumerate SRAT
     */
    if (ACPI_SUCCESS(AcpiGetTable(ACPI_SIG_SBST, 0, &Header))) {
        ACPI_TABLE_SBST * __unused BattTable = NULL;

        LogInformation("ACPI", "Parsing the SBST Table");

        BattTable = (ACPI_TABLE_SBST*)Header;
    }

    return 0;
}

/*
 * This returns 0 if ACPI is not available
 * on the system, or 1 if acpi is available
 */
int AcpiAvailable(void)
{
    return GlbAcpiAvailable;
}
