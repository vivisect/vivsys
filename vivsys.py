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
SERVICE_START = 0x0010
SERVICE_STOP  = 0x0020
DELETE = 0x00010000
SERVICE_KERNEL_DRIVER = 0x00000001
SERVICE_FILE_SYSTEM_DRIVER = 0x00000002
SERVICE_DEMAND_START = 0x00000003
SERVICE_ERROR_IGNORE = 0x00000000
ERROR_SERVICE_ALREADY_RUNNING = 1056
SERVICE_CONTROL_STOP = 0x00000001

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
            outBuf = ctypes.byref((ctypes.c_ubyte * outSize)())
            
        #TODO: support overlapped i/o
        if not windll.kernel32.DeviceIoControl(self.hDev,
                                               code,
                                               inBuf,
                                               inLen,
                                               outBuf,
                                               outSize,
                                               ctypes.byref(retBytes),
                                               None):

            raise ctypes.WinError()
        
        if outSize:
            return buffer(x)[:]
        
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
        self.hScm = None
        self.hSvc = None

    def _cleanupHandles(self):
        if self.hSvc:
             windll.Advapi32.CloseServiceHandle(self.hSvc)
             self.hSvc = None
        if self.hScm:
             windll.Advapi32.CloseServiceHandle(self.hScm)
             self.hScm = None

    def load(self):
        print('Loading {}'.format(self.path))
        # Avoid handle leaks
        self._cleanupHandles()

        try:
            self.hScm = windll.Advapi32.OpenSCManagerW(None, None, SC_MANAGER_CREATE_SERVICE)
            if not self.hScm:
                raise ctypes.WinError()

            # See if the service already exists
            self.hSvc =  windll.Advapi32.OpenServiceW(self.hScm,
                                                  self.name,
                                                  SERVICE_START | DELETE | SERVICE_STOP)

            # Delete a pre-existing service
            if self.hSvc and not windll.Advapi32.DeleteService(self.hSvc):
                raise ctypes.WinError()

            if not self.hSvc:
                self.hSvc = windll.Advapi32.CreateServiceW(self.hScm,
                                                           self.name,
                                                           None,
                                                           SERVICE_START | DELETE | SERVICE_STOP,
                                                           SERVICE_KERNEL_DRIVER,
                                                           SERVICE_DEMAND_START,
                                                           SERVICE_ERROR_IGNORE,
                                                           self.path,
                                                           None, None, None, None, None)
            if not self.hSvc:
                raise ctypes.WinError()
        
            if not windll.Advapi32.StartServiceW(self.hSvc, 0, None):
                err = ctypes.WinError()
                if ERROR_SERVICE_ALREADY_RUNNING != err.winerror:
                    raise err
        except:
            self._cleanupHandles()
            raise

    def unload(self):
        print('Unloading {}'.format(self.path))
        
        # Need a service status struct for the ctlsvc call
        service_status = (wintypes.DWORD * 7)()

        try:
            if not self.hSvc:
            # See if the service already exists
                self.hSvc =  windll.Advapi32.OpenServiceW(self.hScm,
                                                          self.name,
                                                          DELETE | SERVICE_STOP)
            if not self.hSvc:
                raise ctypes.WinError()

            if not windll.Advapi32.ControlService(self.hSvc,
                                                  SERVICE_CONTROL_STOP,
                                                  ctypes.byref(service_status)):
                raise ctypes.WinError()

            if not windll.Advapi32.DeleteService(self.hSvc):
                raise ctypes.WinError()
        finally:
            self._cleanupHandles()

def makeCtlCode(func, dev_type=FILE_DEVICE_UNKNOWN, method=0, access=0):
    return (
    0xFFFFFFFF &
    ((dev_type) << 16) | 
    ((access) << 14) | 
    ((func) << 2) | (method)
    )

drv = Driver('vivsys', 'C:\\vivsys\\vivsys.sys')
drv.load()
ctl_code = makeCtlCode(0x800)
try:
    dev = Device('vivsys')
    dev.ioctl(ctl_code, bytes([0x41, 0x41, 0x41, 0x41, 0x0a]))
finally:
    dev.close()
    drv.unload()




