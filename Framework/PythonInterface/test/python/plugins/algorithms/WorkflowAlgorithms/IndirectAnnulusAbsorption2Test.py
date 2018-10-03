# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import LoadNexusProcessed, IndirectAnnulusAbsorption, DeleteWorkspace
from mantid.api import *


class IndirectAnnulusAbsorption2Test(unittest.TestCase):
    def setUp(self):
        """
        Loads the reduced container and sample files.
        """

        can_ws = LoadNexusProcessed(Filename='irs26173_graphite002_red.nxs')
        red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')

        self._can_ws = can_ws
        self._red_ws = red_ws

    def tearDown(self):
        """
        Removes sample workspaces.
        """
        DeleteWorkspace(self._can_ws)
        DeleteWorkspace(self._red_ws)

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

        corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=self._red_ws,
                                                    SampleChemicalFormula='H2-O',
                                                    Events=200,
                                                    UseCanCorrections=False)

        self.assertEqual(fact.size(), 1)
        self._test_workspaces(corrected, fact)

    def test_beam_dimensions(self):
        """
        Tests beam dimensions
        """

        corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=self._red_ws,
                                                    SampleChemicalFormula='H2-O',
                                                    NumberWavelengths=10,
                                                    Events=200,
                                                    BeamHeight=2,
                                                    BeamWidth=3)

        self.assertEqual(fact.size(), 1)
        self._test_workspaces(corrected, fact)

    def test_sample_and_can_subtraction(self):
        """
        Tests corrections for the sample and simple container subtraction.
        """

        corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=self._red_ws,
                                                    SampleChemicalFormula='H2-O',
                                                    CanWorkspace=self._can_ws,
                                                    Events=200,
                                                    UseCanCorrections=False)

        self.assertEqual(fact.size(), 1)
        self._test_workspaces(corrected, fact)

    def test_sample_and_can_subtraction_with_scale(self):
        """
        Tests corrections for the sample and simple container subtraction
        with can scale.
        """

        corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=self._red_ws,
                                                    SampleChemicalFormula='H2-O',
                                                    CanWorkspace=self._can_ws,
                                                    CanScaleFactor=0.8,
                                                    Events=200,
                                                    UseCanCorrections=False)

        self.assertEqual(fact.size(), 1)
        self._test_workspaces(corrected, fact)

    def test_sample_and_can_corrections(self):
        """
        Tests corrections for the sample and container.
        """

        corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=self._red_ws,
                                                    SampleChemicalFormula='H2-O',
                                                    CanWorkspace=self._can_ws,
                                                    CanChemicalFormula='V',
                                                    CanScaleFactor=0.8,
                                                    Events=200,
                                                    UseCanCorrections=True)

        self.assertEqual(fact.size(), 2)
        self._test_workspaces(corrected, fact)

    def test_number_density_for_sample_can(self):
        """
        Test simple run with sample and can workspace and number density for both
        """

        corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=self._red_ws,
                                                    SampleChemicalFormula='H2-O',
                                                    SampleDensityType='Number Density',
                                                    SampleDensity=0.5,
                                                    CanWorkspace=self._can_ws,
                                                    CanChemicalFormula='V',
                                                    CanDensityType='Number Density',
                                                    CanDensity=0.5,
                                                    Events=200,
                                                    UseCanCorrections=True)

        self.assertEqual(fact.size(), 2)
        self._test_workspaces(corrected, fact)

    def test_mass_density_for_sample_can(self):
        """
        Test simple run with sample and can workspace and number density for both
        """

        corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=self._red_ws,
                                                    SampleChemicalFormula='H2-O',
                                                    SampleDensityType='Mass Density',
                                                    SampleDensity=0.5,
                                                    CanWorkspace=self._can_ws,
                                                    CanChemicalFormula='V',
                                                    CanDensityType='Mass Density',
                                                    CanDensity=0.5,
                                                    Events=200,
                                                    UseCanCorrections=True)

        self.assertEqual(fact.size(), 2)
        self._test_workspaces(corrected, fact)


if __name__ == '__main__':
    unittest.main()
