import os
import ctypes
import platform

import vivsys.device as v_sys_device
import vivsys.driver as v_sys_driver
import vivsys.service as v_sys_service

from vivsys.common import *

sysinfo = SYSTEM_INFO()
kernel32.GetNativeSystemInfo(ctypes.byref(sysinfo))

archmap = {0:'i386',9:'amd64'}

arch = archmap.get( sysinfo.wProcessorArchitecture )
if arch == None:
    raise Exception('Unknown Windows Architecture: %d' % (sysinfo.wProcessorArchitecture,))

driverpath = os.path.join( os.path.dirname(__file__), 'vivsys_%s.sys' % (arch,))

class CmdReadKmem(ctypes.Structure):
    _pack_ = 1
    _fields_ = (
        ("baseAddr", ctypes.c_uint64),
        ("bytesToRead", ctypes.c_uint32)
    )

KMEM_READ   = CTL_CODE(0x800)

class VivSys:

    def __init__(self):
        self.driver = v_sys_driver.Driver(driverpath,name='vivsys')
        # FIXME detect already loaded.
        self.driver.load()
        self.dev = v_sys_device.Device('vivsys')

    def readMemory(self, addr, size):
        cmdRead = CmdReadKmem()
        cmdRead.baseAddr = addr
        cmdRead.bytesToRead = size

        buf = ctypes.create_string_buffer(size)

        retval,retsize = self.dev.ioctl(KMEM_READ, inStruct=cmdRead, outStruct=buf)
        if not retval:
            raise ctypes.WinError()

        return buf.raw

