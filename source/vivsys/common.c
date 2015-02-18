#include "common.h"

ULONGLONG fnv1_hash_string(VOID *key, ULONG n_bytes, BOOLEAN bNormalize)
{
	UCHAR *p = key;
	BYTE cTmp = 0;
	ULONG i = 0;
	ULONGLONG h = 14695981039346656037;

	for (i = 0; i < n_bytes; i++)
	{
		cTmp = p[i];
		if (bNormalize)
		{
			cTmp = (BYTE)tolower(cTmp);
		}
		h = (h * 1099511628211) ^ cTmp;
	}
	return h;
}

NTSTATUS GetModuleBase(ULONGLONG ullModHash, PVOID* ppBaseAddress)
{
	NTSTATUS nts = STATUS_UNSUCCESSFUL;
	ULONG ulLength = 0;
	PSYSTEM_MODULE_INFORMATION  pModuleInfo = NULL;
    SYSTEM_MODULE **pModule = NULL;
	ULONGLONG ullTmpHash = 0;
	ULONG ulNameOffset = 0;
	ULONG ulNameLen = 0;
	PCHAR pModName = NULL;

	if (NULL == ppBaseAddress ||
		0 == ullModHash)
	{
		nts = STATUS_INVALID_PARAMETER;
		goto cleanup;
	}

	__try
	{
		*ppBaseAddress = NULL;
        nts = GetSystemModuleArray(&pModuleInfo, &ulLength);

		if (NT_SUCCESS(nts))
		{
			nts = STATUS_OBJECT_NAME_NOT_FOUND;

            ulLength = (ULONG)pModuleInfo->ulModuleCount;
            pModule  = (PSYSTEM_MODULE*)&pModuleInfo->Modules;

			while (ulLength--)
			{
                DbgPrint("ulLength = %d\n", ulLength);
                pModName = pModule[ulLength]->ImageName;
                ulNameOffset = pModule[ulLength]->ModuleNameOffset;
                DbgPrint("ulNameOffset: 0x%x\n", ulNameOffset);
				if (S_OK != ULongSub(MAX_SYS_MOD_NAME, ulNameOffset, &ulNameLen))
				{
					nts = STATUS_INTEGER_OVERFLOW;
					goto cleanup;
				}

				pModName = pModName + ulNameOffset;

				ulNameLen = (ULONG)strnlen(pModName, ulNameLen);

				ullTmpHash = fnv1_hash_string(pModName, ulNameLen, TRUE);
				if (ullTmpHash == ullModHash)
				{
                    *ppBaseAddress = pModule[ulLength]->Base;
					nts = STATUS_SUCCESS;
					goto cleanup;
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		nts = GetExceptionCode();
		KdPrint(("GetModuleBase: Caught exception: 0x%x\n", nts));
	}

cleanup:
    if (pModuleInfo)
	{
        ExFreePool(pModuleInfo);
        pModuleInfo = NULL;
	}

	return nts;
}

NTSTATUS GetSystemModuleArray(PSYSTEM_MODULE_INFORMATION *pModuleInfo, ULONG *ulBufferSize)
{
	PSYSTEM_MODULE_INFORMATION  pTmpModList = NULL;
	NTSTATUS nts = STATUS_UNSUCCESSFUL;
	ULONG ulLength = 0;

    if (NULL == pModuleInfo || NULL == ulBufferSize)
    {
        nts = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

	nts = ZwQuerySystemInformation(SystemModuleInformation,
		                           &ulLength,
		                           0,
		                           &ulLength);

	if (nts != STATUS_INFO_LENGTH_MISMATCH)
	{
		KdPrint(("GetModuleBase: ZwQuerySystemInformation() failed with error: 0x%x\n", nts));
		goto cleanup;
	}

    pTmpModList = ExAllocatePool(PagedPool, ulLength);
    if (NULL == pTmpModList)
	{
		KdPrint(("GetModuleBase: Failed to allocate Paged Pool memory."));
		nts = STATUS_NO_MEMORY;
		goto cleanup;
	}

	nts = ZwQuerySystemInformation(SystemModuleInformation,
                                   pTmpModList,
		                           ulLength,
		                           &ulLength);

    *ulBufferSize = ulLength;
    *pModuleInfo = pTmpModList;

cleanup:
    if (!NT_SUCCESS(nts))
    {
        ExFreePool(pTmpModList);
        pTmpModList = NULL;
    }

    return nts;
}