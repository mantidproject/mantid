from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (CreateWorkspace, Load, ConvertUnits,
                              SplineInterpolation, ApplyPaalmanPingsCorrection,
                              DeleteWorkspace, GroupWorkspaces, CloneWorkspace)
import numpy


class ApplyPaalmanPingsCorrectionTest(unittest.TestCase):

    def setUp(self):
        """
        Create sample workspaces.
        """

        # Load the sample and can
        sample_ws = Load('irs26176_graphite002_red.nxs')
        can_ws = Load('irs26173_graphite002_red.nxs')

        # Convert sample and can to wavelength
        sample_ws = ConvertUnits(InputWorkspace=sample_ws,
                                 Target='Wavelength',
                                 EMode='Indirect',
                                 EFixed=1.845)
        can_ws = ConvertUnits(InputWorkspace=can_ws,
                              Target='Wavelength',
                              EMode='Indirect',
                              EFixed=1.845)

        self._sample_ws = sample_ws
        self._can_ws = can_ws

        # Load the corrections workspace
        corrections = Load('irs26176_graphite002_cyl_Abs.nxs')

        # Interpolate each of the correction factor workspaces
        # Required to use corrections from the old indirect calculate
        # corrections routines
        for factor_ws in corrections:
            SplineInterpolation(WorkspaceToMatch=sample_ws,
                                WorkspaceToInterpolate=factor_ws,
                                OutputWorkspace=factor_ws,
                                OutputWorkspaceDeriv='')

        self._corrections_ws = corrections


    def tearDown(self):
        """
        Remove workspaces from ADS.
        """

        DeleteWorkspace(self._sample_ws)
        DeleteWorkspace(mtd['can_ws'])
        DeleteWorkspace(self._corrections_ws)


    def _verify_workspace(self, ws, correction_type):
        """
        Do validation on a correction workspace.

        @param ws Workspace to validate
        @param correction_type Type of correction that should have been applied
        """

        # X axis should be in wavelength
        x_unit = ws.getAxis(0).getUnit().unitID()
        self.assertEquals(x_unit, 'Wavelength')

        # Sample logs should contain correction type
        logs = ws.getSampleDetails()
        self.assertTrue('corrections_type' in logs)

        # Ensure value from sample log is correct
        if 'corrections_type' in logs:
            log_correction_type = logs['corrections_type'].value
            self.assertEqual(log_correction_type, correction_type)


    def _create_group_of_factors(self, corrections, factors):
        def is_factor(workspace, factor):
            if factor == "ass":
                return factor in workspace.getName() and not 'assc' in workspace.getName()
            else:
                return factor in workspace.getName()

        def is_factor_workspace(workspace):
            return any([is_factor(workspace, factor) for factor in factors])

        filtered_corr = filter(is_factor_workspace, corrections)
        cloned_corr = [CloneWorkspace(InputWorkspace=correction,
                                      OutputWorkspace=correction.getName() + "_clone")
                       for correction in filtered_corr]
        correction_names = [correction.getName() for correction in cloned_corr]
        return GroupWorkspaces(InputWorkspaces=correction_names, OutputWorkspace="factor_group")


    def test_can_subtraction(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CanWorkspace=self._can_ws)

        self._verify_workspace(corr, 'can_subtraction')


    def test_can_subtraction_with_can_scale(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CanWorkspace=self._can_ws,
                                           CanScaleFactor=0.9)

        self._verify_workspace(corr, 'can_subtraction')

    def test_can_subtraction_with_can_shift(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CanWorkspace=self._can_ws,
                                           CanShiftFactor=0.03)

        self._verify_workspace(corr, 'can_subtraction')


    def test_sample_corrections_only(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CorrectionsWorkspace=self._corrections_ws)

        self._verify_workspace(corr, 'sample_corrections_only')


    def test_sample_and_can_corrections(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CorrectionsWorkspace=self._corrections_ws,
                                           CanWorkspace=self._can_ws)

        self._verify_workspace(corr, 'sample_and_can_corrections')


    def test_sample_and_can_corrections_with_can_scale(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CorrectionsWorkspace=self._corrections_ws,
                                           CanWorkspace=self._can_ws,
                                           CanScaleFactor=0.9)

        self._verify_workspace(corr, 'sample_and_can_corrections')

    def test_sample_and_can_corrections_with_can_shift(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CorrectionsWorkspace=self._corrections_ws,
                                           CanWorkspace=self._can_ws,
                                           CanShiftFactor = 0.03)

        self._verify_workspace(corr, 'sample_and_can_corrections')

    def test_two_factor_approximation(self):
        two_corrections = self._create_group_of_factors(self._corrections_ws, ['acc', 'ass'])

        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CorrectionsWorkspace=two_corrections,
                                           CanWorkspace=self._can_ws,
                                           CanShiftFactor = 0.03)
        DeleteWorkspace(two_corrections)
        self._verify_workspace(corr, 'sample_and_can_corrections')

    def test_container_input_workspace_not_unintentionally_rebinned(self):
        xs = numpy.array([0.0, 1.0, 0.0, 1.1])
        ys = numpy.array([2.2, 3.3])
        sample_1 = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2,
                                   UnitX='Wavelength')
        ys = numpy.array([0.11, 0.22])
        container_1 = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2,
                                      UnitX='Wavelength')
        corrected = ApplyPaalmanPingsCorrection(SampleWorkspace=sample_1,
                                                CanWorkspace=container_1)
        numHisto = container_1.getNumberHistograms()
        for i in range(numHisto):
            container_xs = container_1.readX(i)
            for j in range(len(container_xs)):
                self.assertEqual(container_xs[j], xs[i * numHisto + j])
        DeleteWorkspace(sample_1)
        DeleteWorkspace(container_1)
        DeleteWorkspace(corrected)

    def test_container_rebinning_enabled(self):
        xs = numpy.array([0.0, 1.0, 0.0, 1.1])
        ys = numpy.array([2.2, 3.3])
        sample_1 = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2,
                                   UnitX='Wavelength')
        xs = numpy.array([-1.0, 0.0, 1.0, 2.0, -1.0, 0.0, 1.0, 2.0])
        ys = numpy.array([0.101, 0.102, 0.103, 0.104, 0.105, 0.106])
        container_1 = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2,
                                      UnitX='Wavelength')
        corrected = ApplyPaalmanPingsCorrection(SampleWorkspace=sample_1,
                                                CanWorkspace=container_1,
                                                RebinCanToSample=True)
        self.assertTrue(numpy.all(sample_1.extractY() > corrected.extractY()))
        DeleteWorkspace(sample_1)
        DeleteWorkspace(container_1)
        DeleteWorkspace(corrected)

    def test_container_rebinning_disabled(self):
        xs = numpy.array([0.0, 1.0, 0.0, 1.1])
        ys = numpy.array([2.2, 3.3])
        sample_1 = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2,
                                   UnitX='Wavelength')
        xs = numpy.array([-1.0, 0.0, 1.0, 2.0, -1.0, 0.0, 1.0, 2.0])
        ys = numpy.array([0.101, 0.102, 0.103, 0.104, 0.105, 0.106])
        container_1 = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2,
                                      UnitX='Wavelength')
        corrected_ws_name = 'corrected_workspace'
        kwargs = {
            'SampleWorkspace': sample_1,
            'CanWorkspace': container_1,
            'OutputWorkspaced': corrected_ws_name,
            'RebinCanToSample': False
        }
        # The Minus algorithm will fail due to different bins in sample and
        # container.
        self.assertRaises(RuntimeError, ApplyPaalmanPingsCorrection, **kwargs)
        DeleteWorkspace(sample_1)
        DeleteWorkspace(container_1)

if __name__=="__main__":
    unittest.main()
