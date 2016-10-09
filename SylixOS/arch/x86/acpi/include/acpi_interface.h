/**
 * @file
 * ACPI interface.
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

#ifndef ACPI_INTERFACE_H_
#define ACPI_INTERFACE_H_

#include "acpi.h"
#include "acpi_list.h"

/*
 * Log
 */
#define Log(fmt,arg...) \
        printk(KERN_INFO fmt, ##arg)

#define LogInformation(mod, fmt,arg...) \
        printk(KERN_INFO mod ": " fmt "\n", ##arg)

#define LogDebug(mod, fmt,arg...) \
        printk(KERN_DEBUG mod ": " fmt "\n", ##arg)

#define LogFatal(mod, fmt,arg...) \
        printk(KERN_ERR mod ": " fmt "\n", ##arg)

#define UNUSED(var)

/*
 * Definitions
 */
#define ACPI_MAX_INIT_TABLES    16

#define ACPI_NOT_AVAILABLE      0
#define ACPI_AVAILABLE          1

/*
 * Feature Flags
 */
#define ACPI_FEATURE_STA        0x1
#define ACPI_FEATURE_CID        0x2
#define ACPI_FEATURE_RMV        0x4
#define ACPI_FEATURE_EJD        0x8
#define ACPI_FEATURE_LCK        0x10
#define ACPI_FEATURE_PS0        0x20
#define ACPI_FEATURE_PRW        0x40
#define ACPI_FEATURE_ADR        0x80
#define ACPI_FEATURE_HID        0x100
#define ACPI_FEATURE_UID        0x200
#define ACPI_FEATURE_PRT        0x400
#define ACPI_FEATURE_BBN        0x800
#define ACPI_FEATURE_SEG        0x1000
#define ACPI_FEATURE_REG        0x2000
#define ACPI_FEATURE_CRS        0x4000

/*
 * Type Definitions
 */
#define ACPI_BUS_SYSTEM         0x0
#define ACPI_BUS_TYPE_DEVICE    0x1
#define ACPI_BUS_TYPE_PROCESSOR 0x2
#define ACPI_BUS_TYPE_THERMAL   0x3
#define ACPI_BUS_TYPE_POWER     0x4
#define ACPI_BUS_TYPE_SLEEP     0x5
#define ACPI_BUS_TYPE_PWM       0x6
#define ACPI_BUS_ROOT_BRIDGE    0x7

/*
 * Video Features
 */
#define ACPI_VIDEO_SWITCHING    0x1
#define ACPI_VIDEO_ROM          0x2
#define ACPI_VIDEO_POSTING      0x4
#define ACPI_VIDEO_BACKLIGHT    0x8
#define ACPI_VIDEO_BRIGHTNESS   0x10

/*
 * Battery Features
 */
#define ACPI_BATTERY_NORMAL     0x1
#define ACPI_BATTERY_EXTENDED   0x2
#define ACPI_BATTERY_QUERY      0x4
#define ACPI_BATTERY_CHARGEINFO 0x8
#define ACPI_BATTERY_CAPMEAS    0x10

/*
 * Structures
 */

/*
 * First we declare an interrupt entry
 * a very small structure containing information
 * about an interrupt in this system
 */
#pragma pack(push, 1)
typedef struct _PciRoutingEntry {
    int Interrupts;                                                     /*  The interrupt line          */

    uint8_t Trigger;                                                    /*  Interrupt information       */
    uint8_t Shareable;
    uint8_t Polarity;
    uint8_t Fixed;
} PciRoutingEntry_t;

/*
 * A table containing 128 interrupt entries
 * which is the number of 'redirects' there can
 * be
 */
typedef struct _PciRoutings {
    /*
     * This descripes whether or not
     * an entry is a list or .. not a list
     */
    int InterruptInformation[128];

    /*
     * Just a table of 128 interrupts or
     * 128 lists of interrupts
     */
    union {
        PciRoutingEntry_t *Entry;
        List_t *Entries;
    } Interrupts[128];
} PciRoutings_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _AcpiDevice {
    char *Name;                                                         /*  Name                        */
    int Type;                                                           /*  Type                        */
    void *Handle;                                                       /*  ACPI_HANDLE                 */
    PciRoutings_t *Routings;                                            /*  Irq Routings                */
    char BusId[8];                                                      /*  Bus Id                      */
    int Bus;                                                            /*  PCI Location                */
    int Device;
    int Function;
    int Segment;
    size_t Features;                                                    /*  Supported NS Functions      */
    size_t Status;                                                      /*  Current Status              */
    uint64_t Address;                                                   /*  Bus Address                 */
    char HID[16];                                                       /*  Hardware Id                 */
    char UID[16];                                                       /*  Unique Id                   */
    void *CID;                                                          /*  Compatible Id's             */
    int xFeatures;                                                      /*  Type Features               */
} AcpiDevice_t;
#pragma pack(pop)


/*
 * Initialize functions, don't call these manually
 * they get automatically called in kernel setup
 */

/*
 * Initializes Early access and enumerates
 * ACPI Tables, returns -1 if ACPI is not
 * present on this system
 */
extern int AcpiEnumerate(void);

/*
 * Initializes the full access and functionality
 * of ACPICA / ACPI and allows for scanning of
 * ACPI devices
 */
extern int AcpiInitialize(void);

/*
 * Scans the ACPI bus/namespace for all available
 * ACPI devices/functions and initializes them
 */
extern int AcpiScan(void);

/*
 * Get Functions
 */

/*
 * This returns ACPI_NOT_AVAILABLE if ACPI is not available
 * on the system, or ACPI_AVAILABLE if acpi is available
 */
extern int AcpiAvailable(void);

/*
 * Lookup a bridge device for the given
 * bus that contains pci routings
 */
extern AcpiDevice_t *AcpiLookupDevice(int Bus);

/*
 * Device Functions
 */
extern ACPI_STATUS AcpiDeviceAttachData(AcpiDevice_t *Device, uint32_t Type);

/*
 * Device Get's
 */
extern ACPI_STATUS AcpiDeviceGetStatus(AcpiDevice_t* Device);
extern ACPI_STATUS AcpiDeviceGetBusAndSegment(AcpiDevice_t* Device);
extern ACPI_STATUS AcpiDeviceGetBusId(AcpiDevice_t *Device, uint32_t Type);
extern ACPI_STATUS AcpiDeviceGetFeatures(AcpiDevice_t *Device);
extern ACPI_STATUS AcpiDeviceGetIrqRoutings(AcpiDevice_t *Device);
extern ACPI_STATUS AcpiDeviceGetHWInfo(AcpiDevice_t *Device, ACPI_HANDLE ParentHandle, uint32_t Type);

/*
 * Device Type Helpers
 */
extern ACPI_STATUS AcpiDeviceIsVideo(AcpiDevice_t *Device);
extern ACPI_STATUS AcpiDeviceIsDock(AcpiDevice_t *Device);
extern ACPI_STATUS AcpiDeviceIsBay(AcpiDevice_t *Device);
extern ACPI_STATUS AcpiDeviceIsBattery(AcpiDevice_t *Device);

#ifdef __SYLIXOS_PCI_DRV
extern int AcpiDeviceGetIrq(PCI_DEV_HANDLE PciDev, int Pin,
                            uint8_t *TriggerMode, uint8_t *Polarity,
                            uint8_t *Shareable, uint8_t *Fixed);
#endif

#endif /* ACPI_INTERFACE_H_ */
