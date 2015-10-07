import unittest
from mantid.simpleapi import *
from mantid.api import *


class IndirectCylinderAbsorptionTest(unittest.TestCase):

    def setUp(self):
        """
        Loads the reduced container and sample files.
        """

        can_ws = LoadNexusProcessed(Filename='irs26173_graphite002_red.nxs')
        red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')

        self._can_ws = can_ws
        self._red_ws = red_ws


    def _test_workspaces(self, corrected, factor_group):
        """
        Checks the units of the Ass and corrected workspaces.

        @param corrected Corrected workspace
        @param factor_group WorkspaceGroup containing factors
        """

        # Test units of corrected workspace
        corrected_x_unit = corrected.getAxis(0).getUnit().unitID()
        self.assertEqual(corrected_x_unit, 'DeltaE')

        # Test units of factor workspaces
        for ws in factor_group:
            x_unit = ws.getAxis(0).getUnit().unitID()
            self.assertEquals(x_unit, 'Wavelength')

            y_unit = ws.YUnitLabel()
            self.assertEqual(y_unit, 'Attenuation factor')


    def test_sample_corrections_only(self):
        """
        Tests corrections for the sample only.
        """

        corrected, fact = IndirectCylinderAbsorption(SampleWorkspace=self._red_ws,
                                                     SampleChemicalFormula='H2-O',
                                                     Events=500)

        self.assertEqual(fact.size(), 1)
        self._test_workspaces(corrected, fact)


    def test_sample_and_can_subtraction(self):
        """
        Tests corrections for the sample and simple container subtraction.
        """

        corrected, fact = IndirectCylinderAbsorption(SampleWorkspace=self._red_ws,
                                                     CanWorkspace=self._can_ws,
                                                     SampleChemicalFormula='H2-O',
                                                     UseCanCorrections=False,
                                                     Events=500)

        self.assertEqual(fact.size(), 1)
        self._test_workspaces(corrected, fact)


    def test_sample_and_can_subtraction_with_scale(self):
        """
        Tests corrections for the sample and simple container subtraction
        with can scale.
        """

        corrected, fact = IndirectCylinderAbsorption(SampleWorkspace=self._red_ws,
                                                     CanWorkspace=self._can_ws,
                                                     CanScaleFactor=0.8,
                                                     SampleChemicalFormula='H2-O',
                                                     UseCanCorrections=False,
                                                     Events=500)

        self.assertEqual(fact.size(), 1)
        self._test_workspaces(corrected, fact)


    def test_sample_and_can_corrections(self):
        """
        Tests corrections for the sample and container.
        """

        corrected, fact = IndirectCylinderAbsorption(SampleWorkspace=self._red_ws,
                                                     CanWorkspace=self._can_ws,
                                                     SampleChemicalFormula='H2-O',
                                                     CanChemicalFormula='V',
                                                     UseCanCorrections=True,
                                                     Events=500)

        self.assertEqual(fact.size(), 2)
        self._test_workspaces(corrected, fact)


if __name__ == '__main__':
    unittest.main()
