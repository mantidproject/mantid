# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.simpleapi import CreateMDHistoWorkspace, DeleteWorkspace, HB3AAdjustSampleNorm, LoadMD


class HB3AAdjustSampleNormTest(unittest.TestCase):
    _tolerance = 1.0e-7

    def setUp(self):
        return

    def tearDown(self):
        return

    def __checkAdjustments(self, orig_pos, new_pos, height, distance):
        # Check the changed height
        np.testing.assert_allclose(new_pos.getY() - orig_pos.getY(), height, self._tolerance)

        # Check the changed distance along x-z
        dist = np.linalg.norm([new_pos.getX() - orig_pos.getX(), new_pos.getZ() - orig_pos.getZ()])
        np.testing.assert_allclose(dist, distance, self._tolerance)

    def testAdjustDetector(self):
        # Test a slight adjustment of the detector position
        height_adj = 0.75
        dist_adj = 0.25
        orig = LoadMD("HB3A_data.nxs", LoadHistory=False)
        # Get the original detector position before adjustment
        orig_pos = orig.getExperimentInfo(0).getInstrument().getDetector(1).getPos()
        result = HB3AAdjustSampleNorm(InputWorkspaces=orig,
                                      DetectorHeightOffset=height_adj,
                                      DetectorDistanceOffset=dist_adj)
        # Get the updated detector position
        new_pos = result.getExperimentInfo(0).getInstrument().getDetector(1).getPos()

        # Verify detector adjustment
        self.__checkAdjustments(orig_pos, new_pos, height_adj, dist_adj)

        DeleteWorkspace(orig, result)

    def testDoNotAdjustDetector(self):
        # Ensure detector position does not change when no offsets are given
        orig = LoadMD("HB3A_data.nxs", LoadHistory=False)
        orig_pos = orig.getExperimentInfo(0).getInstrument().getDetector(1).getPos()
        result = HB3AAdjustSampleNorm(InputWorkspaces=orig, DetectorHeightOffset=0.0, DetectorDistanceOffset=0.0)
        new_pos = result.getExperimentInfo(0).getInstrument().getDetector(1).getPos()

        # Verify detector adjustment
        self.__checkAdjustments(orig_pos, new_pos, 0.0, 0.0)

        DeleteWorkspace(orig, result)

    def testInputFail(self):
        signal = range(0, 1000)
        error = range(0, 1000)
        samplews = CreateMDHistoWorkspace(Dimensionality=3,
                                          SignalInput=signal,
                                          ErrorInput=error,
                                          Extents='-3,3,-3,3,-3,3',
                                          NumberOfBins='10,10,10',
                                          Names='x,y,z',
                                          Units='MomentumTransfer,EnergyTransfer,EnergyTransfer')

        # A MDHisto WS with no experiment info should fail
        with self.assertRaises(RuntimeError):
            HB3AAdjustSampleNorm(InputWorkspaces=samplews,
                                 DetectorHeightOffset=0.0,
                                 DetectorDistanceOffset=0.0,
                                 OutputWorkspace="__tmpout",
                                 Wavelength=2.0)

        DeleteWorkspace(samplews)


if __name__ == '__main__':
    unittest.main()
