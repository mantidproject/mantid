import unittest
from mantid.simpleapi import *
from mantid.api import *


class IndirectCylinderAbsorptionTest(unittest.TestCase):

    def setUp(self):
        """
        Loads the reduced container and sample files.
        """

        can_ws = LoadNexusProcessed(Filename='UsageData/irs26173_graphite002_red.nxs')
        red_ws = LoadNexusProcessed(Filename='UsageData/irs26176_graphite002_red.nxs')

        self._can_ws = can_ws
        self._red_ws = red_ws


    def _test_workspaces(self, corrected, ass):
        """
        Checks the units of the Ass and corrected workspaces.

        @param corrected Corrected workspace
        @param ass Assc corrections workspace
        """

        corrected_x_unit = corrected.getAxis(0).getUnit().unitID()
        self.assertEqual(corrected_x_unit, 'DeltaE')

        ass_x_unit = ass.getAxis(0).getUnit().unitID()
        self.assertEquals(ass_x_unit, 'Wavelength')

        ass_y_unit = ass.YUnitLabel()
        self.assertEqual(ass_y_unit, 'Attenuation factor')


    def test_sample_corrections_only(self):
        """
        Tests corrections for the sample only.
        """

        corrected, ass = IndirectCylinderAbsorption(SampleWorkspace=self._red_ws,
                                                    ChemicalFormula='H2-O',
                                                    SampleRadius=0.2)

        self._test_workspaces(corrected, ass)


    def test_sample_and_can_subtraction(self):
        """
        Tests corrections for the sample and simple container subtraction.
        """

        corrected, ass = IndirectCylinderAbsorption(SampleWorkspace=self._red_ws,
                                                    CanWorkspace=self._can_ws,
                                                    ChemicalFormula='H2-O',
                                                    SampleRadius=0.2)

        self._test_workspaces(corrected, ass)


    def test_sample_and_can_subtraction_with_scale(self):
        """
        Tests corrections for the sample and simple container subtraction
        with can scale.
        """

        corrected, ass = IndirectCylinderAbsorption(SampleWorkspace=self._red_ws,
                                                    CanWorkspace=self._can_ws,
                                                    CanScaleFactor=0.8,
                                                    ChemicalFormula='H2-O',
                                                    SampleRadius=0.2)

        self._test_workspaces(corrected, ass)


if __name__ == '__main__':
    unittest.main()
