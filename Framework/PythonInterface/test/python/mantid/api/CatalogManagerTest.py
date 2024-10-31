# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import CatalogManager


class CatalogManagerTest(unittest.TestCase):
    def test_get_catalog_manager_does_not_return_None(self):
        self.assertNotEqual(CatalogManager, None)

    def test_count_active_sessions(self):
        self.assertEqual(0, CatalogManager.numberActiveSessions(), "Should have zero active sessions without logging on.")

    def test_get_active_sessions(self):
        list_of_sessions = CatalogManager.getActiveSessions()
        self.assertTrue(isinstance(list_of_sessions, list), "Expect a list of sessions back")


if __name__ == "__main__":
    unittest.main()
