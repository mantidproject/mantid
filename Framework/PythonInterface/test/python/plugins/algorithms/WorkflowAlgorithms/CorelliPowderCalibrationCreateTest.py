# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
import unittest

from mantid.api import mtd
from mantid.simpleapi import (
    CloneWorkspace, CompareWorkspaces, CorelliPowderCalibrationCreate, ConvertUnits, CreateSampleWorkspace,
    DeleteWorkspaces, MoveInstrumentComponent, Rebin, RotateInstrumentComponent)


class CorelliPowderCalibrationCreateTest(unittest.TestCase):

    def setUp(self) -> None:
        r"""Fixture runs at the beginning of every test method"""
        spacings_reference = [0.9179, 0.9600, 1.0451, 1.2458, 1.3576, 1.5677, 1.6374, 3.1353]  # silicon
        # add one Gaussian peak for every reference d-spacing
        peak_functions = list()
        for spacing in spacings_reference:
            peak_function = f'name=Gaussian, PeakCentre={spacing}, Height={10 * np.sqrt(spacing)}, Sigma={0.003 * spacing}'
            peak_functions.append(peak_function)
        function = ';'.join(peak_functions)
        begin, end, bin_width = spacings_reference[0] - 0.5, spacings_reference[-1] + 0.5, 0.0001
        # Single 10x10 rectangular detector, located 4m downstream the sample along the X-axis
        # Each detector has the same histogram of intensities, showing eight Gaussian peaks with centers at the
        # reference d-spacings
        CreateSampleWorkspace(WorkspaceType='Histogram', Function='User Defined', UserDefinedFunction=function,
                              XUnit='dSpacing', XMin=begin, XMax=end, BinWidth=bin_width,
                              NumBanks=1, PixelSpacing=0.02, SourceDistanceFromSample=10.0, BankDistanceFromSample=4.0,
                              OutputWorkspace='test_workspace_dSpacing')
        RotateInstrumentComponent(Workspace='test_workspace_dSpacing', ComponentName='bank1', X=0., Y=1., z=0.,
                                  Angle=90, RelativeRotation=True)
        MoveInstrumentComponent(Workspace='test_workspace_dSpacing', ComponentName='bank1', X=4.0, y=0.0, z=0.0,
                                RelativePosition=False)
        # Eight peaks now in TOF. Only when the instrument is located 4m downstream along the X-axis will we obtain
        # the correct d-Spacings if we convert back to dSpacings units. If we perturb the instrument and convert
        # back to dSpacing units, we'll obtain eight peaks centered at d-spacings sligthly different than the
        # reference values
        ConvertUnits(InputWorkspace='test_workspace_dSpacing', Target='TOF', EMode='Elastic',
                     OutputWorkspace='test_workspace_TOF')
        Rebin(InputWorkspace='test_workspace_TOF', Params=[300, 1.0, 16666.7], OutputWorkspace='test_workspace_TOF')
        ConvertUnits(InputWorkspace='test_workspace_TOF', Target='dSpacing', EMode='Elastic',
                     OutputWorkspace='test_workspace_dSpacing')
        self.spacings_reference = spacings_reference

    def tearDown(self) -> None:
        r"""Fixture runs at the end of every test method"""
        DeleteWorkspaces(['test_workspace_dSpacing', 'test_workspace_TOF'])

    def spacings_recovered(self, input_workspace, calibrate=True):
        r"""Compare the input_workspace to the reference workspace 'test_workspace_dSpacing' after being
        converted to TOF units. If calibrate=True, calibrate first and then convert units"""
        if calibrate:
            CorelliPowderCalibrationCreate(
                InputWorkspace='perturbed', OutputWorkspacesPrefix='cal_',
                TofBinning=[300, 1.0, 16666.7], PeakPositions=self.spacings_reference, SourceToSampleDistance=10.0,
                ComponentList='bank1', FixY=False, ComponentMaxTranslation=0.03, FixYaw=False, ComponentMaxRotation=6,
                Minimizer='differential_evolution', MaxIterations=10)
        ConvertUnits(InputWorkspace='perturbed', Target='dSpacing', EMode='Elastic', OutputWorkspace='perturbed_dS')
        results = CompareWorkspaces(Workspace1='test_workspace_dSpacing', Workspace2='perturbed_dS', Tolerance=0.001,
                                    CheckInstrument=False)
        DeleteWorkspaces(['perturbed_dS'])
        return results.Result

    def test_exceptions(self):
        # Both FixSource=True, AdjustSource=True can't be True
        try:
            CorelliPowderCalibrationCreate(
                InputWorkspace='test_workspace_TOF', OutputWorkspacesPrefix='cal_',
                TofBinning=[300, 1.0, 16666.7], PeakPositions=self.spacings_reference, FixSource=True,
                AdjustSource=True, ComponentList='bank1', FixY=False, ComponentMaxTranslation=0.2,
                FixYaw=False, ComponentMaxRotation=10)
        except RuntimeError as error:
            assert 'Some invalid Properties found' in str(error)

        # Both FixSource=True, AdjustSource=True can't be False
        try:
            CorelliPowderCalibrationCreate(
                InputWorkspace='test_workspace_TOF', OutputWorkspacesPrefix='cal_',
                TofBinning=[300, 1.0, 16666.7], PeakPositions=self.spacings_reference, FixSource=False,
                AdjustSource=False, ComponentList='bank1', FixY=False, ComponentMaxTranslation=0.2,
                FixYaw=False, ComponentMaxRotation=10)
        except RuntimeError as error:
            assert 'Some invalid Properties found' in str(error)

    @unittest.skip("causes surpassing the timeout in the Jenkins servers")
    def test_translation(self):
        CloneWorkspace(InputWorkspace='test_workspace_TOF', OutputWorkspace='perturbed')
        CloneWorkspace(InputWorkspace='test_workspace_TOF', OutputWorkspace='perturbed')
        MoveInstrumentComponent(Workspace='perturbed', ComponentName='bank1', X=0.02, y=0.005, z=0.005,
                                RelativePosition=True)
        assert self.spacings_recovered('perturbed', calibrate=False) is False
        assert self.spacings_recovered('perturbed', calibrate=True)
        DeleteWorkspaces(['perturbed'])

    @unittest.skip("causes surpassing the timeout in the Jenkins servers")
    def test_rotation(self):
        CloneWorkspace(InputWorkspace='test_workspace_TOF', OutputWorkspace='perturbed')
        RotateInstrumentComponent(Workspace='perturbed', ComponentName='bank1', X=0, Y=0, z=1, Angle=5,
                                  RelativeRotation=True)
        assert self.spacings_recovered('perturbed', calibrate=False) is False
        assert self.spacings_recovered('perturbed', calibrate=True)
        DeleteWorkspaces(['perturbed'])

    def test_translation_rotation(self):
        CloneWorkspace(InputWorkspace='test_workspace_TOF', OutputWorkspace='perturbed')
        MoveInstrumentComponent(Workspace='perturbed', ComponentName='bank1', X=0.02, y=0.005, z=0.005,
                                RelativePosition=True)
        RotateInstrumentComponent(Workspace='perturbed', ComponentName='bank1', X=0, Y=0, z=1, Angle=5,
                                  RelativeRotation=True)
        assert self.spacings_recovered('perturbed', calibrate=False) is False
        assert self.spacings_recovered('perturbed', calibrate=True)
        DeleteWorkspaces(['perturbed'])

    def test_fix_y(self) -> None:
        CloneWorkspace(InputWorkspace='test_workspace_TOF', OutputWorkspace='perturbed')
        y = -0.0042  # desired fixed position
        MoveInstrumentComponent(Workspace='perturbed', ComponentName='bank1', X=0, y=y, z=0,
                                RelativePosition=False)
        r"""Pass option FixY=True"""
        CorelliPowderCalibrationCreate(
            InputWorkspace='perturbed', OutputWorkspacesPrefix='cal_',
            TofBinning=[300, 1.0, 16666.7], PeakPositions=self.spacings_reference, SourceToSampleDistance=10.0,
            ComponentList='bank1', FixY=True, ComponentMaxTranslation=0.2, ComponentMaxRotation=10,
            Minimizer='L-BFGS-B')
        # Check Y-position of first bank hasn't changed
        row = mtd['cal_adjustments'].row(1)
        self.assertAlmostEqual(row['Yposition'], y, places=5)
        DeleteWorkspaces(['perturbed'])

    def test_fix_yaw(self) -> None:
        CloneWorkspace(InputWorkspace='test_workspace_TOF', OutputWorkspace='perturbed')
        RotateInstrumentComponent(Workspace='perturbed', ComponentName='bank1', X=0, Y=0, z=1, Angle=5,
                                  RelativeRotation=True)
        r"""Pass option FixYaw=True"""
        CorelliPowderCalibrationCreate(
            InputWorkspace='perturbed', OutputWorkspacesPrefix='cal_',
            TofBinning=[300, 1.0, 16666.7], PeakPositions=self.spacings_reference, SourceToSampleDistance=10.0,
            ComponentList='bank1', ComponentMaxTranslation=0.2, FixYaw=True, ComponentMaxRotation=10,
            Minimizer='L-BFGS-B')
        # Check no change in the rotations around Z-axis of first bank
        row = mtd['cal_displacements'].row(0)
        self.assertAlmostEqual(row['DeltaGamma'], 0.0, places=5)
        DeleteWorkspaces(['perturbed'])

if __name__ == '__main__':
    unittest.main()
