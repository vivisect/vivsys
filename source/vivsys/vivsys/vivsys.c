#include <ntddk.h>
#include <ntddstor.h>
#include <mountdev.h>
#include <ntddvol.h>

#define READ_KMEM CTL_CODE(FILE_DEVICE_UNKNOWN,       \
                            0x800,                     \
                            METHOD_BUFFERED,           \
                            FILE_ANY_ACCESS)

void vivsysUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS vivsysCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vivsysDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS vivsysDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif

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

    DbgPrint("Hello World!\n");

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

    DbgPrint("Goodbye World!\n");
}

NTSTATUS vivsysDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  irpStack = NULL;
    PVOID               ioBuffer = NULL;
    ULONG               inLength = 0;
    ULONG               outLength = 0;
    ULONG               ioctl = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get the pointer to the input/output buffer and it's length
    //
    ioBuffer = Irp->AssociatedIrp.SystemBuffer;
    inLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (irpStack->MajorFunction)
    {

    case IRP_MJ_DEVICE_CONTROL:
        ioctl = irpStack->Parameters.DeviceIoControl.IoControlCode;
        DbgPrint("ioctl = 0x%x; looking for 0x%x\n", ioctl, READ_KMEM);
        switch (ioctl)
        {
        case READ_KMEM:
        {
            if (!ioBuffer)
            {
                break;
            }
            DbgPrint("vivsys.sys: Got read_kmem request\n");
            DbgPrint("ioBuffer =%s", ioBuffer);
            break;
        }
        }
        break;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
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
