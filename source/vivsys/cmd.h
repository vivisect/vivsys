#include "common.h"

// IOCTL defs
#define READ_KMEM CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                            0x800,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define GET_KMOD CTL_CODE(FILE_DEVICE_UNKNOWN,    \
                            0x801,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define WRITE_PORT CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                            0x802,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define READ_PORT CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                            0x803,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define ALLOC_POOL CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                            0x804,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

#define FREE_POOL CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                            0x805,                \
                            METHOD_BUFFERED,      \
                            FILE_ANY_ACCESS)

// cmd struct defs
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

typedef struct _CMD_ALLOC_POOL
{
    ULONG PoolType;
    ULONG NumBytes;
}CMD_ALLOC_POOL;

typedef struct _CMD_FREE_POOL
{
    ULONGLONG pAddress;
}CMD_FREE_POOL;
#pragma pack(pop)

// Func defs
NTSTATUS doReadIoPort(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength);
NTSTATUS doWriteIoPort(PVOID ioBuffer, ULONG inLength);
NTSTATUS getKernModList(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength);
NTSTATUS doReadKernMem(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength);
NTSTATUS doPoolAlloc(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength);
NTSTATUS doPoolFree(PIRP Irp, PVOID ioBuffer, ULONG inLength, ULONG outLength);