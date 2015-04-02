import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import Load, ConvertUnits, SplineInterpolation, ApplyPaalmanPingsCorrection, DeleteWorkspace


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
        DeleteWorkspace(self._can_ws)
        DeleteWorkspace(self._corrections_ws)


    def _verify_workspace(self, ws, correction_type):
        """
        Do validation on a correction workspace.

        @param ws Workspace to validate
        @param correction_type Type of correction that should hav ebeen applied
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


    def test_can_subtraction(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CanWorkspace=self._can_ws)

        self._verify_workspace(corr, 'can_subtraction')


    def test_can_subtraction_with_can_scale(self):
        corr = ApplyPaalmanPingsCorrection(SampleWorkspace=self._sample_ws,
                                           CanWorkspace=self._can_ws,
                                           CanScaleFactor=0.9)

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


if __name__=="__main__":
    unittest.main()
