# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import DeleteWorkspace, LoadMD, HB3AAdjustSampleNorm, mtd
from mantid.kernel import V3D


class HB3AAdjustSampleNormTest(unittest.TestCase):
    _tolerance = 1.0e-7

    def setUp(self):
        return

    def tearDown(self):
        return

    def __compareBanks(self, orig_component, new_component, offset):
        for bank in range(1, 4):
            orig_ind = orig_component.indexOfAny("bank{}".format(bank))
            orig_pos = orig_component.position(orig_ind)
            new_ind = new_component.indexOfAny("bank{}".format(bank))
            new_pos = new_component.position(new_ind)

            self.assertAlmostEqual(new_pos, orig_pos + V3D(offset, offset, offset), self._tolerance)

    def testAdjustDetector(self):
        orig = LoadMD("HB3A_data.nxs", MetadataOnly=True)
        orig_component = orig.getExperimentInfo(0).componentInfo()
        result = HB3AAdjustSampleNorm(Filename="HB3A_data.nxs", VanadiumFile="HB2C_WANDSCD_norm.nxs",
                                      DetectorHeightOffset=2.0, DetectorDistanceOffset=2.0)
        result_component = result.getExperimentInfo(0).componentInfo()

        self.__compareBanks(orig_component, result_component, 2.0)

        DeleteWorkspace(orig, result)

    def testDoNotAdjustDetector(self):
        orig = LoadMD("HB3A_data.nxs", MetadataOnly=True)
        orig_component = orig.getExperimentInfo(0).componentInfo()
        result = HB3AAdjustSampleNorm(Filename="HB3A_data.nxs", VanadiumFile="HB2C_WANDSCD_norm.nxs")
        result_component = result.getExperimentInfo(0).componentInfo()

        self.__compareBanks(orig_component, result_component, 0.0)

        DeleteWorkspace(orig, result)


if __name__ == '__main__':
    unittest.main()
