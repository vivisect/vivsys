import unittest

import vivsys.device as v_sys_device
import vivsys.kernapi as v_sys_kernapi

from vivsys.common import *

class TestReadMemory(unittest.TestCase):

    def test_get_kernbase(self):
        self.v = v_sys_kernapi.VivSys()
        baseAddr = self.v.getKmodBase('ntoskrnl.exe')
        mz_hdr = self.v.readMemory(baseAddr, 2)
        self.assertEqual(mz_hdr.decode("utf-8"), 'MZ')

    def tearDown(self):
        self.v.driver.unload()
