/**
 * @file
 * ACPI devices.
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

#define __SYLIXOS_PCI_DRV
#include "acpi.h"
#include "accommon.h"
#include "stdio.h"
#include <linux/compat.h>
#include <assert.h>
#include "acpi_interface.h"

/*
 * Internal Use
 */
typedef struct _IrqResource {
    void *Device;
    void *Table;
} IrqResource_t;

/*
 * Video Backlight Capability Callback
 */
ACPI_STATUS AcpiVideoBacklightCapCallback(ACPI_HANDLE Handle, UINT32 Level, void *Context, void **ReturnValue)
{
    ACPI_HANDLE NullHandle = NULL;
    uint64_t *video_features = (uint64_t *)Context;

    if (ACPI_SUCCESS(AcpiGetHandle(Handle, "_BCM", &NullHandle)) &&
        ACPI_SUCCESS(AcpiGetHandle(Handle, "_BCL", &NullHandle))) {
        *video_features |= ACPI_VIDEO_BACKLIGHT;

        /*
         * Check for Brightness
         */
        if (ACPI_SUCCESS(AcpiGetHandle(Handle, "_BQC", &NullHandle)))
            *video_features |= ACPI_VIDEO_BRIGHTNESS;

        /*
         * We have backlight support, no need to scan further
         */
        return AE_CTRL_TERMINATE;
    }

    return AE_OK;
}

/*
 * Is this a video device?
 * If it is, we also retrieve capabilities
 */
ACPI_STATUS AcpiDeviceIsVideo(AcpiDevice_t *Device)
{
    ACPI_HANDLE NullHandle = NULL;
    uint64_t VidFeatures = 0;

    if (Device == NULL)
        return AE_ABORT_METHOD;

    /*
     * Does device support Video Switching
     */
    if (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_DOD", &NullHandle)) &&
        ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_DOS", &NullHandle)))
        VidFeatures |= ACPI_VIDEO_SWITCHING;

    /*
     * Does device support Video Rom?
     */
    if (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_ROM", &NullHandle)))
        VidFeatures |= ACPI_VIDEO_ROM;

    /*
     * Does device support configurable video head?
     */
    if (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_VPO", &NullHandle)) &&
        ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_GPD", &NullHandle)) &&
        ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_SPD", &NullHandle)))
        VidFeatures |= ACPI_VIDEO_POSTING;

    /*
     * Only call this if it is a video device
     */
    if (VidFeatures != 0) {
        AcpiWalkNamespace(ACPI_TYPE_DEVICE, Device->Handle, ACPI_UINT32_MAX,
            AcpiVideoBacklightCapCallback, NULL, &VidFeatures, NULL);

        /*
         * Update ONLY if video device
         */
        Device->xFeatures |= VidFeatures;

        return AE_OK;
    } else
        return AE_NOT_FOUND;
}

/*
 * Is this a docking device?
 * If it has a _DCK method, yes
 */
ACPI_STATUS AcpiDeviceIsDock(AcpiDevice_t *Device)
{
    ACPI_HANDLE NullHandle = NULL;

    if (Device == NULL)
        return AE_ABORT_METHOD;
    else
        return AcpiGetHandle(Device->Handle, "_DCK", &NullHandle);
}

/*
 * Is this a BAY (i.e cd-rom drive with a ejectable bay)
 * We check several ACPI methods here
 */
ACPI_STATUS AcpiDeviceIsBay(AcpiDevice_t *Device)
{
    ACPI_STATUS Status;
    ACPI_HANDLE ParentHandle = NULL;
    ACPI_HANDLE NullHandle = NULL;

    /*
     * Sanity, make sure it is even ejectable
     */
    Status = AcpiGetHandle(Device->Handle, "_EJ0", &NullHandle);

    if (ACPI_FAILURE(Status))
        return Status;

    /*
     * Fine, lets try to fuck it up, _GTF, _GTM, _STM and _SDD,
     * we choose you!
     */
    if ((ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_GTF", &NullHandle))) ||
        (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_GTM", &NullHandle))) ||
        (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_STM", &NullHandle))) ||
        (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_SDD", &NullHandle))))
        return AE_OK;

    /*
     * Uh... ok... maybe we are sub-device of an ejectable parent
     */
    Status = AcpiGetParent(Device->Handle, &ParentHandle);

    if (ACPI_FAILURE(Status))
        return Status;

    /*
     * Now, lets try to fuck up parent !
     */
    if ((ACPI_SUCCESS(AcpiGetHandle(ParentHandle, "_GTF", &NullHandle))) ||
        (ACPI_SUCCESS(AcpiGetHandle(ParentHandle, "_GTM", &NullHandle))) ||
        (ACPI_SUCCESS(AcpiGetHandle(ParentHandle, "_STM", &NullHandle))) ||
        (ACPI_SUCCESS(AcpiGetHandle(ParentHandle, "_SDD", &NullHandle))))
        return AE_OK;

    return AE_NOT_FOUND;
}

/*
 * Is this a video device?
 * If it is, we also retrieve capabilities
 */
ACPI_STATUS AcpiDeviceIsBattery(AcpiDevice_t *Device)
{
    ACPI_HANDLE NullHandle = NULL;
    uint64_t BtFeatures = 0;

    if (Device == NULL)
        return AE_ABORT_METHOD;

    /*
     * Does device support extended battery infromation
     */
    if (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_BIF", &NullHandle))) {
        if (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_BIX", &NullHandle)))
            BtFeatures |= ACPI_BATTERY_EXTENDED;
        else
            BtFeatures |= ACPI_BATTERY_NORMAL;
    }

    /*
     * Does device support us querying battery
     */
    if (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_BST", &NullHandle)))
        BtFeatures |= ACPI_BATTERY_QUERY;

    /*
     * Does device support querying of charge information
     */
    if (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_BTM", &NullHandle)) &&
        ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_BCT", &NullHandle)))
        BtFeatures |= ACPI_BATTERY_CHARGEINFO;

    /*
     * Does device support configurable capacity measurement
     */
    if (ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_BMA", &NullHandle)) &&
        ACPI_SUCCESS(AcpiGetHandle(Device->Handle, "_BMS", &NullHandle)))
        BtFeatures |= ACPI_BATTERY_CAPMEAS;

    /*
     * Only call this if it is a video device
     */
    if (BtFeatures != 0) {
        /*
         * Update ONLY if video device
         */
        Device->xFeatures |= BtFeatures;
        return AE_OK;
    } else
        return AE_NOT_FOUND;
}

/*
 * Get Memory Configuration Range
 */
ACPI_STATUS AcpiDeviceGetMemConfigRange(AcpiDevice_t *Device)
{
    UNUSED(Device);

    return (AE_OK);
}

/*
 * Set Device Data Callback
 */
void AcpiDeviceSetDataCallback(ACPI_HANDLE Handle, void *Data)
{
    UNUSED(Handle);
    UNUSED(Data);
}

/*
 * Set Device Data
 */
ACPI_STATUS AcpiDeviceAttachData(AcpiDevice_t *Device, uint32_t Type)
{
    /*
     * Store, unless its power/sleep buttons
     */
    if ((Type != ACPI_BUS_TYPE_POWER) &&
        (Type != ACPI_BUS_TYPE_SLEEP)) {
        return AcpiAttachData(Device->Handle, AcpiDeviceSetDataCallback, (void*)Device);
    }

    return AE_OK;
}

/*
 * Gets Device Status
 */
ACPI_STATUS AcpiDeviceGetStatus(AcpiDevice_t *Device)
{
    ACPI_STATUS Status = AE_OK;
    ACPI_BUFFER Buffer;
    char lbuf[sizeof(ACPI_OBJECT)];

    /*
     * Set up buffer
     */
    Buffer.Length = sizeof(lbuf);
    Buffer.Pointer = lbuf;

    if (Device->Features & ACPI_FEATURE_STA) {
        Status = AcpiEvaluateObjectTyped(Device->Handle, "_STA", NULL, &Buffer, ACPI_TYPE_INTEGER);

        /*
         * Should not fail :(
         */
        if (ACPI_SUCCESS(Status))
            Device->Status = (uint32_t)((ACPI_OBJECT *)Buffer.Pointer)->Integer.Value;
        else
            Device->Status = 0;
    } else {
        /*
         * The child in should not inherit the parents status if the parent is
         * functioning but not present (ie does not support dynamic status)
         */
        Device->Status = ACPI_STA_DEVICE_PRESENT | ACPI_STA_DEVICE_ENABLED |
                         ACPI_STA_DEVICE_UI      | ACPI_STA_DEVICE_FUNCTIONING;
    }

    return Status;
}

/*
 * Gets Device Bus Number
 */
ACPI_STATUS AcpiDeviceGetBusAndSegment(AcpiDevice_t *Device)
{
    ACPI_STATUS Status = AE_OK;
    ACPI_BUFFER Buffer;
    char lbuf[sizeof(ACPI_OBJECT)];

    /* Set up buffer */
    Buffer.Length = sizeof(lbuf);
    Buffer.Pointer = lbuf;

    /* Sanity */
    if (Device->Features & ACPI_FEATURE_BBN) {
        Status = AcpiEvaluateObjectTyped(Device->Handle, "_BBN", NULL, &Buffer, ACPI_TYPE_INTEGER);

        if (ACPI_SUCCESS(Status))
            Device->Bus = (uint32_t)((ACPI_OBJECT *)Buffer.Pointer)->Integer.Value;
        else
            Device->Bus = 0;
    } else {
        /* Bus number is 0-ish */
        Device->Bus = 0;
    }

    /* Sanity */
    if (Device->Features & ACPI_FEATURE_SEG) {
        Status = AcpiEvaluateObjectTyped(Device->Handle, "_SEG", NULL, &Buffer, ACPI_TYPE_INTEGER);

        if (ACPI_SUCCESS(Status))
            Device->Segment = (uint32_t)((ACPI_OBJECT *)Buffer.Pointer)->Integer.Value;
        else
            Device->Segment = 0;
    } else {
        /* Bus number is 0-ish */
        Device->Segment = 0;
    }

    return Status;
}

/*
 * Gets Device Name
 */
ACPI_STATUS AcpiDeviceGetBusId(AcpiDevice_t *Device, uint32_t Type)
{
    ACPI_STATUS Status = AE_OK;
    ACPI_BUFFER Buffer;
    char BusId[8];

    /* Memset bus_id */
    memset(BusId, 0, sizeof(BusId));

    /* Setup Buffer */
    Buffer.Pointer = BusId;
    Buffer.Length = sizeof(BusId);

    /* Get Object Name based on type */
    switch (Type) {
    case ACPI_BUS_SYSTEM:
        strcpy(Device->BusId, "ACPISB");
        break;
    case ACPI_BUS_TYPE_POWER:
        strcpy(Device->BusId, "POWERF");
        break;
    case ACPI_BUS_TYPE_SLEEP:
        strcpy(Device->BusId, "SLEEPF");
        break;
    default: {
        /* Get name */
        Status = AcpiGetName(Device->Handle, ACPI_SINGLE_NAME, &Buffer);

        /* Sanity */
        if (ACPI_SUCCESS(Status))
            strcpy(Device->BusId, BusId);
    } break;
    }

    return Status;
}

/*
 * Gets Device Features
 */
ACPI_STATUS AcpiDeviceGetFeatures(AcpiDevice_t *Device)
{
    ACPI_STATUS Status;
    ACPI_HANDLE NullHandle = NULL;

    /* Supports dynamic status? */
    Status = AcpiGetHandle(Device->Handle, "_STA", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_STA;

    /* Is compatible ids present? */
    Status = AcpiGetHandle(Device->Handle, "_CID", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_CID;

    /* Supports removable? */
    Status = AcpiGetHandle(Device->Handle, "_RMV", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_RMV;

    /* Supports ejecting? */
    Status = AcpiGetHandle(Device->Handle, "_EJD", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_EJD;
    else {
        Status = AcpiGetHandle(Device->Handle, "_EJ0", &NullHandle);

        if (ACPI_SUCCESS(Status))
            Device->Features |= ACPI_FEATURE_EJD;
    }

    /* Supports device locking? */
    Status = AcpiGetHandle(Device->Handle, "_LCK", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_LCK;

    /* Supports power management? */
    Status = AcpiGetHandle(Device->Handle, "_PS0", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_PS0;
    else {
        Status = AcpiGetHandle(Device->Handle, "_PR0", &NullHandle);

        if (ACPI_SUCCESS(Status))
            Device->Features |= ACPI_FEATURE_PS0;
    }

    /* Supports wake? */
    Status = AcpiGetHandle(Device->Handle, "_PRW", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_PRW;

    /* Has IRQ Routing Table Present ?  */
    Status = AcpiGetHandle(Device->Handle, "_PRT", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_PRT;

    /* Has Current Resources Set ?  */
    Status = AcpiGetHandle(Device->Handle, "_CRS", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_CRS;

    /* Supports Bus Numbering ?  */
    Status = AcpiGetHandle(Device->Handle, "_BBN", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_BBN;

    /* Supports Bus Segment ?  */
    Status = AcpiGetHandle(Device->Handle, "_SEG", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_SEG;

    /* Supports PCI Config Space ?  */
    Status = AcpiGetHandle(Device->Handle, "_REG", &NullHandle);

    if (ACPI_SUCCESS(Status))
        Device->Features |= ACPI_FEATURE_REG;

    return AE_OK;
}

/*
 * IRQ Routing Callback
 */
ACPI_STATUS AcpiDeviceIrqRoutingCallback(ACPI_RESOURCE *Resource, void *Context)
{
    /* Cast the information given to us */
    IrqResource_t *IrqResource = (IrqResource_t*)Context;
    AcpiDevice_t *Device = (AcpiDevice_t*)IrqResource->Device;
    ACPI_PCI_ROUTING_TABLE *IrqTable = (ACPI_PCI_ROUTING_TABLE*)IrqResource->Table;

    /* Needed for storing the interrupt setting */
    PciRoutingEntry_t *pEntry = NULL;
    DataKey_t pKey;

    /* Set the key */
    pKey.Value = 0;

    /* Normal IRQ Resource? */
    if (Resource->Type == ACPI_RESOURCE_TYPE_IRQ) {
        /* Yess, variables for this
         * Calculate offset as well */
        ACPI_RESOURCE_IRQ *Irq;
        int Offset = ((ACPI_HIWORD(ACPI_LODWORD(IrqTable->Address))) * 4) + IrqTable->Pin;

        /* Shorthand access */
        Irq = &Resource->Data.Irq;

        /* Allocate the entry */
        pEntry = (PciRoutingEntry_t*)AcpiOsAllocate(sizeof(PciRoutingEntry_t));

        /* Set information */
        pEntry->Polarity = Irq->Polarity;
        pEntry->Trigger = Irq->Triggering;
        pEntry->Shareable = Irq->Sharable;
        pEntry->Interrupts = Irq->Interrupts[IrqTable->SourceIndex];

        /* Do we already have an entry?? */
        if (Device->Routings->InterruptInformation[Offset] == 1) {

            /* Ok... We are an list */
            ListAppend(Device->Routings->Interrupts[Offset].Entries,
                ListCreateNode(pKey, pKey, pEntry));

        } else if (Device->Routings->InterruptInformation[Offset] == 0
                && Device->Routings->Interrupts[Offset].Entry != NULL) {

            /* Create a new list */
            List_t *IntList = ListCreate(KeyInteger, LIST_NORMAL);

            /* Append the existing entry */
            ListAppend(IntList, ListCreateNode(pKey, pKey, Device->Routings->Interrupts[Offset].Entry));

            /* Append the new entry */
            ListAppend(IntList, ListCreateNode(pKey, pKey, pEntry));

            /* Store the list and set upgraded */
            Device->Routings->Interrupts[Offset].Entries = IntList;
            Device->Routings->InterruptInformation[Offset] = 1;

        } else
            Device->Routings->Interrupts[Offset].Entry = pEntry;

    } else if (Resource->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
        /* Extended, variables for this
         * Calculate offset as well */
        ACPI_RESOURCE_EXTENDED_IRQ *Irq;
        int Offset = ((ACPI_HIWORD(ACPI_LODWORD(IrqTable->Address))) * 4) + IrqTable->Pin;

        /* Shorthand access */
        Irq = &Resource->Data.ExtendedIrq;

        /* Allocate the entry */
        pEntry = (PciRoutingEntry_t*)AcpiOsAllocate(sizeof(PciRoutingEntry_t));

        /* Set information */
        pEntry->Polarity = Irq->Polarity;
        pEntry->Trigger = Irq->Triggering;
        pEntry->Shareable = Irq->Sharable;
        pEntry->Interrupts = Irq->Interrupts[IrqTable->SourceIndex];

        /* Do we already have an entry?? */
        if (Device->Routings->InterruptInformation[Offset] == 1) {

            /* Ok... We are an list */
            ListAppend(Device->Routings->Interrupts[Offset].Entries,
                ListCreateNode(pKey, pKey, pEntry));

        } else if (Device->Routings->InterruptInformation[Offset] == 0
                && Device->Routings->Interrupts[Offset].Entry != NULL) {

            /* Create a new list */
            List_t *IntList = ListCreate(KeyInteger, LIST_NORMAL);

            /* Append the existing entry */
            ListAppend(IntList, ListCreateNode(pKey, pKey, Device->Routings->Interrupts[Offset].Entry));

            /* Append the new entry */
            ListAppend(IntList, ListCreateNode(pKey, pKey, pEntry));

            /* Store the list and set upgraded */
            Device->Routings->Interrupts[Offset].Entries = IntList;
            Device->Routings->InterruptInformation[Offset] = 1;

        } else
            Device->Routings->Interrupts[Offset].Entry = pEntry;
    }

    return AE_OK;
}

/*
 * Gets IRQ Routings
 */
ACPI_STATUS AcpiDeviceGetIrqRoutings(AcpiDevice_t *Device)
{
    ACPI_STATUS Status;
    ACPI_BUFFER aBuff;
    ACPI_PCI_ROUTING_TABLE *PciTable;
    PciRoutings_t *Table;
    int i;

    /* Setup Buffer */
    aBuff.Length = 0x2000;
    aBuff.Pointer = (char*)AcpiOsAllocate(0x2000);

    /* Try to get routings */
    Status = AcpiGetIrqRoutingTable(Device->Handle, &aBuff);
    if (ACPI_FAILURE(Status))
        goto done;

    /* Allocate Table */
    Table = (PciRoutings_t*)AcpiOsAllocate(sizeof(PciRoutings_t));

    /* Reset it */
    for (i = 0; i < 128; i++) {
        Table->Interrupts[i].Entry = NULL;
        Table->InterruptInformation[i] = 0;
    }

    /* Link it */
    Device->Routings = Table;

    /* Enumerate */
    for (PciTable = (ACPI_PCI_ROUTING_TABLE *)aBuff.Pointer; PciTable->Length;
         PciTable = (ACPI_PCI_ROUTING_TABLE *)((char *)PciTable + PciTable->Length)) {
        /* Variables */
        ACPI_HANDLE SourceHandle;
        IrqResource_t IrqRes;

        /* Wub, we have a routing */
        if (*(char*)PciTable->Source == '\0') {
            /* Ok, eol */
            PciRoutingEntry_t *pEntry = (PciRoutingEntry_t*)AcpiOsAllocate(sizeof(PciRoutingEntry_t));

            /* Set it */
            int _dev = (ACPI_HIWORD(ACPI_LODWORD(PciTable->Address)));
            int offset = (_dev * 4) + PciTable->Pin;

            /* Fixed GSI */
            pEntry->Interrupts = PciTable->SourceIndex;
            pEntry->Polarity = ACPI_ACTIVE_LOW;
            pEntry->Trigger = ACPI_LEVEL_SENSITIVE;
            pEntry->Fixed = 1;

            /* Set entry */
            Table->Interrupts[offset].Entry = pEntry;

            /* Done! */
            continue;
        }

        /* Get handle of source */
        Status = AcpiGetHandle(Device->Handle, PciTable->Source, &SourceHandle);
        if (ACPI_FAILURE(Status)) {
            LogFatal("ACPI", "Failed AcpiGetHandle\n");
            continue;
        }

        /* Get all IRQ resources */
        IrqRes.Device = (void*)Device;
        IrqRes.Table = (void*)PciTable;

        /* Walk the handle */
        Status = AcpiWalkResources(SourceHandle,
            METHOD_NAME__CRS, AcpiDeviceIrqRoutingCallback, &IrqRes);

        /* Sanitize the return status */
        if (ACPI_FAILURE(Status)) {
            LogFatal("ACPI", "Failed IRQ resource\n");
            continue;
        }
    }

done:
    AcpiOsFree(aBuff.Pointer);
    return Status;
}

/*
 * Gets Device Information
 */
ACPI_STATUS AcpiDeviceGetHWInfo(AcpiDevice_t *Device, ACPI_HANDLE ParentHandle, uint32_t Type)
{
    ACPI_STATUS Status;
    ACPI_DEVICE_INFO *DeviceInfo;
    char lbuf[2048];
    ACPI_BUFFER Buffer;
    ACPI_PNP_DEVICE_ID_LIST *Cid = NULL;
    char *Hid = NULL;
    char *Uid = NULL;
    const char *CidAdd = NULL;

    /* Memset buffer */
    memset(lbuf, 0, sizeof(lbuf));

    /* Set up initial variables */
    Buffer.Length = sizeof(lbuf);
    Buffer.Pointer = lbuf;
    DeviceInfo = Buffer.Pointer;

    /* What are we dealing with? */
    switch (Type) {
    /* Normal Device */
    case ACPI_BUS_TYPE_DEVICE: {
        /* Get Object Info */
        Status = AcpiGetObjectInfo(Device->Handle, &DeviceInfo);

        /* Sanity */
        if (ACPI_FAILURE(Status))
            return Status;

        /* Get only valid fields */
        if (DeviceInfo->Valid & ACPI_VALID_HID)
            Hid = DeviceInfo->HardwareId.String;
        if (DeviceInfo->Valid & ACPI_VALID_UID)
            Uid = DeviceInfo->UniqueId.String;
        if (DeviceInfo->Valid & ACPI_VALID_CID)
            Cid = &DeviceInfo->CompatibleIdList;
        if (DeviceInfo->Valid & ACPI_VALID_ADR) {
            Device->Address = DeviceInfo->Address;
            Device->Features |= ACPI_FEATURE_ADR;
        }

        /* Check for special device, i.e Video / Bay / Dock */
        if (AcpiDeviceIsVideo(Device) == AE_OK)
            CidAdd = "VIDEO";
        else if (AcpiDeviceIsDock(Device) == AE_OK)
            CidAdd = "DOCK";
        else if (AcpiDeviceIsBay(Device) == AE_OK)
            CidAdd = "BAY";
        else if (AcpiDeviceIsBattery(Device) == AE_OK)
            CidAdd = "BATT";

    } break;

    case ACPI_BUS_SYSTEM:
        Hid = "SBUS";
        break;
    case ACPI_BUS_TYPE_POWER:
        Hid = "PWRBN";
        break;
    case ACPI_BUS_TYPE_PROCESSOR:
        Hid = "CPU";
        break;
    case ACPI_BUS_TYPE_SLEEP:
        Hid = "SLPBN";
        break;
    case ACPI_BUS_TYPE_THERMAL:
        Hid = "THERM";
        break;
    case ACPI_BUS_TYPE_PWM:
        Hid = "POWER";
        break;
    }

    /* Fix for Root System Bus (\_SB) */
    if (((ACPI_HANDLE)ParentHandle == ACPI_ROOT_OBJECT) && (Type == ACPI_BUS_TYPE_DEVICE))
        Hid = "SYSTM";

    /* Store HID and UID */
    if (Hid) {
        strcpy(Device->HID, Hid);
        Device->Features |= ACPI_FEATURE_HID;
    }

    if (Uid) {
        strcpy(Device->UID, Uid);
        Device->Features |= ACPI_FEATURE_UID;
    }

    /* Now store CID */
    if (Cid != NULL || CidAdd != NULL) {
        ACPI_PNP_DEVICE_ID_LIST *list;
        ACPI_SIZE size = 0;
        UINT32 count = 0;

        /* Get size if list exists */
        if (Cid)
            size = Cid->ListSize;
        else if (CidAdd) {
            /* Allocate a bare structure */
            size = sizeof(ACPI_PNP_DEVICE_ID_LIST);
            Cid = ACPI_ALLOCATE_ZEROED(size);

            /* Set */
            Cid->ListSize = size;
            Cid->Count = 0;
        }

        /* Do we need to manually add extra entry ? */
        if (CidAdd)
            size += sizeof(ACPI_PNP_DEVICE_ID_LIST);

        /* Allocate new list */
        list = (ACPI_PNP_DEVICE_ID_LIST*)AcpiOsAllocate((size_t)size);

        /* Copy list */
        if (Cid) {
            memcpy(list, Cid, Cid->ListSize);
            count = Cid->Count;
        }

        if (CidAdd) {
            list->Ids[count].Length = strlen(CidAdd) + 1;
            list->Ids[count].String = (char*)AcpiOsAllocate(list->Ids[count].Length);
            strncpy(list->Ids[count].String, CidAdd, list->Ids[count].Length);
            count++;
        }

        /* Set information */
        list->Count = count;
        list->ListSize = size;
        Device->CID = list;
        Device->Features |= ACPI_FEATURE_CID;
    }

    return AE_OK;
}


/*
 * Pin conversion from behind a bridge
 */
int AcpiDerivePin(int Device, int Pin, int ParentPin)
{
    return ((Pin + ParentPin) % 4);
}

/*
 * Get Irq by Bus / Dev / Pin
 * Returns -1 if no overrides exists
 */
int AcpiDeviceGetIrq(PCI_DEV_HANDLE PciDev, int Pin,
                     uint8_t *TriggerMode, uint8_t *Polarity,
                     uint8_t *Shareable, uint8_t *Fixed)
{
    /* Locate correct bus */
    AcpiDevice_t *AcpiDev = NULL;
    int Dev = PciDev->PCIDEV_iDevDevice;
    DataKey_t iKey;

    /* Calculate routing index */
    int rIndex = (Dev * 4) + Pin;

    /* Set Key */
    iKey.Value = 0;

    /* Start by checking if we can find the
    * routings by checking the given device */
    AcpiDev = AcpiLookupDevice(PciDev->PCIDEV_iDevBus);

    /* Sanitize */
    if (AcpiDev != NULL) {
        /* Sanity */
        if (AcpiDev->Routings->Interrupts[rIndex].Entry != NULL) {

            /* Extract the entry */
            PciRoutingEntry_t *pEntry = NULL;

            /* Either from list or raw */
            if (AcpiDev->Routings->InterruptInformation[rIndex] == 0) {
                pEntry = AcpiDev->Routings->Interrupts[rIndex].Entry;
            } else {
                pEntry = (PciRoutingEntry_t*)
                    ListGetDataByKey(AcpiDev->Routings->Interrupts[rIndex].Entries, iKey, 0);
            }

            /* Update IRQ Information */
            if (pEntry->Trigger == ACPI_LEVEL_SENSITIVE)
                *TriggerMode = 1;
            else
                *TriggerMode = 0;

            if (pEntry->Polarity == ACPI_ACTIVE_LOW)
                *Polarity = 1;
            else
                *Polarity = 0;

            *Shareable = pEntry->Shareable;
            *Fixed = pEntry->Fixed;
            return pEntry->Interrupts;
        }
    }

    /* Damn, check parents */
    PciDev = API_PciDevParentHandleGet(PciDev->PCIDEV_iDevBus,
                                       PciDev->PCIDEV_iDevDevice,
                                       PciDev->PCIDEV_iDevFunction);
    while (PciDev) {

        /* Correct the pin */
        Pin = AcpiDerivePin(Dev, Pin, PciDev->PCIDEV_ucPin - 1);

        Dev = PciDev->PCIDEV_iDevDevice;

        /* Calculate new corrected routing index */
        rIndex = (Dev * 4) + Pin;

        /* Start by checking if we can find the
        * routings by checking the given device */
        AcpiDev = AcpiLookupDevice(PciDev->PCIDEV_iDevBus);

        /* Sanitize */
        if (AcpiDev != NULL) {
            if (AcpiDev->Routings->Interrupts[rIndex].Entry != NULL) {

                /* Extract the entry */
                PciRoutingEntry_t *pEntry = NULL;

                /* Either from list or raw */
                if (AcpiDev->Routings->InterruptInformation[rIndex] == 0) {
                    pEntry = AcpiDev->Routings->Interrupts[rIndex].Entry;
                } else {
                    pEntry = (PciRoutingEntry_t*)
                        ListGetDataByKey(AcpiDev->Routings->Interrupts[rIndex].Entries, iKey, 0);
                }

                /* Update IRQ Information */
                if (pEntry->Trigger == ACPI_LEVEL_SENSITIVE)
                    *TriggerMode = 1;
                else
                    *TriggerMode = 0;

                if (pEntry->Polarity == ACPI_ACTIVE_LOW)
                    *Polarity = 1;
                else
                    *Polarity = 0;

                *Shareable = pEntry->Shareable;
                *Fixed = pEntry->Fixed;
                return pEntry->Interrupts;
            }
        }

        PciDev = API_PciDevParentHandleGet(PciDev->PCIDEV_iDevBus,
                                           PciDev->PCIDEV_iDevDevice,
                                           PciDev->PCIDEV_iDevFunction);
    }

    return -1;
}
