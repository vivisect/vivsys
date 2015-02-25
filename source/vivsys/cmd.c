#include "cmd.h"

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
    __except (EXCEPTION_EXECUTE_HANDLER)
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

NTSTATUS doWriteKernMem(PVOID ioBuffer, ULONG inLength)
{
    NTSTATUS nts = STATUS_UNSUCCESSFUL;
    PMDL mdl = NULL;
    PVOID kAddr = NULL;
    ULONG NumBytes = 0;
    PVOID pSysBuff = NULL;

    if (NULL == ioBuffer ||
        sizeof(CMD_WRITE_KMEM) >= inLength ||
        0 == ((CMD_WRITE_KMEM*)ioBuffer)->NumBytes)
    {
        nts = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

#ifndef _WIN64
    kAddr = (PVOID)LOWDWORD(((CMD_WRITE_KMEM*)ioBuffer)->BaseAddr);
#else
    kAddr = (PVOID)((CMD_WRITE_KMEM*)ioBuffer)->BaseAddr;
#endif

    NumBytes = ((CMD_WRITE_KMEM*)ioBuffer)->NumBytes;

    if (kAddr <= MmHighestUserAddress)
    {
        KdPrint(("Tried to Write to a user-mode addr: 0x%p\n", kAddr));
        nts = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    __try
    {
        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            mdl = IoAllocateMdl(kAddr, NumBytes, FALSE, FALSE, NULL);
            if (NULL == mdl)
            {
                nts = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            MmProbeAndLockPages(mdl, KernelMode, IoWriteAccess);
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

        RtlCopyMemory(pSysBuff, ((CMD_WRITE_KMEM*)ioBuffer)->Data, NumBytes);
        nts = STATUS_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        nts = GetExceptionCode();
        KdPrint(("Exception writing to address 0x%p: 0x%x", kAddr, nts));
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

NTSTATUS doPoolAlloc(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength)
{
    NTSTATUS nts = STATUS_UNSUCCESSFUL;
    POOL_TYPE pt = 0;
    ULONG AllocSize = 0;
    PVOID pBuffer = NULL;

    if (sizeof(CMD_ALLOC_POOL) != inLength ||
        NULL == ioBuffer ||
        0 == ((CMD_ALLOC_POOL*)ioBuffer)->NumBytes ||
        sizeof(ULONGLONG) != outLength)
    {
        nts = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    pt = ((CMD_ALLOC_POOL*)ioBuffer)->PoolType;
    AllocSize = ((CMD_ALLOC_POOL*)ioBuffer)->NumBytes;

    pBuffer = ExAllocatePool(pt, AllocSize);
    if (NULL == pBuffer)
    {
        nts = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    RtlZeroMemory(pBuffer, AllocSize);

    *(ULONGLONG*)ioBuffer = (ULONGLONG)pBuffer;
    Irp->IoStatus.Information = sizeof(ULONGLONG);
    nts = STATUS_SUCCESS;

cleanup:
    return nts;
}

NTSTATUS doPoolFree(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength)
{
    NTSTATUS nts = STATUS_UNSUCCESSFUL;
    ULONG_PTR pAddr = 0;

    UNREFERENCED_PARAMETER(Irp);

    if (sizeof(CMD_FREE_POOL) != inLength ||
        0 != outLength || 
        NULL == ioBuffer || 
        0 == ((CMD_FREE_POOL*)ioBuffer)->pAddress)
    {
        nts = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    pAddr = (ULONG_PTR)((CMD_FREE_POOL*)ioBuffer)->pAddress;

    ExFreePool((PVOID)pAddr);
    pAddr = 0;
    nts = STATUS_SUCCESS;

cleanup:
    return nts;
}