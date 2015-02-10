import unittest
import vivsys.service as v_sys_svc
from vivsys.common import *

class ServiceTest(unittest.TestCase):

    def test_service_is(self):
        scm = v_sys_svc.ServiceControlManager()
        with scm:
            self.assertTrue( scm.isServiceName('LanmanWorkstation') )

    def test_service_isnot(self):
        scm = v_sys_svc.ServiceControlManager()
        with scm:
            self.assertFalse( scm.isServiceName('TotallyFakeService') )

    def test_service_status(self):
        scm = v_sys_svc.ServiceControlManager()
        with scm:
            with scm.openService('LanmanWorkstation') as svc:
                status = svc.getServiceStatus()
                self.assertEqual( status.dwCurrentState, SERVICE_RUNNING )
