from __future__ import absolute_import, division, print_function

import unittest
from mantid.simpleapi import FlatPlateMonteCarloAbsorption, LoadNexusProcessed, DeleteWorkspace, ConvertUnits
from mantid.api import *


class FlatPlateMonteCarloAbsorptionTest(unittest.TestCase):
    def setUp(self):
        """
        Loads the reduced sample and container files
        """

        red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')
        red_ws = ConvertUnits(InputWorkspace=red_ws, Target="Wavelength", EMode="Indirect", EFixed=1.845)

        self._red_ws = red_ws

    def _test_corrections_workspace(self, ws):
        x_unit = ws.getAxis(0).getUnit().unitID()
        self.assertEquals(x_unit, 'Wavelength')

        y_unit = ws.YUnitLabel()
        self.assertEquals(y_unit, 'Attenuation factor')

        num_hists = ws.getNumberHistograms()
        self.assertEquals(num_hists, 10)


    def test_basic(self):
        corrections = FlatPlateMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                  ChemicalFormula='H2-O',
                                                  EventsPerPoint=200)

        self._test_corrections_workspace(corrections)

    def test_beam_dimensions(self):
        corrections = FlatPlateMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                  ChemicalFormula='H2-O',
                                                  EventsPerPoint=200,
                                                  BeamHeight=2,
                                                  BeamWidth=3)

        self._test_corrections_workspace(corrections)

    def test_number_density(self):
        """
        Number density of water is about 0.033428
        This should produce the same results as the mass density test
        """
        corrections = FlatPlateMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                  ChemicalFormula='H2-O',
                                                  EventsPerPoint=200,
                                                  DensityType="Number Density",
                                                  Density=0.033428)

        self._test_corrections_workspace(corrections)

        self.assertEqual(round(corrections.readY(0)[0], 6), 0.372866)
        self.assertEqual(round(corrections.readY(1)[0], 6), 0.3579953)

    def test_mass_density(self):
        """
        Mass density of water is 1.0 g/cm^3
        This should produce the same result as the number density test
        """
        corrections = FlatPlateMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                  ChemicalFormula='H2-O',
                                                  EventsPerPoint=200,
                                                  DensityType="Mass Density",
                                                  Density=1.0)

        self._test_corrections_workspace(corrections)

        self.assertEqual(round(corrections.readY(0)[0], 6), 0.372866)
        self.assertEqual(round(corrections.readY(1)[0], 6), 0.3579953)

    def test_not_in_wavelength(self):
        red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')

        self.assertRaises(RuntimeError,
                          FlatPlateMonteCarloAbsorption,
                          InputWorkspace=red_ws,
                          ChemicalFormula='H2-O',
                          EventsPerPoint=200,
                          DensityType="Mass Density",
                          Density=0.5)

        DeleteWorkspace(red_ws)

if __name__ == '__main__':
    unittest.main()
