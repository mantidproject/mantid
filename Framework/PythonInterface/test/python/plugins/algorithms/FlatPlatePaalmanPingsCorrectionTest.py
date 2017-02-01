from __future__ import (absolute_import, division, print_function)

import unittest
from mantid import mtd, config
from mantid.simpleapi import CreateSampleWorkspace, Scale, DeleteWorkspace, ConvertToPointData, \
                             SetInstrumentParameter, FlatPlatePaalmanPingsCorrection


class FlatPlatePaalmanPingsCorrectionTest(unittest.TestCase):
    def setUp(self):
        """
        Create sample workspaces.
        """

        # Create some test data
        sample = CreateSampleWorkspace(NumBanks=1,
                                       BankPixelWidth=1,
                                       XUnit='Wavelength',
                                       XMin=6.8,
                                       XMax=7.9,
                                       BinWidth=0.1)
        self._sample_ws = sample

        can = Scale(InputWorkspace=sample, Factor=1.2)
        self._can_ws = can

        # Create empty test data not in wavelength
        sample_empty_unit = CreateSampleWorkspace(NumBanks=1,
                                                  BankPixelWidth=1,
                                                  XUnit='Empty',
                                                  XMin=6.8,
                                                  XMax=7.9,
                                                  BinWidth=0.1)

        SetInstrumentParameter(Workspace=sample_empty_unit,
                               ParameterName='Efixed',
                               ParameterType='Number',
                               Value='5.')

        self._sample_empty_unit = sample_empty_unit

        empty_unit_point = ConvertToPointData(sample_empty_unit)

        self._empty_unit_point = empty_unit_point

        self._corrections_ws_name = 'corrections'

    def tearDown(self):
        """
        Remove workspaces from ADS.
        """

        DeleteWorkspace(self._sample_ws)
        DeleteWorkspace(self._can_ws)
        DeleteWorkspace(self._sample_empty_unit)
        DeleteWorkspace(self._empty_unit_point)

        if self._corrections_ws_name in mtd:
            DeleteWorkspace(self._corrections_ws_name)

    def _verify_workspace(self, ws_name):
        """
        Do validation on a correction workspace.

        @param ws_name Name of workspace to validate
        """

        corrections_ws = mtd[self._corrections_ws_name]

        # Check it is in the corrections workspace group
        self.assertTrue(corrections_ws.contains(ws_name))

        test_ws = mtd[ws_name]

        # Check workspace is in wavelength
        self.assertEqual(test_ws.getAxis(0).getUnit().unitID(),
                         'Wavelength')

        # Check it has the same number of spectra as the sample
        self.assertEqual(test_ws.getNumberHistograms(),
                         self._sample_ws.getNumberHistograms())

        # Check it has X binning matching sample workspace
        self.assertEqual(test_ws.blocksize(), self._sample_ws.blocksize())

    def _verify_workspaces_for_can(self):
        """
        Do validation on the additional correction factors for sample and can.
        """

        ass_ws_name = self._corrections_ws_name + '_ass'
        assc_ws_name = self._corrections_ws_name + '_assc'
        acsc_ws_name = self._corrections_ws_name + '_acsc'
        acc_ws_name = self._corrections_ws_name + '_acc'

        workspaces = [ass_ws_name, assc_ws_name, acsc_ws_name, acc_ws_name]

        for workspace in workspaces:
            self._verify_workspace(workspace)

    def test_sampleOnly_indirect(self):
        """
        Test simple run with sample workspace only for indirect mode
        """

        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._sample_ws,
                                        SampleChemicalFormula='H2-O',
                                        SampleThickness=0.1,
                                        SampleAngle=45,
                                        NumberWavelengths=10,
                                        Emode='Indirect',
                                        Efixed=1.845)

        ass_ws_name = self._corrections_ws_name + '_ass'
        self._verify_workspace(ass_ws_name)

    def test_sampleOnly_direct(self):
        """
        Test simple run with sample workspace only for direct mode
        """

        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._sample_ws,
                                        SampleChemicalFormula='H2-O',
                                        SampleThickness=0.1,
                                        SampleAngle=45,
                                        NumberWavelengths=10,
                                        Emode='Direct',
                                        Efixed=1.845)

        ass_ws_name = self._corrections_ws_name + '_ass'
        self._verify_workspace(ass_ws_name)

    def test_sampleAndCan(self):
        """
        Test simple run with sample and can workspace.
        """

        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._sample_ws,
                                        SampleChemicalFormula='H2-O',
                                        SampleThickness=0.1,
                                        SampleAngle=45,
                                        CanWorkspace=self._can_ws,
                                        CanChemicalFormula='V',
                                        CanFrontThickness=0.01,
                                        CanBackThickness=0.01,
                                        NumberWavelengths=10,
                                        Emode='Indirect',
                                        Efixed=1.845)

        self._verify_workspaces_for_can()

    def test_sampleAndCanDefaults(self):
        """
        Test simple run with sample and can workspace using the default values.
        """

        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._sample_ws,
                                        SampleChemicalFormula='H2-O',
                                        CanWorkspace=self._can_ws,
                                        CanChemicalFormula='V')

        self._verify_workspaces_for_can()

    def test_number_density_for_sample_can(self):
        """
        Test simple run with sample and can workspace and number density for both
        """

        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._sample_ws,
                                        SampleChemicalFormula='H2-O',
                                        SampleDensityType='Number Density',
                                        SampleDensity=0.5,
                                        CanWorkspace=self._can_ws,
                                        CanChemicalFormula='V',
                                        CanDensityType='Number Density',
                                        CanDensity=0.5)
        self._verify_workspaces_for_can()

    def test_mass_density_for_sample_can(self):
        """
        Test simple run with sample and can workspace and mass density for both
        """

        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._sample_ws,
                                        SampleChemicalFormula='H2-O',
                                        SampleDensityType='Mass Density',
                                        SampleDensity=0.5,
                                        CanWorkspace=self._can_ws,
                                        CanChemicalFormula='V',
                                        CanDensityType='Mass Density',
                                        CanDensity=0.5)

        self._verify_workspaces_for_can()

    def test_InterpolateDisabled(self):
        """
        Tests that a workspace with a bin count equal to NumberWavelengths is created
        when interpolation is disabled.
        """

        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._sample_ws,
                                        SampleChemicalFormula='H2-O',
                                        CanWorkspace=self._can_ws,
                                        CanChemicalFormula='V',
                                        NumberWavelengths=20,
                                        Interpolate=False)

        corrections_ws = mtd[self._corrections_ws_name]

        # Check each correction workspace has X binning matching NumberWavelengths
        for workspace in corrections_ws:
            self.assertEqual(workspace.blocksize(), 20)

    def test_validationNoCanFormula(self):
        """
        Tests validation for no chemical formula for can when a can WS is provided.
        """

        self.assertRaises(RuntimeError,
                          FlatPlatePaalmanPingsCorrection,
                          OutputWorkspace=self._corrections_ws_name,
                          SampleWorkspace=self._sample_ws,
                          SampleChemicalFormula='H2-O',
                          SampleThickness=0.1,
                          SampleAngle=45,
                          CanWorkspace=self._can_ws,
                          CanFrontThickness=0.1,
                          CanBackThickness=0.1,
                          NumberWavelengths=10,
                          Emode='Indirect',
                          Efixed=1.845)

    def test_efixed(self):
        """
        Tests in the EFixed mode
        """
        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._sample_empty_unit,
                                        SampleChemicalFormula='H2-O',
                                        Emode='Efixed')

        for workspace in mtd[self._corrections_ws_name]:
            self.assertEqual(workspace.blocksize(), 1)
            run = workspace.getRun()
            self.assertEqual(run.getLogData('emode').value,'Efixed')
            self.assertAlmostEqual(run.getLogData('efixed').value, 5.)

    def test_efixed_override(self):
        """
        Tests in the Efixed mode with overridden Efixed value for point data
        """
        FlatPlatePaalmanPingsCorrection(OutputWorkspace=self._corrections_ws_name,
                                        SampleWorkspace=self._empty_unit_point,
                                        SampleChemicalFormula='H2-O',
                                        Emode='Efixed',
                                        Efixed=7.5)

        for workspace in mtd[self._corrections_ws_name]:
            self.assertEqual(workspace.blocksize(), 1)
            run = workspace.getRun()
            self.assertEqual(run.getLogData('emode').value,'Efixed')
            self.assertAlmostEqual(run.getLogData('efixed').value, 7.5)

if __name__ == "__main__":
    unittest.main()
