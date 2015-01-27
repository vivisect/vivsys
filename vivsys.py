from tools import *
import ctypes

class CmdReadKmem(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("baseAddr", ctypes.c_uint64),
                ("bytesToRead", ctypes.c_uint32)]

# TODO
class Mem(object):
    def __init__(self, baseAddr=None):
        self.baseAddr = baseAddr
    def read(num_bytes):
        raise NotImplementedError("read")
    def write(data):
        raise NotImplementedError("write")
    def seek(offset):
        raise NotImplementedError("seek")

class KMem(Mem):
    def __init__(self, dev, baseAddr=None):
        Mem.__init__(self, baseAddr)
        self.dev = dev
# TODO Add slicing
    def read(self, addr, numBytes):
        kmemRead = 0x800

        ctl_code = makeCtlCode(kmemRead)
        cmdRead = CmdReadKmem()
        cmdRead.baseAddr = addr
        cmdRead.bytesToRead = numBytes
        cmdRead = memoryview(cmdRead).tobytes()

        return self.dev.ioctl(ctl_code, cmdRead, outSize=numBytes)

class VivSys(Driver):
   def __init__(self):
       Driver.__init__(self, 'vivsys', 'C:\\vivsys\\vivsys.sys') #TODO: remove this path
       self.load()
       self.dev = Device('vivsys')

   def getKMem(self, baseAddr=None):
       return KMem(self.dev, baseAddr)

# Test code
try:
    v = VivSys()
    kmem = v.getKMem()
    print(kmem.read(0x82a02000, 4))
finally:
    v.unload()

