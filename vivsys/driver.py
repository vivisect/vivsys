import os
import hashlib

from vivsys.common import *

import vivsys.service as v_sys_service

class Driver(object):

    def __init__(self, path, name=None):

        if not os.path.isfile(path):
            raise Exception('Invalid Driver Path: %s' % path)

        self.scm = v_sys_service.ServiceControlManager()
        
        if name == None:
            with open(path,'rb') as fd:
                name = hashlib.md5( fd.read() ).hexdigest() 

        self.name = name
        self.path = path

    def __enter__(self):
        self.load()
        return self

    def __exit__(self, exc, ex, tb):
        self.unload()

    def load(self):
        '''
        Load the driver.

        NOTE: if a driver is already loaded with the given
              service name, this API will stop/del the serivce.

        Example:
            drv = Driver( mypath )
            drv.load()
        '''
        with self.scm as scm:

            if scm.isServiceName(self.name):
                with scm.openService(self.name) as svc:
                    svc.stopService()
                    svc.delService()

            with scm.createDriverService(self.path, self.name) as svc:
                svc.startService()

    def unload(self):
        '''
        Unload the driver.

        NOTE: this will also completely remove the associated service.

        Example:
            drv = Driver( mypath )
            drv.unload()
        '''
        with self.scm as scm:
            with scm.openService(self.name) as svc:
                    svc.stopService()
                    svc.delService()


