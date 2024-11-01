# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
    Specifically tests the RenameWorkspace algorithm in the simple API
"""
import unittest
from mantid.api import mtd
from mantid.simpleapi import CreateSampleWorkspace, RenameWorkspace


class SimpleAPIRenameWorkspaceTest(unittest.TestCase):
    _raw_ws = None

    def setUp(self):
        if self._raw_ws is None:
            ws = CreateSampleWorkspace()
            ws_mon = CreateSampleWorkspace()
            ws.setMonitorWorkspace(ws_mon)
            self.__class__._raw_ws = ws

    def test_renameWorkspace(self):
        RenameWorkspace(self._raw_ws, "name1", RenameMonitors=True)
        self.assertTrue("name1" in mtd)
        self.assertTrue("name1_monitors" in mtd)

        RenameWorkspace(InputWorkspace="name1", OutputWorkspace="name2", RenameMonitors=True)
        self.assertFalse("name1" in mtd)
        self.assertFalse("name1_monitors" in mtd)
        self.assertTrue("name2" in mtd)
        self.assertTrue("name2_monitors" in mtd)

        RenameWorkspace("name2", "name3", True)
        self.assertFalse("name2" in mtd)
        self.assertFalse("name2_monitors" in mtd)
        self.assertTrue("name3" in mtd)
        self.assertTrue("name3_monitors" in mtd)

        RenameWorkspace("name3", "name4")
        self.assertFalse("name3" in mtd)
        self.assertTrue("name3_monitors" in mtd)
        self.assertTrue("name4" in mtd)
        self.assertFalse("name4_monitors" in mtd)

        RenameWorkspace("name3_monitors", "name4_monitors", True)
        self.assertFalse("name3_monitors" in mtd)
        self.assertTrue("name4_monitors" in mtd)

        RenameWorkspace("name4", "name5")
        self.assertFalse("name4" in mtd)
        self.assertTrue("name4_monitors" in mtd)
        self.assertTrue("name5" in mtd)
        self.assertFalse("name5_monitors" in mtd)

        RenameWorkspace("name5", "name6", True)
        self.assertFalse("name5" in mtd)
        self.assertFalse("name4_monitors" in mtd)
        self.assertTrue("name6" in mtd)
        self.assertTrue("name6_monitors" in mtd)

        name7 = RenameWorkspace("name6", RenameMonitors=True)
        self.assertFalse("name6" in mtd)
        self.assertFalse("name6_monitors" in mtd)
        self.assertTrue("name7" in mtd)
        self.assertTrue("name7_monitors" in mtd)

        ws = RenameWorkspace(name7, OutputWorkspace="name8", RenameMonitors=True)
        self.assertEqual(ws.name(), "name8")
        self.assertFalse("name7" in mtd)
        self.assertFalse("name7_monitors" in mtd)
        self.assertTrue("name8" in mtd)
        self.assertTrue("name8_monitors" in mtd)

        # test with the logging keyword
        try:
            name9 = RenameWorkspace("name8", EnableLogging=False)
            self.assertTrue("name9" in mtd)
            self.assertTrue(name9)
            self.assertFalse("name8" in mtd)
        except RuntimeError as e:
            self.fail("Magic keyword not recognised " + e.what())

        # test trying to disable ADS
        try:
            RenameWorkspace("name9", "name10", StoreInADS=False)
            self.fail("Disabling on ADS did not throw for RenameWorkspace")
        except KeyError:
            pass

        # test text value for disabling
        try:
            RenameWorkspace("name9", "name10", StoreInADS="disable")
            self.fail("Disabling on ADS did not throw for RenameWorkspace")
        except KeyError:
            pass


if __name__ == "__main__":
    unittest.main()
