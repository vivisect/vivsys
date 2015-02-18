#include <ntddk.h>
#include <ntddstor.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <intsafe.h>
#include <Windef.h>

#define MAX_SYS_MOD_NAME 256 

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemModuleInformation = 11
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_MODULE   // Information Class 11
{
	ULONG_PTR Reserved[2];
	PVOID Base;
	ULONG Size;
	ULONG Flags;
	USHORT Index;
	USHORT Unknown;
	USHORT LoadCount;
	USHORT ModuleNameOffset;
	CHAR ImageName[256];
} SYSTEM_MODULE, *PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION   // Information Class 11
{
    ULONG_PTR ulModuleCount;
    SYSTEM_MODULE Modules[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

NTSTATUS
NTAPI
ZwQuerySystemInformation(__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
__inout PVOID SystemInformation,
__in ULONG SystemInformationLength,
__out PULONG ReturnLength OPTIONAL);

ULONGLONG fnv1_hash_string(VOID *key, ULONG n_bytes, BOOLEAN bNormalize);
NTSTATUS GetSystemModuleArray(PSYSTEM_MODULE_INFORMATION *pModuleInfo, ULONG *ulBufferSize);