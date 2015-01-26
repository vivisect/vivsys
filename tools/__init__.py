import os
import ctypes
from ctypes import windll
from ctypes import wintypes

GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3

FILE_DEVICE_UNKNOWN = 0x22
METHOD_BUFFERED = 0
FILE_ANY_ACCESS = 0
SC_MANAGER_CREATE_SERVICE = 0x0002

DELETE = 0x00010000
SERVICE_START = 0x0010
SERVICE_STOP  = 0x0020
SERVICE_QUERY_STATUS = 0x0004

SERVICE_RUNNING = 0x00000004
SERVICE_KERNEL_DRIVER = 0x00000001
SERVICE_FILE_SYSTEM_DRIVER = 0x00000002
SERVICE_DEMAND_START = 0x00000003
SERVICE_ERROR_IGNORE = 0x00000000
SERVICE_CONTROL_STOP = 0x00000001

ERROR_SERVICE_ALREADY_RUNNING = 1056
ERROR_SERVICE_MARKED_FOR_DELETE = 1072

class Device(object):
    def __init__(self, dev_name, access=GENERIC_READ | GENERIC_WRITE, share=0, flags=0):

        self.hDev = windll.kernel32.CreateFileW('\\\\.\\' + dev_name,
                                           access,
                                           share,
                                           None,
                                           OPEN_EXISTING,
                                           flags,
                                           None)

        if -1 == self.hDev:
            self.hDev = None
            raise ctypes.WinError()

    def close(self):
        if self.hDev:
            windll.kernel32.CloseHandle(self.hDev)
            self.hDev = None

    def ioctl(self, code, inBuf=None, outSize=0):
        inLen = 0
        outBuf = None
        retBytes = ctypes.c_ulong()
        
        if inBuf:
            inLen = len(inBuf)
        if outSize:
            outBuf = (ctypes.c_ubyte * outSize)()
            
        #TODO: support overlapped i/o
        if not windll.kernel32.DeviceIoControl(self.hDev,
                                               code,
                                               inBuf,
                                               inLen,
                                               ctypes.byref(outBuf),
                                               outSize,
                                               ctypes.byref(retBytes),
                                               None):

            raise ctypes.WinError()
         
        if outSize:
            return bytes(outBuf[:])
        
    def read(self):
        # TODO
        pass

    def write(self):
        # TODO
        pass

class Driver(object):
    def __init__(self, svc_name, driver_path):
        self.name = svc_name
        self.path = driver_path

    def _doesServiceExist(self):
        rv = False

        try:
            hScm = windll.Advapi32.OpenSCManagerW(None, None, SC_MANAGER_CREATE_SERVICE)
            if not hScm:
                raise ctypes.WinError()
            # See if the service already exists
            hSvc =  windll.Advapi32.OpenServiceW(hScm,
                                                 self.name,
                                                 SERVICE_START | DELETE | SERVICE_STOP)
            if hSvc:
                rv = True
        finally:
            if hScm:
                windll.Advapi32.CloseServiceHandle(hScm)
            if hSvc:
                windll.Advapi32.CloseServiceHandle(hSvc)
        return rv

    def _removeService(self):
        hScm = None
        hSvc = None
        # Need a service status struct for the ctlsvc call
        service_status = (wintypes.DWORD * 7)()

        try:
            hScm = windll.Advapi32.OpenSCManagerW(None, None, SC_MANAGER_CREATE_SERVICE)
            if not hScm:
                raise ctypes.WinError()
            
            hSvc =  windll.Advapi32.OpenServiceW(hScm,
                                                 self.name,
                                                 SERVICE_QUERY_STATUS | DELETE | SERVICE_STOP)
            if not hSvc:
                raise ctypes.WinError()

            if not windll.Advapi32.QueryServiceStatus(hSvc,
                                                      ctypes.byref(service_status)):
                raise ctypes.WinError()

            if SERVICE_RUNNING == service_status[1]:
                if not windll.Advapi32.ControlService(hSvc,
                                                      SERVICE_CONTROL_STOP,
                                                      ctypes.byref(service_status)):
                    raise ctypes.WinError()

            if not windll.Advapi32.DeleteService(hSvc):
                err = ctypes.WinError()
                if ERROR_SERVICE_MARKED_FOR_DELETE != err.winerror:
                    raise err
        finally:
            if hScm:
                windll.Advapi32.CloseServiceHandle(hScm)
            if hSvc:
                windll.Advapi32.CloseServiceHandle(hSvc)


    def load(self):
        print('Loading {}'.format(self.path))
        # Delete a pre-existing service
        if self._doesServiceExist():
            self._removeService()

        try:
            hScm = windll.Advapi32.OpenSCManagerW(None, None, SC_MANAGER_CREATE_SERVICE)
            if not hScm:
                raise ctypes.WinError()
  
            hSvc = windll.Advapi32.CreateServiceW(hScm,
                                                  self.name,
                                                  None,
                                                  SERVICE_START | DELETE | SERVICE_STOP,
                                                  SERVICE_KERNEL_DRIVER,
                                                  SERVICE_DEMAND_START,
                                                  SERVICE_ERROR_IGNORE,
                                                  self.path,
                                                  None, None, None, None, None)
            if not hSvc:
                raise ctypes.WinError()
        
            if not windll.Advapi32.StartServiceW(hSvc, 0, None):
                err = ctypes.WinError()
                if ERROR_SERVICE_ALREADY_RUNNING != err.winerror:
                    raise err
        finally:
            if hScm:
                windll.Advapi32.CloseServiceHandle(hScm)
            if hSvc:
                windll.Advapi32.CloseServiceHandle(hSvc)

    def unload(self):
        print('Unloading {}'.format(self.path))
        self._removeService()

def makeCtlCode(func, dev_type=FILE_DEVICE_UNKNOWN, method=0, access=0):
    return (
    0xFFFFFFFF &
    ((dev_type) << 16) | 
    ((access) << 14) | 
    ((func) << 2) | (method)
    )
