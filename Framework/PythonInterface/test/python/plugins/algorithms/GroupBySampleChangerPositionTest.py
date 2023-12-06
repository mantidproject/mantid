# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name
import unittest
from mantid.simpleapi import ISISIndirectEnergyTransfer, GroupBySampleChangerPosition, AnalysisDataServiceImpl


class GroupBySampleChangerPositionTest(unittest.TestCase):
    def test_group_with_no_sample_position(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles="IRS26176.RAW", Instrument="IRIS", Analyser="graphite", Reflection="002", SpectraRange=[3, 53]
        )
        GroupBySampleChangerPosition(InputWorkspace=wks, OutputGroupPrefix="Prefix", OutputGroupSuffix="Suffix")
        ads = AnalysisDataServiceImpl.Instance()
        self.assertFalse(ads.doesExist("Prefix_btm_Suffix"))
        self.assertFalse(ads.doesExist("Prefix_mid_Suffix"))
        self.assertFalse(ads.doesExist("Prefix_top_Suffix"))
        self.assertTrue(ads.doesExist("Prefix_Suffix"))
        ads.remove("Prefix_Suffix")

    def test_group_with_sample_position(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles="IRS94297.nxs, IRS94298.nxs, IRS94299.nxs",
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
        )
        GroupBySampleChangerPosition(InputWorkspace=wks, OutputGroupPrefix="Prefix", OutputGroupSuffix="Suffix")
        ads = AnalysisDataServiceImpl.Instance()
        self.assertTrue(ads.doesExist("Prefix_btm_Suffix"))
        self.assertTrue(ads.doesExist("Prefix_mid_Suffix"))
        self.assertTrue(ads.doesExist("Prefix_top_Suffix"))
        self.assertFalse(ads.doesExist("Prefix_Suffix"))
        ads.remove("Prefix_btm_Suffix")
        ads.remove("Prefix_mid_Suffix")
        ads.remove("Prefix_top_Suffix")


if __name__ == "__main__":
    unittest.main()
