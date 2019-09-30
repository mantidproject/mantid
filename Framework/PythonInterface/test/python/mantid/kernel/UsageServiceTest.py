# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest

from mantid.kernel import (UsageService, UsageServiceImpl)


class UsageServiceTest(unittest.TestCase):

    def test_singleton_returns_instance_of_UsageService(self):
        self.assertTrue(isinstance(UsageService, UsageServiceImpl))

    def test_getSetEnabled(self):
        UsageService.setEnabled(False)
        self.assertEqual(UsageService.isEnabled(),False)
        UsageService.setEnabled(True)
        self.assertEqual(UsageService.isEnabled(),True)
        UsageService.setEnabled(False)
        self.assertEqual(UsageService.isEnabled(),False)

    def test_getSetApplication(self):
        self.assertEqual(UsageService.getApplicationName(), "python")
        UsageService.setApplicationName("python unit tests")
        self.assertEqual(UsageService.getApplicationName(), "python unit tests")
        UsageService.setApplicationName("python")
        self.assertEqual(UsageService.getApplicationName(), "python")

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
        UsageService.registerFeatureUsage("Algorithm", "Test.v1", True)


    def test_Flush(self):
        UsageService.setEnabled(False)
        #this will do nothing as it is disabled
        UsageService.flush()

    def test_Shutdown(self):
        UsageService.shutdown()


if __name__ == '__main__':
    unittest.main()
