#include "common.h"

#define READ_KMEM CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                            0x800,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define GET_KMOD CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                            0x801,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define WRITE_PORT CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                            0x802,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define READ_PORT CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                            0x803,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define LOWDWORD(x) ((ULONG)(((ULONG_PTR)(x)) & 0xffffffff))

void vivsysUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS vivsysCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vivsysDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vivsysDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS GetModuleBase(ULONGLONG ullModHash, PVOID* ppBaseAddress);


#pragma pack(push)
#pragma pack(1)
typedef struct _CMD_READ_KMEM
{
    ULONGLONG BaseAddr;
    ULONG Size;
}CMD_READ_KMEM;

typedef struct _CMD_READ_PORT
{
    ULONG Port;
    ULONG Count;
}CMD_READ_PORT;

typedef struct _CMD_WRITE_PORT
{
	ULONG Port;
	ULONG Count;
	BYTE Data[1];
}CMD_WRITE_PORT;

#pragma pack(pop)

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
    UNICODE_STRING DeviceName = { 0 };
    UNICODE_STRING Win32Device = { 0 };
    PDEVICE_OBJECT DeviceObject = NULL;
    NTSTATUS nts = STATUS_SUCCESS;
    ULONG i = 0;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&DeviceName, L"\\Device\\vivsys");
    RtlInitUnicodeString(&Win32Device, L"\\DosDevices\\vivsys");

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = vivsysDefaultHandler;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = vivsysCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = vivsysCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = vivsysDispatch;
    DriverObject->DriverUnload = vivsysUnload;

    nts = IoCreateDevice(DriverObject,
        0,
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &DeviceObject);

    if (!NT_SUCCESS(nts))
    {
        return nts;
    }

    if (!DeviceObject)
    {
        return STATUS_UNEXPECTED_IO_ERROR;
    }

	KdPrint(("Hello World!\n"));

    DeviceObject->Flags |= DO_DIRECT_IO;
    DeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;
    nts = IoCreateSymbolicLink(&Win32Device, &DeviceName);

    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

void vivsysUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING Win32Device;
    RtlInitUnicodeString(&Win32Device, L"\\DosDevices\\vivsys");
    IoDeleteSymbolicLink(&Win32Device);
    IoDeleteDevice(DriverObject->DeviceObject);

    KdPrint(("Goodbye World!\n"));
}

NTSTATUS doReadKernMem(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength)
{
    NTSTATUS nts = STATUS_UNSUCCESSFUL;
    PMDL mdl = NULL;
    PVOID kAddr = NULL;
    ULONG kMemSize = 0;
    PVOID pSysBuff = NULL;

    if (NULL == ioBuffer ||
        sizeof(CMD_READ_KMEM) != inLength ||
        0 == outLength ||
        0 == ((CMD_READ_KMEM*)ioBuffer)->Size)
    {
        nts = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

#ifndef _WIN64
    kAddr = (PVOID)LOWDWORD(((CMD_READ_KMEM*)ioBuffer)->BaseAddr);
#else
    kAddr = (PVOID)((CMD_READ_KMEM*)ioBuffer)->BaseAddr;
#endif

    kMemSize = ((CMD_READ_KMEM*)ioBuffer)->Size;
    RtlSecureZeroMemory(ioBuffer, outLength);

    if (kAddr <= MmHighestUserAddress)
    {
		KdPrint(("Tried to Read a user-mode addr: 0x%p\n", kAddr));
        nts = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    __try
    {
        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            mdl = IoAllocateMdl(kAddr, kMemSize, FALSE, FALSE, NULL);
            if (NULL == mdl)
            {
                nts = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
            pSysBuff = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
            if (NULL == pSysBuff)
            {
                nts = STATUS_INVALID_PARAMETER;
                goto cleanup;
            }
        }
        else
        {
            pSysBuff = kAddr;
        }
       
        nts = STATUS_SUCCESS;
        RtlCopyMemory(ioBuffer, pSysBuff, outLength);
        Irp->IoStatus.Information = outLength;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        nts = GetExceptionCode();
		KdPrint(("Exception reading address 0x%p: 0x%x", kAddr, nts));
        goto cleanup;
    }

cleanup:
    if (mdl)
    {
        MmUnlockPages(mdl);
        IoFreeMdl(mdl);
        mdl = NULL;
    }
    return nts;
}

NTSTATUS getKernModList(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength)
{
    PSYSTEM_MODULE_INFORMATION pSysModList = NULL;
    ULONG ulBufferSize = 0;
	ULONG ulTotalSize = 0;
	ULONG_PTR pTmp = 0;
	NTSTATUS nts = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(inLength);

	if (NULL == ioBuffer ||
        0 == outLength)
	{
		nts = STATUS_INVALID_PARAMETER;
		goto cleanup;
	}

    pTmp = (ULONG_PTR)ioBuffer;

    nts = GetSystemModuleArray(&pSysModList, &ulBufferSize);
    if (!NT_SUCCESS(nts))
    {
        goto cleanup;
    }

	if (S_OK != ULongAdd(ulBufferSize, sizeof(ULONG), &ulTotalSize))
	{
		nts = STATUS_INTEGER_OVERFLOW;
		goto cleanup;
	}

	if (ulTotalSize > outLength)
    {
        nts = STATUS_INFO_LENGTH_MISMATCH;
        goto cleanup;
    }

	
	*(ULONG*)pTmp = sizeof(ULONG_PTR);
	pTmp += sizeof(ULONG);

	RtlCopyMemory((PVOID)pTmp, pSysModList, ulBufferSize);
    Irp->IoStatus.Information = ulBufferSize;
cleanup:
    if (pSysModList)
    {
        ExFreePool(pSysModList);
        pSysModList = NULL;
    }

	return nts;
}

NTSTATUS doWriteIoPort(PVOID ioBuffer, ULONG inLength)
{
	NTSTATUS nts = STATUS_SUCCESS;
	PUCHAR port = NULL;
	ULONG count = 0;
    //UCHAR pTmp[250] = { 0 };

	if (NULL == ioBuffer || 
		sizeof(CMD_WRITE_PORT) > inLength ||
		0 == ((CMD_WRITE_PORT*)ioBuffer)->Count)
	{
		return STATUS_INVALID_PARAMETER;
	}

	port = (PUCHAR)((CMD_WRITE_PORT*)ioBuffer)->Port;
	count = ((CMD_WRITE_PORT*)ioBuffer)->Count;

    WRITE_PORT_BUFFER_UCHAR(port, ((CMD_WRITE_PORT*)ioBuffer)->Data, count);

	return nts;
}

NTSTATUS doReadIoPort(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength)
{
    NTSTATUS nts = STATUS_SUCCESS;
    PUCHAR port = NULL;
    ULONG count = 0;

    if (NULL == ioBuffer ||
        sizeof(CMD_READ_PORT) != inLength ||
        ((CMD_READ_PORT*)ioBuffer)->Count != outLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    port = (PUCHAR)((CMD_READ_PORT*)ioBuffer)->Port;
    count = ((CMD_READ_PORT*)ioBuffer)->Count;

    READ_PORT_BUFFER_UCHAR(port, (PUCHAR)ioBuffer, count);
    Irp->IoStatus.Information = count;
    return nts;
}

NTSTATUS handleCommand(PIRP Irp, ULONG ctlCode, PVOID ioBuffer, ULONG inLength, ULONG outLength)
{
    NTSTATUS nts = STATUS_UNSUCCESSFUL;

    if (NULL == Irp)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Commands go here
    switch (ctlCode)
    {
        case READ_KMEM:
        {
            KdPrint(("vivsys.sys: Got read_kmem request\n"));
            nts = doReadKernMem(Irp, ioBuffer, inLength, outLength);
            break;
        }
		case GET_KMOD:
		{
			KdPrint(("vivsys.sys: Got get_kmod request\n"));
			nts = getKernModList(Irp, ioBuffer, inLength, outLength);
			break;
		}
		case WRITE_PORT:
		{
			KdPrint(("vivsys.sys: Got write_port request\n"));
			nts = doWriteIoPort(ioBuffer, inLength);
			break;
		}
        case READ_PORT:
        {
            KdPrint(("vivsys.sys: Got read_port request\n"));
            nts = doReadIoPort(Irp, ioBuffer, inLength, outLength);
            break;
        }
        default:
        {

            nts = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }
    }

cleanup:
    return nts;
}





NTSTATUS vivsysDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS            nts = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION  irpStack = NULL;
    PVOID               ioBuffer = NULL;
    ULONG               inLength = 0;
    ULONG               outLength = 0;
    ULONG               ioctl = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Status = nts;
    Irp->IoStatus.Information = 0;

    //
    // Get the pointer to the input/output buffer and its length
    //
    ioBuffer = Irp->AssociatedIrp.SystemBuffer;
    inLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_DEVICE_CONTROL:
        {
            ioctl = irpStack->Parameters.DeviceIoControl.IoControlCode;
            nts = handleCommand(Irp, ioctl, ioBuffer, inLength, outLength);
            break;
        }
    }

    Irp->IoStatus.Status = nts;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return nts;
}

NTSTATUS vivsysCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS vivsysDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
}
