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

#include "acpi.h"
#include "accommon.h"
#include "stdio.h"
#include <linux/compat.h>
#include <assert.h>
#include "acpi_interface.h"

/*
 * OSC
 */
#define ACPI_OSC_QUERY_INDEX				0
#define ACPI_OSC_SUPPORT_INDEX				1
#define ACPI_OSC_CONTROL_INDEX				2

#define ACPI_OSC_QUERY_ENABLE				0x1

#define ACPI_OSC_SUPPORT_SB_PR3_SUPPORT		0x4
#define ACPI_OSC_SUPPORT_SB_APEI_SUPPORT	0x10

struct _acpi_osc {
	char       *uuid;                                                   /*  Object UUID                 */
	int         revision;                                               /*  Revision = 1                */
	ACPI_BUFFER capabilities;                                           /*  list of DWORD capabilities  */
	ACPI_BUFFER retval;                                                 /*  free by caller if success   */
};

/*
 * OSI Stuff
 */
#define OSI_STRING_LENGTH_MAX   64
#define OSI_STRING_ENTRIES_MAX  16

typedef struct _OsiSetupEntry {
	char    String[OSI_STRING_LENGTH_MAX];
	uint8_t Enable;
} OsiSetupEntry_t;

static OsiSetupEntry_t OsiSetupEntries[OSI_STRING_ENTRIES_MAX] = {
	{"Module Device", 1},
	{"Processor Device", 1},
	{"3.0 _SCP Extensions", 1},
	{"Processor Aggregator Device", 1},
};

/*
 * ACPICA Stuff
 */
extern void AcpiUtConvertStringToUuid(char*, UINT8*);

/*
 * Global ACPI Information
 */
static char * __unused sb_uuid_str       = "0811B06E-4A27-44F9-8D60-3CBBC22E7B48";
static char * __unused osc_uuid_str      = "33DB4D5B-1FF7-401C-9657-7441C03DD766";
static char * __unused osc_batt_uuid_str = "F18FC78B-0F15-4978-B793-53F833A1D35B";

/*
 * Fixed Event Handlers
 */
UINT32 AcpiShutdownHandler(void *Context)
{
	ACPI_EVENT_STATUS eStatus;
	ACPI_STATUS __unused Status;

	/*
	 * Get Event Data
	 */
	Status = AcpiGetEventStatus(ACPI_EVENT_POWER_BUTTON, &eStatus);

	assert(ACPI_SUCCESS(Status));

	if (eStatus & ACPI_EVENT_FLAG_ENABLED) {
		AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);
	}

    API_KernelReboot(LW_REBOOT_SHUTDOWN);

	return AE_OK;
}

UINT32 AcpiSleepHandler(void *Context)
{
	return AE_OK;
}

/*
 * Notify Handler
 */
void AcpiBusNotifyHandler(ACPI_HANDLE Device, UINT32 NotifyType, void *Context)
{
	UNUSED(Device);
	UNUSED(Context);

	Log("ACPI_Notify: Type 0x%x\n", NotifyType);
}

/*
 * Global Event Handler
 */
void AcpiEventHandler(UINT32 EventType, ACPI_HANDLE Device, UINT32 EventNumber, void *Context)
{
	UNUSED(Device);
	UNUSED(Context);

	Log("ACPI_Event: Type 0x%x, Number 0x%x\n", EventType, EventNumber);
}

/*
 * Interface Handlers
 */
UINT32 AcpiOsi(ACPI_STRING InterfaceName, UINT32 Supported)
{
	if (!strcmp("Darwin", InterfaceName)) {
		/*
		 * Apple firmware will behave poorly if it receives positive
		 * answers to "Darwin" and any other OS. Respond positively
		 * to Darwin and then disable all other vendor strings.
		 */
		AcpiUpdateInterfaces(ACPI_DISABLE_ALL_VENDOR_STRINGS);
		Supported = ACPI_UINT32_MAX;
	}

	return Supported;
}

/*
 * Run OSC Query
 */
ACPI_STATUS AcpiRunOscRequest(ACPI_HANDLE device, struct _acpi_osc *osc)
{
	ACPI_STATUS status = AE_ERROR;
	ACPI_OBJECT_LIST input;
	ACPI_OBJECT in_params[4];
	ACPI_OBJECT *out_param;
	uint8_t uuid[16];
	ACPI_BUFFER output;
	uint32_t query_status = 0;

	if(osc == NULL)
		return AE_BAD_DATA;

	/*
	 * Convert
	 */
	AcpiUtConvertStringToUuid(osc->uuid, uuid);

	/*
	 * Setup return object
	 */
	osc->retval.Length = ACPI_ALLOCATE_BUFFER;
	osc->retval.Pointer = NULL;

	/*
	 * Setup output
	 */
	output.Length = ACPI_ALLOCATE_BUFFER;
	output.Pointer = NULL;

	/*
	 * Set up list
	 */
	input.Count = 4;
	input.Pointer = in_params;

	/*
	 * Set up parameters
	 */
	in_params[0].Type = ACPI_TYPE_BUFFER;
	in_params[0].Buffer.Length = sizeof(uuid);
	in_params[0].Buffer.Pointer = uuid;

	in_params[1].Type = ACPI_TYPE_INTEGER;
	in_params[1].Integer.Value = osc->revision;

	in_params[2].Type = ACPI_TYPE_INTEGER;
	in_params[2].Integer.Value = (osc->capabilities.Length / sizeof(ACPI_SIZE));

	in_params[3].Type = ACPI_TYPE_BUFFER;
	in_params[3].Buffer.Length = osc->capabilities.Length;
	in_params[3].Buffer.Pointer = osc->capabilities.Pointer;

	/*
	 * Evaluate Object
	 */
	status = AcpiEvaluateObject(device, "_OSC", &input, &output);

	if (ACPI_FAILURE(status))
		return status;

	if (!output.Length)
		return AE_NULL_OBJECT;

	out_param = output.Pointer;

	if (out_param->Type != ACPI_TYPE_BUFFER ||
	    out_param->Buffer.Length != osc->capabilities.Length) {
		/*
		 * OSC Returned wrong type, sounds wierd
		 */
		status = AE_TYPE;
		goto fail;
	}

	/*
	 * Now check error codes in query dword (Ignore bit 0)
	 */
	query_status = *((uint32_t*)out_param->Buffer.Pointer);
	query_status &= ~(ACPI_OSC_QUERY_ENABLE);

	if (query_status) {
		Log("OSC Query Failed, Status Word: 0x%x\n", query_status);
		status = AE_ERROR;
		goto fail;
	}

	/*
	 * Set return object
	 */
	osc->retval.Length = out_param->Buffer.Length;
	osc->retval.Pointer = out_param->Buffer.Pointer;

	/*
	 * Allocate a new buffer
	 */
	osc->retval.Pointer = AcpiOsAllocate(out_param->Buffer.Length);

	if (osc->retval.Pointer == NULL) {
		status = AE_NO_MEMORY;
		goto fail;
	} else
		memcpy(osc->retval.Pointer, out_param->Buffer.Pointer, out_param->Buffer.Length);

	/*
	 * Set ok
	 */
	status = AE_OK;

fail:
	/*
	 * Free
	 */
    AcpiOsFree(output.Pointer);

	/*
	 * Return
	 */
	if (status != AE_OK)
		osc->retval.Pointer = NULL;
	
	return status;
}

/*
 * Run OSC Support
 */
void AcpiCheckBusOscSupport(void)
{
	ACPI_HANDLE handle;
	uint32_t capabilities[2];
	struct _acpi_osc osc;
	uint32_t __unused apei_support;

	/*
	 * Setup OSC
	 */
	osc.uuid = sb_uuid_str;
	osc.revision = 1;
	osc.capabilities.Length = sizeof(capabilities);
	osc.capabilities.Pointer = capabilities;

	/*
	 * Setup capabilities
	 */
	capabilities[ACPI_OSC_QUERY_INDEX] = ACPI_OSC_QUERY_ENABLE;
	capabilities[ACPI_OSC_SUPPORT_INDEX] = ACPI_OSC_SUPPORT_SB_PR3_SUPPORT;

	/*
	 * Haha we have like zero power management support, gg wp
	 */

	/*
	 * Get root handle
	 */
	if (ACPI_FAILURE(AcpiGetHandle(NULL, "\\_SB", &handle)))
		return;

	/*
	 * Run OSC Query
	 */
	if (ACPI_SUCCESS(AcpiRunOscRequest(handle, &osc))) {
		/*
		 * Get capabilities
		 */
		uint32_t *capabilitybuffer = osc.retval.Pointer;

		/*
		 * Did we get more than we asked for?
		 */
		if (osc.retval.Length > ACPI_OSC_SUPPORT_INDEX)
			apei_support = capabilitybuffer[ACPI_OSC_SUPPORT_INDEX] & ACPI_OSC_SUPPORT_SB_APEI_SUPPORT;

		/*
		 * Cleanup
		 */
		AcpiOsFree(osc.retval.Pointer);
	}
}

/*
 * Enable or Disable OSI Interface
 */
void AcpiOsiSetup(char *OsiStr)
{
	OsiSetupEntry_t *osi;
	uint8_t Enable = 1;
	int i;

	if (*OsiStr == '!') {
		/*
		 * Go to next char
		 */
		OsiStr++;
		
		/*
		 * Disable all interfaces
		 */
		if (*OsiStr == '*') {
			/*
			 * Update
			 */
			AcpiUpdateInterfaces(ACPI_DISABLE_ALL_STRINGS);
			
			for (i = 0; i < OSI_STRING_ENTRIES_MAX; i++) {
				osi = &OsiSetupEntries[i];
				osi->Enable = 0;
			}
			
			return;
		}

		Enable = 0;
	}
	
	for (i = 0; i < OSI_STRING_ENTRIES_MAX; i++) {
		osi = &OsiSetupEntries[i];
		if (!strcmp(osi->String, OsiStr)) {
			osi->Enable = Enable;
			break;
		} else if (osi->String[0] == '\0') {
			osi->Enable = Enable;
			strncpy(osi->String, OsiStr, OSI_STRING_LENGTH_MAX);
			break;
		}
	}
}

/*
 * Install OSI Interfaces
 */
void AcpiOsiInstall(void)
{
	ACPI_STATUS __unused Status;
	int i;
	OsiSetupEntry_t *osi;
	char *str;

	/*
	 * Install ALL OSI Interfaces
	 */
	for (i = 0; i < OSI_STRING_ENTRIES_MAX; i++) {
		osi = &OsiSetupEntries[i];
		str = osi->String;

		if (*str == '\0')
			break;

		if (osi->Enable)
			Status = AcpiInstallInterface(str);
		else {
			Status = AcpiRemoveInterface(str);
		}
	}
}

/*
 * Initializes the full access and functionality
 * of ACPICA / ACPI and allows for scanning of
 * ACPI devices
 */
int AcpiInitialize(void)
{
	ACPI_STATUS Status;
	ACPI_OBJECT arg1;
	ACPI_OBJECT_LIST args;


	/*
	 * Debug
	 */
	LogInformation("ACPI", "Initializing");
	LogInformation("ACPI", "Installing OSI Interface");
	
	/*
	 * Install OSL Handler
	 */
	Status = AcpiInstallInterfaceHandler(AcpiOsi);

	/*
	 * We fake Windows 7
	 */
	AcpiOsiSetup("Windows 2009");

	/*
	 * Install
	 */
	AcpiOsiInstall();

	/*
	 * Initialize the ACPICA subsystem
	 */
	LogInformation("ACPI", "Installing Subsystems");
	Status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(Status)) {
		LogFatal("ACPI", "Failed to initialize subsystems, %u!", Status);
		return -1;
	}

	/*
	 * Install the default address space handlers.
	 */
	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
		ACPI_ADR_SPACE_SYSTEM_MEMORY, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status)) {
		LogDebug("ACPI", "Could not initialise SystemMemory handler, %s!", 
			AcpiFormatException(Status));
	}

	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
		ACPI_ADR_SPACE_SYSTEM_IO, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status)) {
		LogDebug("ACPI", "Could not initialise SystemIO handler, %s!",
			AcpiFormatException(Status));
	}

	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
		ACPI_ADR_SPACE_PCI_CONFIG, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status)) {
		LogDebug("ACPI", "Could not initialise PciConfig handler, %s!",
			AcpiFormatException(Status));
	}

	/*
	 * Create the ACPI namespace from ACPI tables
	 */
	LogInformation("ACPI", "Loading Tables");
	Status = AcpiLoadTables();
	if (ACPI_FAILURE(Status)) {
		LogFatal("ACPI", "Failed LoadTables, %u!", Status);
        return -1;
	}

	/*
	 * Initialize the ACPI hardware
	 */
	LogInformation("ACPI", "Enabling Subsystems");
	Status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(Status)) {
		LogFatal("ACPI", "Failed AcpiEnableSubsystem, %u!", Status);
        return -1;
	}
	
	/*
	 * Probe for EC here
	 */

	/*
	 * Complete the ACPI namespace object initialization
	 */
	LogInformation("ACPI", "Initializing Objects");
	Status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(Status)) {
		LogFatal("ACPI", "Failed AcpiInitializeObjects, %u!", Status);
        return -1;
	}

	/*
	 * Run _OSC on root, it should always be run after InitializeObjects
	 */
	AcpiCheckBusOscSupport();

	/*
	 * Set APIC Mode
	 */
	arg1.Type = ACPI_TYPE_INTEGER;
	arg1.Integer.Value = 1;				/* 0 - PIC, 1 - IOAPIC, 2 - IOSAPIC */
	args.Count = 1;
	args.Pointer = &arg1;

	AcpiEvaluateObject(ACPI_ROOT_OBJECT, "_PIC", &args, NULL);

	LogInformation("ACPI", "Installing Event Handlers");

	/*
	 * Install a notify handler
	 */
	AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, AcpiBusNotifyHandler, NULL);

	/*
	 * Install a global event handler
	 */
	AcpiInstallGlobalEventHandler(AcpiEventHandler, NULL);
	AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, AcpiShutdownHandler, NULL);
	AcpiInstallFixedEventHandler(ACPI_EVENT_SLEEP_BUTTON, AcpiSleepHandler, NULL);

	return 0;
}
