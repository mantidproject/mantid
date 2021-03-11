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
                              NumBanks=1, NumEvents=100000, PixelSpacing=0.02, OutputWorkspace="test_workspace",
                              SourceDistanceFromSample=10.0, BankDistanceFromSample=5.0)
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

        # Both FixSource=True, AdjustSource=True can't be True
        try:
            CorelliPowderCalibrationCreate(
                InputWorkspace='test_workspace', OutputWorkspacesPrefix='cal_',
                TofBinning=[300, 1.0, 16666.7], PeakPositions=spacings_reference, FixSource=True, AdjustSource=True,
                ComponentList='bank1', ComponentMaxTranslation=0.2, ComponentMaxRotation=10)
        except RuntimeError as error:
            assert 'Some invalid Properties found' in str(error)

        # Both FixSource=True, AdjustSource=True can't be False
        try:
            CorelliPowderCalibrationCreate(
                InputWorkspace='test_workspace', OutputWorkspacesPrefix='cal_',
                TofBinning=[300, 1.0, 16666.7], PeakPositions=spacings_reference, FixSource=False, AdjustSource=False,
                ComponentList='bank1', ComponentMaxTranslation=0.2, ComponentMaxRotation=10)
        except RuntimeError as error:
            assert 'Some invalid Properties found' in str(error)

        # The calibration algorithm will attempt to correct the position and orientation of the bank so that peak
        # centers for all detectors in the bank (not just detector-ID 155) approach our reference values. As
        # a result, the final position and orientation is not exactly perpendicular to the X-axis and positioned
        # five meters away from the sample.
        CorelliPowderCalibrationCreate(
            InputWorkspace='test_workspace', OutputWorkspacesPrefix='cal_',
            TofBinning=[300, 1.0, 16666.7], PeakPositions=spacings_reference, SourceToSampleDistance=10.0,
            ComponentList='bank1', ComponentMaxTranslation=0.2, ComponentMaxRotation=10)
        # Check source position
        row = mtd['cal_adjustments'].row(0)
        assert_allclose([row[name] for name in ('Xposition', 'Yposition', 'Zposition')], [0., 0., -10.0], atol=0.001)
        # Check position of first bank
        row = mtd['cal_adjustments'].row(1)
        target_position, target_orientation, target_rotation = [5.18, -0.32,  0.20], [0.001, 0.999, -0.027], 98.0
        # ToDO investigate the relatively large tolerance required for some operative systems, atol=0.05
        assert_allclose([row[name] for name in ('Xposition', 'Yposition', 'Zposition')], target_position, atol=0.05)
        assert_allclose([row[name] for name in ('XdirectionCosine', 'YdirectionCosine', 'ZdirectionCosine')],
                        target_orientation, atol=0.05)
        assert_allclose(row['RotationAngle'], target_rotation, atol=2.0)


if __name__ == '__main__':
    unittest.main()
