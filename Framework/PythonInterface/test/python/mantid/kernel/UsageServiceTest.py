import unittest

from mantid.kernel import (UsageService, UsageServiceImpl)

class UsageServiceTest(unittest.TestCase):

    def test_singleton_returns_instance_of_UsageService(self):
        self.assertTrue(isinstance(UsageService, UsageServiceImpl))

    def test_getSetEnabled(self):
        UsageService.setEnabled(False)
        self.assertEquals(UsageService.isEnabled(),False)
        UsageService.setEnabled(True)
        self.assertEquals(UsageService.isEnabled(),True)
        UsageService.setEnabled(False)
        self.assertEquals(UsageService.isEnabled(),False)

    def test_getSetApplication(self):
        self.assertEquals(UsageService.getApplication(),"python")
        UsageService.setApplication("python unit tests")
        self.assertEquals(UsageService.getApplication(),"python unit tests")
        UsageService.setApplication("python")
        self.assertEquals(UsageService.getApplication(),"python")

    def test_setInterval(self):
        UsageService.setEnabled(False)
        UsageService.setInterval(60)

    def test_registerStartup(self):
        UsageService.setEnabled(False)
        #this will do nothing as it is disabled
        UsageService.registerStartup()

    def test_registerFeatureUsage(self):
        UsageService.setEnabled(False)
        #this will do nothing as it is disabled
        UsageService.registerFeatureUsage("Algorithm","Test.v1",True)


    def test_Flush(self):
        UsageService.setEnabled(False)
        #this will do nothing as it is disabled
        UsageService.flush()

    def test_Shutdown(self):
        UsageService.shutdown()

if __name__ == '__main__':
    unittest.main()
