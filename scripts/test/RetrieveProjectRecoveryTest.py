from __future__ import (absolute_import, division, print_function)
import unittest
from ErrorReporter.retrieve_recovery_files import RetrieveRecoveryFiles
from mantid.kernel import ConfigService
import os


class RetrieveProjectRecoveryTest(unittest.TestCase):
    def test_get_properties_directory_returns_correct_directory(self):
        directory = RetrieveRecoveryFiles._get_properties_directory()

        self.assertEqual(directory, ConfigService.getUserPropertiesDir())

    def test_zip(self):
        retrieve_recovery_files = RetrieveRecoveryFiles()
        retrieve_recovery_files.zip_recovery_directory()



if __name__ == '__main__':
    unittest.main()