# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name
import unittest
from mantid.api import WorkspaceGroup
from mantid.simpleapi import VesuvioDiffractionReduction
from mantid.kernel import config


class VesuvioDiffractionReductionTest(unittest.TestCase):
    def setUp(self):
        self._oldFacility = config["default.facility"]
        if self._oldFacility.strip() == "":
            self._oldFacility = "TEST_LIVE"
        config.setFacility("ISIS")

    def tearDown(self):
        config.setFacility(self._oldFacility)

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = VesuvioDiffractionReduction(InputFiles=["29244"], InstrumentParFIle="IP0005.dat")

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], "vesuvio29244_diffspec_red")

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), "dSpacing")
        self.assertEqual(red_ws.getNumberHistograms(), 1)

    def test_grouping_individual(self):
        """
        Test setting individual grouping, one spectrum per detector.
        """

        wks = VesuvioDiffractionReduction(InputFiles=["29244"], GroupingPolicy="Individual", InstrumentParFIle="IP0005.dat")

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(len(wks), 1)

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), "dSpacing")
        self.assertEqual(red_ws.getNumberHistograms(), 196)


if __name__ == "__main__":
    unittest.main()
