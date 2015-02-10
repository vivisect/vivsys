import os
import ctypes
from ctypes import windll
from ctypes import wintypes

#TODO: merge this with vivisect's ctypes helpers
LPVOID  = ctypes.c_void_p
HANDLE  = LPVOID
SIZE_T  = LPVOID
QWORD   = ctypes.c_ulonglong
DWORD   = ctypes.c_ulong
WORD    = ctypes.c_ushort
BOOL    = ctypes.c_ulong
BYTE    = ctypes.c_ubyte
LPWSTR  = ctypes.c_wchar_p
NULL    = 0

#Setup the kernel32 APIs
kernel32 = windll.kernel32
kernel32.CreateFileW.restype = HANDLE
kernel32.CreateFileW.argtypes = [LPWSTR, DWORD, DWORD, LPVOID, DWORD, DWORD, HANDLE]
kernel32.CloseHandle.restype = BOOL
kernel32.CloseHandle.argtypes = [HANDLE]
kernel32.DeviceIoControl.restype = BOOL
kernel32.DeviceIoControl.argtypes = [HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPVOID, LPVOID]
kernel32.GetNativeSystemInfo.restype = None
kernel32.GetNativeSystemInfo.argtypes = [LPVOID]

advapi32 = windll.advapi32
advapi32.OpenSCManagerW.restype = HANDLE
advapi32.OpenSCManagerW.argtypes = [LPWSTR, LPWSTR, DWORD]
advapi32.CloseServiceHandle.restype = BOOL 
advapi32.CloseServiceHandle.argtypes = [HANDLE]
advapi32.OpenServiceW.restype = HANDLE
advapi32.OpenServiceW.argtypes = [HANDLE, LPWSTR, DWORD]
advapi32.QueryServiceStatus.restype = BOOL
advapi32.QueryServiceStatus.argtypes = [HANDLE, LPVOID]
advapi32.ControlService.restype = BOOL
advapi32.ControlService.argtypes = [HANDLE, DWORD, LPVOID]
advapi32.DeleteService.restype = BOOL
advapi32.DeleteService.argtypes = [HANDLE]
advapi32.CreateServiceW.restype = HANDLE
advapi32.CreateServiceW.argtypes = [HANDLE, LPWSTR, LPWSTR, DWORD, DWORD, DWORD, DWORD, LPWSTR, LPWSTR, LPVOID, LPWSTR, LPWSTR, LPWSTR]
advapi32.StartServiceW.restype = BOOL
advapi32.StartServiceW.argtypes = [HANDLE, DWORD, LPVOID]

GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
GENERIC_RW    = GENERIC_READ | GENERIC_WRITE
OPEN_EXISTING = 3

FILE_DEVICE_UNKNOWN = 0x22
METHOD_BUFFERED = 0
FILE_ANY_ACCESS = 0

SC_MANAGER_CREATE_SERVICE = 0x0002

DELETE                  = 0x00010000
SERVICE_START           = 0x0010
SERVICE_STOP            = 0x0020
SERVICE_QUERY_STATUS    = 0x0004

SERVICE_RUNNING             = 0x00000004
SERVICE_KERNEL_DRIVER       = 0x00000001
SERVICE_FILE_SYSTEM_DRIVER  = 0x00000002
SERVICE_DEMAND_START        = 0x00000003
SERVICE_ERROR_IGNORE        = 0x00000000
SERVICE_CONTROL_STOP        = 0x00000001

ERROR_SERVICE_ALREADY_RUNNING   = 1056
ERROR_SERVICE_MARKED_FOR_DELETE = 1072

SERVICE_ALL_ACCESS  = SERVICE_QUERY_STATUS | DELETE | SERVICE_STOP

class SERVICE_STATUS(ctypes.Structure):
    _fields_ = (
        ('dwServiceType',DWORD),
        ('dwCurrentState',DWORD),
        ('dwControlsAccepted',DWORD),
        ('dwWin32ExitCode',DWORD),
        ('dwServiceSpecificExitCode',DWORD),
        ('dwCheckPoint',DWORD),
        ('dwWaitHint',DWORD),
    )

class SYSTEM_INFO(ctypes.Structure):
    _fields_ = (
        ('wProcessorArchitecture',WORD),
        ('wReserved',WORD),
        ('dwPageSize',DWORD),
        ('lpMinimumApplicationAddress',LPVOID),
        ('lpMaximumApplicationAddress',LPVOID),
        ('dwActiveProcessorMask',LPVOID),
        ('dwNumberOfProcessors',DWORD),
        ('dwProcessorType',DWORD),
        ('dwAllocationGranularity',DWORD),
        ('wProcessorLevel',WORD),
        ('wProcessorRevision',WORD)
    )

def CTL_CODE(func, devtype=FILE_DEVICE_UNKNOWN, method=METHOD_BUFFERED, access=FILE_ANY_ACCESS):
    return (
        0xffffffff &
        ((devtype) << 16) | 
        ((access) << 14) | 
        ((func) << 2) | (method)
    )


