import ctypes

from vivsys.common import *

devbase = '\\\\.\\'

class Device(object):

    def __init__(self, dev_name, access=GENERIC_RW, share=0, flags=0):

        self.hDev = kernel32.CreateFileW(devbase + dev_name,
                                         access,
                                         share,
                                         NULL,
                                         OPEN_EXISTING,
                                         flags,
                                         NULL)

        if -1 == self.hDev:
            self.hDev = None
            raise ctypes.WinError()

    def __enter__(self):
        return self

    def __exit__(self, exc, ex, tb):
        self.close()

    def close(self):
        if self.hDev:
            kernel32.CloseHandle(self.hDev)
            self.hDev = None

    def ioctl(self, code, inStruct=None, outStruct=None):
        '''
        Issue a DeviceIoControl call for the device.

        Example:
            dev = Device('mydevice')
            outbuf = ctypes.create_string_buffer(1024)
            dev.ioctl(IOCTL_EXAMPLE_WITHBYTES,outStruct=outbuf)

        '''
        inLen = 0
        outLen = 0

        inRef = None
        outRef = None

        if inStruct != None:
            inRef = ctypes.byref(inStruct)
            inLen = ctypes.sizeof(inStruct)

        if outStruct != None:
            outRef = ctypes.byref(outStruct)
            outLen = ctypes.sizeof(outStruct)

        retBytes = ctypes.c_ulong()

        #TODO: support overlapped i/o
        retval = kernel32.DeviceIoControl(self.hDev,
                                        code,
                                        inRef,
                                        inLen,
                                        outRef,
                                        outLen,
                                        ctypes.byref(retBytes),
                                        NULL)

        return (retval,retBytes.value)
        
    def read(self):
        # TODO
        pass

    def write(self):
        # TODO
        pass

