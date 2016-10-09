/**
 * @file
 * ACPI sylixos.
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
#define __SYLIXOS_STDIO
#include "acpi.h"
#include "accommon.h"
#include "stdio.h"
#include <linux/compat.h>

ACPI_MODULE_NAME("sylixos")

/*
 * Globals
 */
static volatile void *Acpi_RedirectionTarget = NULL;

/******************************************************************************
*
* FUNCTION:    AcpiOsInitialize
*
* PARAMETERS:  None
*
* RETURN:      Status
*
* DESCRIPTION: Init this OSL
*
*****************************************************************************/

ACPI_STATUS AcpiOsInitialize(void)
{
	Acpi_RedirectionTarget = NULL;

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsTerminate
*
* PARAMETERS:  None
*
* RETURN:      Status
*
* DESCRIPTION: Nothing to do for SylixOS
*
*****************************************************************************/

ACPI_STATUS AcpiOsTerminate(void)
{
	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsGetRootPointer
*
* PARAMETERS:  None
*
* RETURN:      RSDP physical address
*
* DESCRIPTION: Gets the root pointer (RSDP)
*
*****************************************************************************/

/*
 The ACPI specification is highly portable specification, however, 
 it has a static part which is generally non-portable: the location of the Root System Descriptor Pointer. 
 This pointer may be found in many different ways depending on the chipset. 
 On PC-compatible computers (without EFI) it is located in lower memory generally 
 somewhere between 0x80000 and 0x100000. However, even within the PC compatible platform, 
 an EFI-enabled board will export the RSDP to the OS on when it loads it through the EFI system tables. 
 Other boards on server machines which are not PC-compatibles, 
 like embedded and handheld devices which implement ACPI will again, 
 not all be expected to position the RSDP in the same place as any other board. 
 The RSDP is therefore located in a chipset-specific manner; From the time the OS has the RSDP, 
 the rest of ACPI is completely portable. However, the way the RSDP is found is not. 
 This would be the reason that the ACPICA code wouldn't try to provide routines to expressly find the RSDP in a portable manner.
 */

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
	ACPI_PHYSICAL_ADDRESS Ret;

	AcpiFindRootPointer(&Ret);
	return Ret;
}

/******************************************************************************
*
* FUNCTION:    AcpiOsPredefinedOverride
*
* PARAMETERS:  InitVal             - Initial value of the predefined object
*              NewVal              - The new value for the object
*
* RETURN:      Status, pointer to value. Null pointer returned if not
*              overriding.
*
* DESCRIPTION: Allow the OS to override predefined names
*
*****************************************************************************/

ACPI_STATUS AcpiOsPredefinedOverride(
			const ACPI_PREDEFINED_NAMES *InitVal,
			ACPI_STRING                 *NewVal)
{
	if (!InitVal || !NewVal) {
		return (AE_BAD_PARAMETER);
	}

	*NewVal = NULL;
	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsTableOverride
*
* PARAMETERS:  ExistingTable       - Header of current table (probably firmware)
*              NewTable            - Where an entire new table is returned.
*
* RETURN:      Status, pointer to new table. Null pointer returned if no
*              table is available to override
*
* DESCRIPTION: Return a different version of a table if one is available
*
*****************************************************************************/

ACPI_STATUS AcpiOsTableOverride(
			ACPI_TABLE_HEADER       *ExistingTable,
			ACPI_TABLE_HEADER       **NewTable)
{
	if (!ExistingTable || !NewTable) {
		return (AE_BAD_PARAMETER);
	}

	*NewTable = NULL;
	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsPhysicalTableOverride
*
* PARAMETERS:  ExistingTable       - Header of current table (probably firmware)
*              NewAddress          - Where new table address is returned
*                                    (Physical address)
*              NewTableLength      - Where new table length is returned
*
* RETURN:      Status, address/length of new table. Null pointer returned
*              if no table is available to override.
*
* DESCRIPTION: Returns AE_SUPPORT.
*
*****************************************************************************/

ACPI_STATUS AcpiOsPhysicalTableOverride(
			ACPI_TABLE_HEADER       *ExistingTable,
			ACPI_PHYSICAL_ADDRESS   *NewAddress,
			UINT32                  *NewTableLength)
{
	return (AE_NOT_IMPLEMENTED);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsMapMemory
*
* PARAMETERS:  Where               - Physical address of memory to be mapped
*              Length              - How much memory to map
*
* RETURN:      Pointer to mapped memory. Null on error.
*
* DESCRIPTION: Map physical memory into caller's address space
*
*****************************************************************************/

void *AcpiOsMapMemory(
	  ACPI_PHYSICAL_ADDRESS   Where,
	  ACPI_SIZE               Length)
{ 
    addr_t  ulPhyBase = ROUND_DOWN(Where, LW_CFG_VMM_PAGE_SIZE);
    addr_t  ulOffset  = Where - ulPhyBase;
    addr_t  ulVirBase;

    Length   += ulOffset;
    Length    = ROUND_UP(Length, LW_CFG_VMM_PAGE_SIZE);

    ulVirBase = (addr_t)API_VmmIoRemapNocache((PVOID)ulPhyBase, Length);
    if (ulVirBase) {
        return (void *)(ulVirBase + ulOffset);
    } else {
        return (void *)(NULL);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsUnmapMemory
*
* PARAMETERS:  Where               - Logical address of memory to be unmapped
*              Length              - How much memory to unmap
*
* RETURN:      None.
*
* DESCRIPTION: Delete a previously created mapping. Where and Length must
*              correspond to a previous mapping exactly.
*
*****************************************************************************/

void AcpiOsUnmapMemory(void *Where, ACPI_SIZE Length)
{
    API_VmmIoUnmap(Where);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsAllocate
*
* PARAMETERS:  Size                - Amount to allocate, in bytes
*
* RETURN:      Pointer to the new allocation. Null on error.
*
* DESCRIPTION: Allocate memory. Algorithm is dependent on the OS.
*
*****************************************************************************/

void *AcpiOsAllocate(ACPI_SIZE Size)
{
	void *ret;

	ret = sys_malloc(Size);
	if (ret) {
	    lib_bzero(ret, Size);
	}

	return ret;
}

/******************************************************************************
*
* FUNCTION:    AcpiOsFree
*
* PARAMETERS:  Mem                 - Pointer to previously allocated memory
*
* RETURN:      None.
*
* DESCRIPTION: Free memory allocated via AcpiOsAllocate
*
*****************************************************************************/

void AcpiOsFree(void *Mem)
{
    sys_free(Mem);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsInstallInterruptHandler
*
* PARAMETERS:  InterruptNumber     - Level handler should respond to.
*              ServiceRoutine      - Address of the ACPI interrupt handler
*              Context             - User context
*
* RETURN:      Handle to the newly installed handler.
*
* DESCRIPTION: Install an interrupt handler. Used to install the ACPI
*              OS-independent handler.
*
*****************************************************************************/

UINT32 AcpiOsInstallInterruptHandler(
       UINT32                  InterruptNumber,
	   ACPI_OSD_HANDLER        ServiceRoutine,
	   void                    *Context)
{
    API_InterVectorSetFlag(InterruptNumber, LW_IRQ_FLAG_QUEUE);

    API_InterVectorConnect(InterruptNumber,
                           (PINT_SVR_ROUTINE)ServiceRoutine,
                           (PVOID)Context,
                           "acpi_isr");                                 /*  安装操作系统中断向量表      */

    API_InterVectorEnable(InterruptNumber);

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsRemoveInterruptHandler
*
* PARAMETERS:  Handle              - Returned when handler was installed
*
* RETURN:      Status
*
* DESCRIPTION: Uninstalls an interrupt handler.
*
*****************************************************************************/

ACPI_STATUS AcpiOsRemoveInterruptHandler(
			UINT32                  InterruptNumber,
			ACPI_OSD_HANDLER        ServiceRoutine)
{
    API_InterVectorDisable(InterruptNumber);

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsStall
*
* PARAMETERS:  Microseconds        - Time to stall
*
* RETURN:      None. Blocks until stall is completed.
*
* DESCRIPTION: Sleep at microsecond granularity (1 Milli = 1000 Micro)
*
*****************************************************************************/

void AcpiOsStall(UINT32 Microseconds)
{
	API_TimeMSleep((Microseconds / 1000) + 1);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsSleep
*
* PARAMETERS:  Milliseconds        - Time to sleep
*
* RETURN:      None. Blocks until sleep is completed.
*
* DESCRIPTION: Sleep at millisecond granularity
*
*****************************************************************************/

void AcpiOsSleep(UINT64 Milliseconds)
{
    API_TimeMSleep(Milliseconds);
}

/*********************************************************************************************************
  PCI 主控器配置地址 PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_CONFIG_ADDR0()          PCI_CONFIG_ADDR
#define PCI_CONFIG_ADDR1()          PCI_CONFIG_DATA

/*********************************************************************************************************
  PCI 主控器配置 IO 操作 PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_IN_BYTE(addr)           in8((addr))
#define PCI_IN_WORD(addr)           in16((addr))
#define PCI_IN_DWORD(addr)          in32((addr))
#define PCI_OUT_BYTE(addr, data)    out8((UINT8)(data), (addr))
#define PCI_OUT_WORD(addr, data)    out16((UINT16)(data), (addr))
#define PCI_OUT_DWORD(addr, data)   out32((UINT32)(data), (addr))

static INT  x86PciConfigInByte (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 *pucValue)
{
    UINT8   ucRet = 0;
    INT     iRetVal = PX_ERROR;

    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    ucRet = PCI_IN_BYTE(PCI_CONFIG_ADDR1() + (iOft & 0x3));
    iRetVal = ERROR_NONE;

    if (pucValue) {
        *pucValue = ucRet;
    }

    return  (iRetVal);
}

static INT  x86PciConfigInWord (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 *pusValue)
{
    UINT16  usRet = 0;
    INT     iRetVal = PX_ERROR;

    if (iOft & 0x01) {
        return  (PX_ERROR);
    }

    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    usRet = PCI_IN_WORD(PCI_CONFIG_ADDR1() + (iOft & 0x2));
    iRetVal = ERROR_NONE;

    if (pusValue) {
        *pusValue = usRet;
    }

    return  (iRetVal);
}

static INT  x86PciConfigInDword (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 *puiValue)
{
    UINT32  uiRet   = 0;
    INT     iRetVal = PX_ERROR;

    if (iOft & 0x03) {
        return  (PX_ERROR);
    }

    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    uiRet = PCI_IN_DWORD(PCI_CONFIG_ADDR1());
    iRetVal = ERROR_NONE;

    if (puiValue) {
        *puiValue = uiRet;
    }

    return  (iRetVal);
}

static INT  x86PciConfigOutByte (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 ucValue)
{
    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    PCI_OUT_BYTE((PCI_CONFIG_ADDR1() + (iOft & 0x3)), ucValue);

    return  (ERROR_NONE);
}

static INT  x86PciConfigOutWord (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 usValue)
{
    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    PCI_OUT_WORD((PCI_CONFIG_ADDR1() + (iOft & 0x2)), usValue);

    return  (ERROR_NONE);
}

static INT  x86PciConfigOutDword (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 uiValue)
{
    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    PCI_OUT_DWORD(PCI_CONFIG_ADDR1(), uiValue);

    return  (ERROR_NONE);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsReadPciConfiguration
*
* PARAMETERS:  PciId               - Seg/Bus/Dev
*              Register            - Device Register
*              Value               - Buffer where value is placed
*              Width               - Number of bits
*
* RETURN:      Status
*
* DESCRIPTION: Read data from PCI configuration space
*
*****************************************************************************/

ACPI_STATUS AcpiOsReadPciConfiguration(
			ACPI_PCI_ID             *PciId,
			UINT32                  Register,
			UINT64                  *Value,
			UINT32                  Width)
{
	switch (Width) {
		case 8: {
		    x86PciConfigInByte(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT8 *)Value);
		} break;

		case 16: {
			x86PciConfigInWord(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT16 *)Value);
		} break;

		case 32: {
			x86PciConfigInDword(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT32 *)Value);
		} break;

		default:
			return (AE_ERROR);
	}

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWritePciConfiguration
*
* PARAMETERS:  PciId               - Seg/Bus/Dev
*              Register            - Device Register
*              Value               - Value to be written
*              Width               - Number of bits
*
* RETURN:      Status
*
* DESCRIPTION: Write data to PCI configuration space
*
*****************************************************************************/

ACPI_STATUS AcpiOsWritePciConfiguration(
			ACPI_PCI_ID             *PciId,
			UINT32                  Register,
			UINT64                  Value,
			UINT32                  Width)
{
	switch (Width) {
		case 8: {
		    x86PciConfigOutByte(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT8)Value);
		} break;

		case 16: {
		    x86PciConfigOutWord(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT16)Value);
		} break;

		case 32: {
		    x86PciConfigOutDword(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT32)Value);
		} break;

		default:
			return (AE_ERROR);
	}

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsReadPort
*
* PARAMETERS:  Address             - Address of I/O port/register to read
*              Value               - Where value is placed
*              Width               - Number of bits
*
* RETURN:      Value read from port
*
* DESCRIPTION: Read data from an I/O port or register
*
*****************************************************************************/

ACPI_STATUS AcpiOsReadPort(
            ACPI_IO_ADDRESS         Address,
            UINT32                  *Value,
            UINT32                  Width)
{
	ACPI_FUNCTION_NAME(OsReadPort);

	switch (Width) {
	case 8:
		*Value = in8((UINT16)Address);
		break;

	case 16:
		*Value = in16((UINT16)Address);
		break;

	case 32:
		*Value = in32((UINT16)Address);
		break;

	default:
		ACPI_ERROR((AE_INFO, "Bad width parameter: %X", Width));
		return (AE_BAD_PARAMETER);
	}

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWritePort
*
* PARAMETERS:  Address             - Address of I/O port/register to write
*              Value               - Value to write
*              Width               - Number of bits
*
* RETURN:      None
*
* DESCRIPTION: Write data to an I/O port or register
*
*****************************************************************************/

ACPI_STATUS AcpiOsWritePort(
            ACPI_IO_ADDRESS         Address,
            UINT32                  Value,
            UINT32                  Width)
{
	ACPI_FUNCTION_NAME(OsWritePort);

    switch (Width) {
    case 8:
        out8((UINT8)Value, (UINT16)Address);
        break;

    case 16:
        out16((UINT16)Value, (UINT16)Address);
        break;

    case 32:
        out32((UINT32)Value, (UINT16)Address);
        break;

    default:
        ACPI_ERROR((AE_INFO, "Bad width parameter: %X", Width));
        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsReadMemory
*
* PARAMETERS:  Address             - Physical Memory Address to read
*              Value               - Where value is placed
*              Width               - Number of bits (8,16,32, or 64)
*
* RETURN:      Value read from physical memory address. Always returned
*              as a 64-bit integer, regardless of the read width.
*
* DESCRIPTION: Read data from a physical memory address
*
*****************************************************************************/

ACPI_STATUS AcpiOsReadMemory(
            ACPI_PHYSICAL_ADDRESS   Address,
            UINT64                  *Value,
            UINT32                  Width)
{
    switch (Width) {
    case 8:
        *Value = read8(Address);
        break;

    case 16:
        *Value = read16(Address);
        break;

    case 32:
        *Value = read32(Address);
        break;

    default:
        ACPI_ERROR((AE_INFO, "Bad width parameter: %X", Width));
        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWriteMemory
*
* PARAMETERS:  Address             - Physical Memory Address to write
*              Value               - Value to write
*              Width               - Number of bits (8,16,32, or 64)
*
* RETURN:      None
*
* DESCRIPTION: Write data to a physical memory address
*
*****************************************************************************/

ACPI_STATUS AcpiOsWriteMemory(
            ACPI_PHYSICAL_ADDRESS   Address,
            UINT64                  Value,
            UINT32                  Width)
{
    switch (Width) {
    case 8:
        write8((UINT8)Value, Address);
        break;

    case 16:
        write16((UINT16)Value, Address);
        break;

    case 32:
        write32((UINT32)Value, Address);
        break;

    default:
        ACPI_ERROR((AE_INFO, "Bad width parameter: %X", Width));
        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsSignal
*
* PARAMETERS:  Function            - ACPICA signal function code
*              Info                - Pointer to function-dependent structure
*
* RETURN:      Status
*
* DESCRIPTION: Miscellaneous functions. Example implementation only.
*
*****************************************************************************/

ACPI_STATUS AcpiOsSignal(
            UINT32                  Function,
            void                    *Info)
{
	switch (Function) {
	case ACPI_SIGNAL_FATAL:
		break;

	case ACPI_SIGNAL_BREAKPOINT:
		break;

	default:
		break;
	}

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsGetThreadId
*
* PARAMETERS:  None
*
* RETURN:      Id of the running thread
*
* DESCRIPTION: Get the Id of the current (running) thread
*
*****************************************************************************/

ACPI_THREAD_ID AcpiOsGetThreadId(void)
{
	return API_ThreadIdSelf();
}

/******************************************************************************
*
* FUNCTION:    AcpiOsExecute
*
* PARAMETERS:  Type                - Type of execution
*              Function            - Address of the function to execute
*              Context             - Passed as a parameter to the function
*
* RETURN:      Status
*
* DESCRIPTION: Execute a new thread
*
*****************************************************************************/

ACPI_STATUS AcpiOsExecute(
            ACPI_EXECUTE_TYPE       Type,
            ACPI_OSD_EXEC_CALLBACK  Function,
            void                    *Context)
{
    LW_CLASS_THREADATTR  threadattr = API_ThreadAttrGetDefault();

    threadattr.THREADATTR_pvArg = Context;

    API_ThreadCreate("t_acpi", (PTHREAD_START_ROUTINE)Function, &threadattr, NULL);

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsPrintf
*
* PARAMETERS:  Fmt, ...            - Standard printf format
*
* RETURN:      None
*
* DESCRIPTION: Formatted output
*
*****************************************************************************/

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Fmt, ...)
{
	va_list		Args;
	
	va_start(Args, Fmt);
	AcpiOsVprintf(Fmt, Args);
	va_end(Args);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsVprintf
*
* PARAMETERS:  Fmt                 - Standard printf format
*              Args                - Argument list
*
* RETURN:      None
*
* DESCRIPTION: Formatted output with argument list pointer
*
*****************************************************************************/

void AcpiOsVprintf(const char *Fmt, va_list Args)
{
	char Buffer[512];

	/* Clear Buffer */
	memset(Buffer, 0, 512);

	/* Format To Buffer */
	vsprintf(Buffer, Fmt, Args);

	printk(KERN_INFO "ACPI: %s", Buffer);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsGetLine
*
* PARAMETERS:  Buffer              - Where to return the command line
*              BufferLength        - Maximum length of Buffer
*              BytesRead           - Where the actual byte count is returned
*
* RETURN:      Status and actual bytes read
*
* DESCRIPTION: Formatted input with argument list pointer
*
*****************************************************************************/

ACPI_STATUS AcpiOsGetLine(
            char                    *Buffer,
            UINT32                  BufferLength,
            UINT32                  *BytesRead)
{
	int                     Temp = EOF;
	UINT32                  i;

	for (i = 0;; i++) {
		if (i >= BufferLength) {
			return (AE_BUFFER_OVERFLOW);
		}

		if ((Temp = getchar()) == EOF) {
			return (AE_ERROR);
		}

		if (!Temp || Temp == '\n') {
			break;
		}

		Buffer[i] = (char)Temp;
	}

	/*
	 * Null terminate the buffer
	 */
	Buffer[i] = 0;

	/*
	 * Return the number of bytes in the string
	 */
	if (BytesRead) {
		*BytesRead = i;
	}

	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    Spinlock interfaces
*
* DESCRIPTION: Map these interfaces to semaphore interfaces
*
*****************************************************************************/

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
    spinlock_t *pLock = (spinlock_t *)AcpiOsAllocate(sizeof(spinlock_t));

    if (pLock) {
        LW_SPIN_INIT(pLock);
        *OutHandle = (ACPI_SPINLOCK)pLock;
        return (AE_OK);

    } else {
        *OutHandle = NULL;
        return (AE_ERROR);
    }
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
    sys_free(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
    ACPI_CPU_FLAGS  intreg;

    LW_SPIN_LOCK_IRQ((spinlock_t *)Handle, &intreg);
	return intreg;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
    LW_SPIN_UNLOCK_IRQ((spinlock_t *)Handle, Flags);
}

/******************************************************************************
*
* FUNCTION:    Semaphore functions
*
* DESCRIPTION: Stub functions used for single-thread applications that do
*              not require semaphore synchronization. Full implementations
*              of these functions appear after the stubs.
*
*****************************************************************************/

ACPI_STATUS AcpiOsCreateSemaphore(
            UINT32              MaxUnits,
            UINT32              InitialUnits,
            ACPI_SEMAPHORE     *OutHandle)
{
	*OutHandle = (ACPI_SEMAPHORE)API_SemaphoreCCreate("sem_acpi", InitialUnits, MaxUnits,
	             (LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL), NULL);
	if (*OutHandle != LW_HANDLE_INVALID) {
	    return (AE_OK);
	} else {
        return (AE_ERROR);
	}
}

ACPI_STATUS AcpiOsDeleteSemaphore(
            ACPI_SEMAPHORE      Handle)
{
    API_SemaphoreCDelete((LW_HANDLE *)&Handle);

	return (AE_OK);
}

ACPI_STATUS AcpiOsWaitSemaphore(
            ACPI_SEMAPHORE      Handle,
            UINT32              Units,
            UINT16              Timeout)
{
    if (Handle) {
        API_SemaphoreCPend((LW_HANDLE)Handle,
                           Timeout == ACPI_DO_NOT_WAIT ? LW_OPTION_NOT_WAIT : LW_OPTION_WAIT_INFINITE);
    }

	return (AE_OK);
}

ACPI_STATUS AcpiOsSignalSemaphore(
            ACPI_SEMAPHORE      Handle,
            UINT32              Units)
{
    if (Handle) {
        API_SemaphoreCRelease((LW_HANDLE)Handle, Units, NULL);
    }

	return (AE_OK);
}

ACPI_STATUS AcpiOsCreateMutex (
            ACPI_MUTEX              *OutHandle)
{
    *OutHandle = (ACPI_SEMAPHORE)API_SemaphoreMCreate("mutex_acpi", 10,
                 (LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL | LW_OPTION_RECURSIVE), NULL);
    if (*OutHandle != LW_HANDLE_INVALID) {
        return (AE_OK);
    } else {
        return (AE_ERROR);
    }
}

void AcpiOsDeleteMutex (
     ACPI_MUTEX              Handle)
{
    API_SemaphoreMDelete((LW_HANDLE *)&Handle);
}

ACPI_STATUS AcpiOsAcquireMutex (
            ACPI_MUTEX              Handle,
            UINT16                  Timeout)
{
    if (Handle) {
        API_SemaphoreMPend((LW_HANDLE)Handle,
                           Timeout == ACPI_DO_NOT_WAIT ? LW_OPTION_NOT_WAIT : LW_OPTION_WAIT_INFINITE);
    }

    return (AE_OK);
}

void AcpiOsReleaseMutex (
     ACPI_MUTEX              Handle)
{
    if (Handle) {
        API_SemaphoreMPost((LW_HANDLE)Handle);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsGetTimer
*
* PARAMETERS:  None
*
* RETURN:      Current ticks in 100-nanosecond units
*
* DESCRIPTION: Get the value of a system timer
*
******************************************************************************/

UINT64 AcpiOsGetTimer(void)
{
	return API_TimeGet64() * 10000000 / LW_TICK_HZ;
}

/******************************************************************************
*
* FUNCTION:    AcpiOsReadable
*
* PARAMETERS:  Pointer             - Area to be verified
*              Length              - Size of area
*
* RETURN:      TRUE if readable for entire length
*
* DESCRIPTION: Verify that a pointer is valid for reading
*
*****************************************************************************/

BOOLEAN AcpiOsReadable(void *Pointer, ACPI_SIZE Length)
{
	return TRUE;
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWritable
*
* PARAMETERS:  Pointer             - Area to be verified
*              Length              - Size of area
*
* RETURN:      TRUE if writable for entire length
*
* DESCRIPTION: Verify that a pointer is valid for writing
*
*****************************************************************************/

BOOLEAN AcpiOsWritable(void *Pointer, ACPI_SIZE Length)
{
	return TRUE;
}

/******************************************************************************
*
* FUNCTION:    AcpiOsRedirectOutput
*
* PARAMETERS:  Destination         - An open file handle/pointer
*
* RETURN:      None
*
* DESCRIPTION: Causes redirect of AcpiOsPrintf and AcpiOsVprintf
*
*****************************************************************************/

void AcpiOsRedirectOutput(void *Destination)
{
	Acpi_RedirectionTarget = Destination;
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWaitEventsComplete
*
* PARAMETERS:  None
*
* RETURN:      None
*
* DESCRIPTION: Wait for all asynchronous events to complete. This
*              implementation does nothing.
*
*****************************************************************************/

void AcpiOsWaitEventsComplete(void)
{
	return;
}

/*
 * Stubs for the disassembler
 */
#include "include/acdisasm.h"

void MpSaveGpioInfo(
     ACPI_PARSE_OBJECT       *Op,
     AML_RESOURCE            *Resource,
     UINT32                  PinCount,
     UINT16                  *PinList,
     char                    *DeviceName)
{
}

void MpSaveSerialInfo(
     ACPI_PARSE_OBJECT       *Op,
     AML_RESOURCE            *Resource,
     char                    *DeviceName)
{
}

