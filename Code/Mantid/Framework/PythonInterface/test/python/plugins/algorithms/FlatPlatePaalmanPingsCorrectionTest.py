import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import CreateSampleWorkspace, Scale, DeleteWorkspace, FlatPlatePaalmanPingsCorrection


class FlatPlatePaalmanPingsCorrectionTest(unittest.TestCase):

    def setUp(self):
        """
        Create sample workspaces.
        """

        sample = CreateSampleWorkspace(NumBanks=5,
                                       BankPixelWidth=2,
                                       XUnit='Wavelength',
                                       XMin=6.8,
                                       XMax=7.9,
                                       BinWidth=0.1)
        self._sample_ws = sample

        can = Scale(InputWorkspace=sample, Factor=1.2)
        self._can_ws = can

        self._corrections_ws_name = 'corrections'


    def tearDown(self):
        """
        Remove workspaces from ADS.
        """

        DeleteWorkspace(self._sample_ws)
        DeleteWorkspace(self._can_ws)

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

        # Check it has X binning matching NumWavelengths
        self.assertEqual(test_ws.blocksize(), 10)


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


    def test_sampleOnly(self):
        """
        Test simple run with sample workspace only.
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
        self. _verify_workspace(ass_ws_name)


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


if __name__=="__main__":
    unittest.main()
