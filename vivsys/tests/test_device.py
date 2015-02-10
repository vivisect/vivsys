import unittest

import vivsys.device as v_sys_device

from vivsys.common import *

class TestDevice(unittest.TestCase):

    def test_device_phys0(self):
        IOCTL_DISK_BASE = 0x7
        IOCTL_DISK_IS_WRITABLE = CTL_CODE(0x9,devtype=IOCTL_DISK_BASE)

        with v_sys_device.Device(r'PhysicalDrive0') as dev:
            self.assertTrue( dev.ioctl( IOCTL_DISK_IS_WRITABLE )[0] )
