#include "common.h"
#include "cmd.h"

void vivsysUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS vivsysCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vivsysDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vivsysDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS GetModuleBase(ULONGLONG ullModHash, PVOID* ppBaseAddress);

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
        case ALLOC_POOL:
        {
            KdPrint(("vivsys.sys: Got alloc_pool request\n"));
            nts = doPoolAlloc(Irp, ioBuffer, inLength, outLength);
            break;
        }
        case FREE_POOL:
        {
            KdPrint(("vivsys.sys: Got free_pool request\n"));
            nts = doPoolFree(Irp, ioBuffer, inLength, outLength);
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
