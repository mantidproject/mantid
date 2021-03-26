# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import unittest
from mantid.simpleapi import (AlignComponents, ConvertUnits, CloneWorkspace, CreateSampleWorkspace, Max,
                              MoveInstrumentComponent, CreateEmptyTableWorkspace, mtd, RotateInstrumentComponent)
from mantid.api import AlgorithmFactory
from mantid.kernel import V3D


class AlignComponentsTest(unittest.TestCase):
    def testAlignComponentsPosition(self):
        r"""

        CreateSampleWorkspace here generates one bank of 2x2 pixels. All pixels have a single peak at
        TOF=10000 micro-seconds. The bank is facing the sample, centered along the vertical axis (Y),
        and at a scattering angle from the beam axis (Z).
        Because the pixels are at different locations, converting units to d-spacing results in peaks at different
        values of d-spacing. For the bank in this test we have:

            workspace index       | 0      | 1      | 2      | 3      |
            ----------------------|--------|--------|--------|--------|
            detector ID           | 4      | 5      | 6      | 6      |
            ----------------------|--------|--------|--------|--------|
            peack-center TOF      |10000   | 10000  | 10000  | 10000  |
            ----------------------|--------|--------|--------|--------|
            peak-center d-spacing | 5.2070 | 5.2070 | 5.1483 | 5.1483 |

        The first workspace index corresponds to a pixel centered on the beam axis, hence the scattering angle is zero
        and the corresponding d-spacing is infinite.

        Correspondence betwee
        """
        def serve_instrument(output_workspace):
            scattering_angle = 20  # in degrees
            # Instrument with four spectra. Each spectrum has one single Gaussian peak in TOF, centered at 10000
            CreateSampleWorkspace(OutputWorkspace=output_workspace,
                                  BinWidth=0.1,
                                  NumBanks=1,
                                  BankPixelWidth=2,
                                  Function='User Defined',
                                  UserDefinedFunction='name=Gaussian, PeakCentre=10000, Height=100, Sigma=2',
                                  Xmin=9900,
                                  Xmax=10100,
                                  BankDistanceFromSample=2,
                                  SourceDistanceFromSample=20)
            MoveInstrumentComponent(Workspace=output_workspace,
                                    ComponentName='bank1',
                                    RelativePosition=False,
                                    X=0,
                                    Y=0.0,
                                    Z=0)  # move to the origin
            RotateInstrumentComponent(Workspace=output_workspace,
                                      ComponentName='bank1',
                                      X=0,
                                      Y=1,
                                      Z=0,
                                      Angle=scattering_angle,
                                      RelativeRotation=False)
            sin, cos = np.sin(np.radians(scattering_angle)), np.cos(np.radians(scattering_angle))
            # detector pixel width is 0.008m, perpendicular to the scattered beam. Detector is 2m away from the sample
            z, x = 2 * cos + 0.004 * sin, 2 * sin - 0.004 * cos
            MoveInstrumentComponent(Workspace=output_workspace,
                                    ComponentName='bank1',
                                    RelativePosition=False,
                                    X=x,
                                    Y=0,
                                    Z=z)  # translate 2 meters away and center the detector
            return mtd['original']

        serve_instrument('original')
        # Convert to d-spacing
        ConvertUnits(InputWorkspace='original', Target='dSpacing', EMode='Elastic', OutputWorkspace='original_dspacing')
        # Find the bin boundaries limiting the peak maximum
        Max(InputWorkspace='original_dspacing', OutputWorkspace='original_d_at_max')
        # Average the two bin boundaries to find the bin center, taken to be the location of the peak center
        original_d = np.average(mtd['original_d_at_max'].extractX(), axis=1)

        peak_positions = [5.1483, 5.2070]  # reference peak positions in d-spacing (Angstroms)

        # Generate a table of peak centers in TOF units
        table_tofs = CreateEmptyTableWorkspace(OutputWorkspace='table_tofs')
        column_info = [('int', 'detid'), ('double', '@5.1483'), ('double', '@5.2070')]
        [table_tofs.addColumn(c_type, c_name) for c_type, c_name in column_info]
        table_tofs.addRow([4, float('nan'), 10000.0])
        table_tofs.addRow([5, float('nan'), 10000.0])  # a peak in TOF correspoding to a peak of 306.5928 Angstroms
        table_tofs.addRow([6, 10000.0, float('nan')])  # a peak in TOF correspoding to a peak of 306.5928 Angstroms
        table_tofs.addRow([7, 10000.0, float('nan')])  # a peak in TOF correspoding to a peak of 216.7940 Angstroms

        # perturb the position of the bank with a translation or the order of a few milimeters
        component = 'bank1'
        xyz_shift = V3D(0.005, 0.010, 0.007)
        CloneWorkspace(InputWorkspace='original', OutputWorkspace='perturbed')
        MoveInstrumentComponent(Workspace='perturbed',
                                ComponentName='bank1',
                                RelativePosition=True,
                                X=xyz_shift.X(),
                                Y=xyz_shift.Y(),
                                Z=xyz_shift.Z())

        # calibrate the perturbed bank
        AlignComponents(PeakCentersTofTable='table_tofs',
                        PeakPositions=peak_positions,
                        OutputWorkspace='calibrated',
                        InputWorkspace='perturbed',
                        ComponentList=component,
                        AdjustmentsTable='adjustments',
                        Xposition=True,
                        Yposition=True,
                        Zposition=True)

        # compare the peak-centers (in d-spacing units) between the original and calibrated
        # spectra, up to 0.001 Angstroms
        ConvertUnits(InputWorkspace='calibrated',
                     Target='dSpacing',
                     EMode='Elastic',
                     OutputWorkspace='calibrated_dspacing')
        Max(InputWorkspace='calibrated_dspacing', OutputWorkspace='calibrated_d_at_max')
        calibrated_d = np.average(mtd['calibrated_d_at_max'].extractX(), axis=1)
        assert np.allclose(original_d, calibrated_d, atol=0.001)

        # perturb the orientation of the bank with a rotation of a small angle around an axis almost parallel
        # to the vertical
        axis_shift, angle_shift = V3D(0.2, 0.8, np.sqrt(1 - 0.2**2 - 0.8**2)), 9.0  # angle shift in degrees
        CloneWorkspace(InputWorkspace='original', OutputWorkspace='perturbed')
        RotateInstrumentComponent(Workspace='perturbed',
                                  ComponentName='bank1',
                                  RelativeRotation=True,
                                  X=axis_shift.X(),
                                  Y=axis_shift.Y(),
                                  Z=axis_shift.Z(),
                                  Angle=angle_shift)
        ConvertUnits(InputWorkspace='perturbed',
                     OutputWorkspace='perturbed_dspacing',
                     Target='dSpacing',
                     Emode='Elastic')

        # calibrate the perturbed bank
        AlignComponents(PeakCentersTofTable='table_tofs',
                        PeakPositions=peak_positions,
                        OutputWorkspace='calibrated',
                        InputWorkspace='perturbed',
                        ComponentList=component,
                        AdjustmentsTable='adjustments',
                        AlphaRotation=True,
                        BetaRotation=True,
                        GammaRotation=True)
        ConvertUnits(InputWorkspace='calibrated',
                     OutputWorkspace='calibrated_dspacing',
                     Target='dSpacing',
                     Emode='Elastic')

        # compare the peak-centers (in d-spacing units) between the original and calibrated
        # spectra, up to 0.001 Angstroms
        Max(InputWorkspace='calibrated_dspacing', OutputWorkspace='calibrated_d_at_max')
        calibrated_d = np.average(mtd['calibrated_d_at_max'].extractX(), axis=1)
        assert np.allclose(original_d, calibrated_d, atol=0.001)


if __name__ == "__main__":
    # Only test is Algorithm is loaded
    if AlgorithmFactory.exists("AlignComponents"):
        unittest.main()
