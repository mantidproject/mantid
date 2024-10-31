# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-public-methods
import unittest

from mantid.simpleapi import CreateGroupingWorkspace, CreateSampleWorkspace
import numpy as np


class GroupingWorkspaceTest(unittest.TestCase):
    def setUp(self):
        myFunc = (
            "name=Gaussian, PeakCentre=2, Height=100, Sigma=0.01;"
            + "name=Gaussian, PeakCentre=1, Height=100, Sigma=0.01;"
            + "name=Gaussian, PeakCentre=4, Height=100, Sigma=0.01"
        )
        ws = CreateSampleWorkspace(
            "Event", "User Defined", myFunc, BankPixelWidth=1, XUnit="dSpacing", XMax=5, BinWidth=0.001, NumEvents=100000, NumBanks=8
        )
        self.groupingWorkspace, _, _ = CreateGroupingWorkspace(
            InputWorkspace=ws, ComponentName="basic_rect", CustomGroupingString="1-4,5-8"
        )

    def test_get_total_groups(self):
        self.assertEqual(2, self.groupingWorkspace.getTotalGroups())

    def test_get_group_IDs(self):
        ids = self.groupingWorkspace.getGroupIDs()
        self.assertEqual(2, len(ids))

    def test_get_group_spectra_IDs(self):
        self.assertEqual(0, len(self.groupingWorkspace.getDetectorIDsOfGroup(-1)))  # default sws2d values are all 0

    def test_get_group_IDs_values(self):
        expected = np.sort(np.unique(self.groupingWorkspace.extractY()))
        actual = np.sort(self.groupingWorkspace.getGroupIDs())
        np.testing.assert_allclose(actual, expected)

    def test_get_groupSpectra_IDs_values(self):
        group_list = self.groupingWorkspace.getGroupIDs()
        self.assertNotEqual(0, len(group_list))
        for group in group_list:
            indexes = np.where(self.groupingWorkspace.extractY().flatten() == group)[0]
            expected = np.sort(np.array(self.groupingWorkspace.getSpectrumNumbers())[indexes])
            actual = np.sort(self.groupingWorkspace.getDetectorIDsOfGroup(group.item()))
            np.testing.assert_allclose(actual, expected)


if __name__ == "__main__":
    unittest.main()
