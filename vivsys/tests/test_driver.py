import os
import unittest

import vivsys.driver as v_sys_driver
import vivsys.device as v_sys_device

class TestDriver(unittest.TestCase):

    def test_loadunload(self):
        driver = v_sys_driver.Driver(r'c:\vivsys.sys')
        driver.load()
        device = v_sys_device.Device(r'vivsys')
        device.close()
        driver.unload()
