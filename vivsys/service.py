import ctypes

from vivsys.common import *

class ServiceControlManager:

    def __init__(self):
        self.hScm = None

    def __enter__(self):
        self.hScm = advapi32.OpenSCManagerW(None, None, SC_MANAGER_CREATE_SERVICE)
        if not self.hScm:
            raise ctypes.WinError()
        return self

    def __exit__(self, exc, ex, tb):
        advapi32.CloseServiceHandle(self.hScm)
        self.hScm = None

    def _req_with(self):
        if self.hScm == None:
            raise Exception('ServiceControlManager not in with block!')

    def openService(self, name, access=SERVICE_ALL_ACCESS):
        '''
        Retrieve a Service object for the given service name.

        Example:
            with ServiceControlManager() as scm:
                with scm.openService('woot') as svc:
                    dostuff(svc)
        '''
        self._req_with()
        hSvc =  advapi32.OpenServiceW(self.hScm, name, access)
        if not hSvc:
            raise ctypes.WinError()

        return Service(self.hScm, hSvc)

    def createDriverService(self, path, name):
        '''
        Helper method for creation of driver services.
        '''
        self._req_with()
        hSvc = advapi32.CreateServiceW(self.hScm,
                                       name,
                                       None,
                                       SERVICE_START | DELETE | SERVICE_STOP,
                                       SERVICE_KERNEL_DRIVER,
                                       SERVICE_DEMAND_START,
                                       SERVICE_ERROR_IGNORE,
                                       path,
                                       None, NULL, None, None, None)
        if not hSvc:
            raise ctypes.WinError()

        return Service(self.hScm,hSvc)

    def isServiceName(self, name):
        '''
        Return True/False if a service (by name) exists.
        '''
        self._req_with()

        retval = False
        hSvc =  advapi32.OpenServiceW(self.hScm, name, SERVICE_ALL_ACCESS)
        if hSvc:
            retval = True
            advapi32.CloseServiceHandle(hSvc)

        return retval

class Service:
    '''
    An object to minimally wrap the Windows Service APIs
    which are needed for loading/unloading drivers.
    '''
    def __init__(self, hScm, hSvc):
        self.hScm = hScm
        self.hSvc = hSvc
        self.inwith = False

    def __enter__(self):
        self.inwith = True
        return self

    def __exit__(self, exc, ex, tb):
        self.close()

    def close(self):
        advapi32.CloseServiceHandle(self.hSvc)
        self.hSvc = None

    def _req_with(self):
        if not self.inwith:
            raise Exception('Service not in with block!')

    def getServiceStatus(self):
        '''
        Returns a SERVICE_STATUS structure for the service.
        '''
        self._req_with()
        status = SERVICE_STATUS()
        if not advapi32.QueryServiceStatus(self.hSvc, ctypes.byref(status)):
            raise ctypes.WinError()
        return status

    def delService(self):
        '''
        Delete the service.

        Example:
            scm = ServiceControlManager()
            with scm:
                with scm.openService('woot') as svc:
                    svc.delService()
        '''
        self._req_with()
        if not advapi32.DeleteService(self.hSvc):
            err = ctypes.WinError()
            if ERROR_SERVICE_MARKED_FOR_DELETE != err.winerror:
                raise err

    def stopService(self):
        '''
        Stop the service ( if running ).
        '''
        self._req_with()
        status = self.getServiceStatus()
        if status.dwCurrentState == SERVICE_RUNNING:
            if not advapi32.ControlService(self.hSvc, SERVICE_CONTROL_STOP, ctypes.byref(status)):
                raise ctypes.WinError()
        return status

    def startService(self):
        '''
        Start the service.
        '''
        self._req_with()
        if not advapi32.StartServiceW(self.hSvc, 0, NULL):
            err = ctypes.WinError()
            if ERROR_SERVICE_ALREADY_RUNNING != err.winerror:
                raise err
