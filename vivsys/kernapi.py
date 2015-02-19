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

class CmdReadIoPort(ctypes.Structure):
    _pack_ = 1
    _fields_ = (
        ("port", ctypes.c_uint32),
        ("count", ctypes.c_uint32)
    )

READ_KMEM = CTL_CODE(0x800)
GET_KMOD_LIST  = CTL_CODE(0x801)
WRITE_IO_PORT  = CTL_CODE(0x802)
READ_IO_PORT  = CTL_CODE(0x803)

class VivSys:

    def __init__(self):
        self.driver = v_sys_driver.Driver(driverpath, name='vivsys')
        # FIXME detect already loaded.
        self.driver.load()
        self.dev = v_sys_device.Device('vivsys')

    def readMemory(self, addr, size):
        cmd_read = CmdReadKmem()
        cmd_read.baseAddr = addr
        cmd_read.bytesToRead = size

        buf = ctypes.create_string_buffer(size)

        retval, retsize = self.dev.ioctl(READ_KMEM, inStruct=cmd_read, outStruct=buf)
        if not retval:
            raise ctypes.WinError()

        return buf.raw

    # TODO: make a ctypes struct def for this
    def writeIoPort(self, port, data):
        p = int(port).to_bytes(ctypes.sizeof(DWORD),  byteorder='little')
        count = len(data).to_bytes(ctypes.sizeof(DWORD),  byteorder='little')
        write_port = p + count + bytes(data)

        in_buf = ctypes.create_string_buffer(len(write_port))
        in_buf.value = write_port
        
        retval, retsize = self.dev.ioctl(WRITE_IO_PORT, inStruct=in_buf)
        if not retval:
            raise ctypes.WinError()

    def readIoPort(self, port, count):
        cmd_read = CmdReadIoPort()
        cmd_read.port = port
        cmd_read.count = count

        buf = ctypes.create_string_buffer(count)
        
        retval, retsize = self.dev.ioctl(READ_IO_PORT, inStruct=cmd_read, outStruct=buf)
        if not retval:
            raise ctypes.WinError()

        return buf.raw

    def getKmodList(self):
        
        mod_list = []
        curr_size = PAGE_SIZE * 2
        
        # If this buff is larger than 0x100 pages, something is wrong 
        while curr_size  < (PAGE_SIZE * 0x100):
            kmod_size = ctypes.c_uint32()

            buf = ctypes.create_string_buffer(curr_size)
            # Now get the actual data
            retval, retsize = self.dev.ioctl(GET_KMOD_LIST, inStruct=None, outStruct=buf)
        
            if not retval:
                if ERROR_BAD_LENGTH == ctypes.GetLastError():
                    curr_size *= 2 
                    continue
                raise ctypes.WinError()
            break
            
            curr_size *= 2 
        
        # If we get here, the sysmod buffer is insanely huge, 
        if not retsize:
            raise Exception('Maximum system module buffer size exceeded')
 
        #FIXME: Change this when we port the wow64 stuff over
        ptr_size = int.from_bytes(buf.raw[:4], byteorder='little')
        if ptr_size == ctypes.sizeof(DWORD): 
            sys_mod = SYSTEM_MODULE32
        elif ptr_size == ctypes.sizeof(QWORD):
            sys_mod = SYSTEM_MODULE64
        else:
            raise Exception('Invalid ptr size of sys module list')

        mod_count = int.from_bytes(buf.raw[4:8], byteorder='little')

        data = buf.raw[ctypes.sizeof(DWORD) + ptr_size:]

        sys_buf = ctypes.cast(data, ctypes.POINTER(sys_mod))
        for i in range(mod_count):
            path = bytes(sys_buf[i].ImageName)
            path = path[:path.find(0)].decode('utf-8')
            
            # Normalize pathname to remove the DOS prefix and expand the sysroot
            if path.startswith('\\??\\'):
                path = path.lstrip('\\??\\')
            if path.startswith('\\SystemRoot\\'):
                path = os.path.join(os.getenv('SystemRoot'), '',  path.replace('\\SystemRoot\\', ''))

            mod_list.append((path, sys_buf[i].Base, sys_buf[i].Size, sys_buf[i].LoadCount))

        return mod_list


    
