# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd, WorkspaceGroup
from mantid.simpleapi import IndirectDiffScan


class IndirectDiffScanTest(unittest.TestCase):
    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        IndirectDiffScan(InputFiles=["IRS26176.RAW"], Instrument="IRIS", SpectraRange=[105, 112])

        wks = mtd["Output"]
        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], "iris26176_diffspec_red")

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), "dSpacing")
        self.assertEqual(red_ws.getNumberHistograms(), 1)

        scan_ws = mtd["Output_scan"]
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 6.6324986)

    def test_multi_files(self):
        """
        Test reducing multiple files.
        """

        IndirectDiffScan(InputFiles=["IRS26176.RAW", "IRS26173.RAW"], Instrument="IRIS", SpectraRange=[105, 112])

        wks = mtd["Output"]
        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(len(wks), 2)
        self.assertEqual(wks.getNames()[0], "iris26176_diffspec_red")
        self.assertEqual(wks.getNames()[1], "iris26173_diffspec_red")

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), "dSpacing")
        self.assertEqual(red_ws.getNumberHistograms(), 1)

        scan_ws = mtd["Output_scan"]
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 6.6324986)

    def test_osiris(self):
        """
        Test with OSIRIS
        """
        IndirectDiffScan(InputFiles=["osi89813.raw"], Instrument="OSIRIS", SpectraRange=[100, 150])

        wks = mtd["Output"]
        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], "osiris89813_diffspec_red")

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), "dSpacing")
        self.assertEqual(red_ws.getNumberHistograms(), 1)


if __name__ == "__main__":
    unittest.main()
