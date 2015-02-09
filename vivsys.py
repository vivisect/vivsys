from tools import *
from hashlib import md5
from binascii import hexlify
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

        ctl_code = CTL_CODE(0x800)
        cmdRead = CmdReadKmem()
        cmdRead.baseAddr = addr
        cmdRead.bytesToRead = numBytes
        cmdRead = memoryview(cmdRead).tobytes()

        return self.dev.ioctl(ctl_code, cmdRead, outSize=numBytes)

class VivSys(Driver):
   def __init__(self):
       bin_path = 'C:\\vivsys\\vivsys.sys' #TODO: remove
       svc_name = None

       with open(bin_path, 'rb') as f:
           m = md5()
           m.update(f.read())
           svc_name = hexlify(m.digest())
           svc_name = str(svc_name, 'ascii')

       Driver.__init__(self, svc_name, bin_path) 
       self.load()
       self.dev = Device('vivsys')

   def getKMem(self, baseAddr=None):
       return KMem(self.dev, baseAddr)

# Test code
v = None
try:
    v = VivSys()
    kmem = v.getKMem()
    #print(kmem.read(0xFFFFF80002A9C970, 4))
finally:
    pass
    if v:
        v.unload()
    
