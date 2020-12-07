# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from numpy.testing import assert_allclose
import unittest

from mantid.api import mtd
from mantid.simpleapi import (
    CorelliPowderCalibrationCreate, CreateSampleWorkspace, MoveInstrumentComponent, RotateInstrumentComponent)


class CorelliPowderCalibrationCreateTest(unittest.TestCase):

    def test_exec(self):
        # Single 10x10 rectangular detector, located 5m downstream the sample
        CreateSampleWorkspace(WorkspaceType="Event", Function="Powder Diffraction", XMin=300, XMax=16666.7, BinWidth=1,
                              NumBanks=1, NumEvents=100000, PixelSpacing=0.02, OutputWorkspace="test_workspace")
        # The detector ID at the center of the detector panel is detector-ID = 155, corresponding to workspace index 55.
        # When the detector panel is placed perpendicular to the X axis and five meters away from the sample,
        # detector-ID 155 shows nine peaks with the following peak-centers, in d-spacing (Angstroms) units:
        spacings_reference = [0.304670, 0.610286, 0.915385, 1.220476, 1.525575, 1.830671, 2.135765, 2.44092, 2.74598]
        # We select these d-spacings as the reference d-spacings
        # Place the detector at a position and orientation close, but not equal to, perpendicular to the X axis
        # 5 meters from the sample
        RotateInstrumentComponent(Workspace='test_workspace', ComponentName='bank1', X=0.1, Y=99, z=0.1, Angle=88,
                                  RelativeRotation=True)
        MoveInstrumentComponent(Workspace='test_workspace', ComponentName='bank1', X=4.98, y=-0.12, z=0.08,
                                RelativePosition=False)
        # The calibration algorithm will attempt to correct the position and orientation of the bank so that peak
        # centers for all detectors in the bank (not just detector-ID 155) approach our reference values. As
        # a result, the final position and orientation is not exactly perpendicular to the X-axis and positioned
        # five meters away from the sample.
        target_position, target_orientation, target_rotation = [5.18, -0.32, 0.21], [0.001, 0.999, -0.027], 98.0
        CorelliPowderCalibrationCreate(
            InputWorkspace='test_workspace', OutputWorkpsacesPrefix='cal_', TubeDatabaseDir='/tmp',
            TofBinnin=[300, 1.0, 16666.7], PeakPositions=spacings_reference, AdjustSource=False, ComponentList='bank1',
            ComponentMaxTranslation=0.2, ComponentMaxRotation=10)
        values_output = mtd['cal_adjustments'].row(0)
        assert_allclose(values_output[:3], target_position, atol=0.001)
        assert_allclose(values_output[3: 6], target_orientation, atol=0.001)
        assert_allclose(values_output[-1], target_rotation, atol=0.1)


if __name__ == '__main__':
    unittest.main()
