# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from mantid.simpleapi import CalculateSampleTransmission


class CalculateSampleTransmissionTest(unittest.TestCase):
    def test_sample_transmission_calculation(self):
        """
        Test a basic transmission calculation using number density.
        """

        # Using water sample
        formula = "H2-O"
        density = 0.100272
        thickness = 0.1

        ws = CalculateSampleTransmission(WavelengthRange='5.0,0.2,7.0', ChemicalFormula=formula,
                                         Density=density, Thickness=thickness, DensityType='Number Density')

        self.assertEqual(ws.getNumberHistograms(), 2)

        expected_trans = [0.56619985, 0.56605978, 0.56591975, 0.56577975, 0.56563978, 0.56549985, 0.56535996, 0.56522009, 0.56508027, 0.56494047]
        expected_scatt = [0.43017862, 0.43017862, 0.43017862, 0.43017862, 0.43017862, 0.43017862, 0.43017862, 0.43017862, 0.43017862, 0.43017862]
        trans = ws.readY(0)
        scatt = ws.readY(1)

        np.testing.assert_array_almost_equal(trans, expected_trans, decimal=4)
        np.testing.assert_array_almost_equal(scatt, expected_scatt, decimal=4)

    def test_mass_density(self):
        """
        Tests a transmission calculation using mass density
        """
        formula = "H2-O"
        density = 1
        thickness = 0.1

        ws = CalculateSampleTransmission(WavelengthRange='5.0,0.2,7.0', ChemicalFormula=formula,
                                         Density=density, Thickness=thickness, DensityType='Mass Density')

        self.assertEqual(ws.getNumberHistograms(), 2)

        expected_trans = [0.56619985, 0.56605978, 0.56591975, 0.56577975, 0.56563978, 0.56549985, 0.56535996, 0.56522009, 0.56508027, 0.56494047]
        expected_scatt = [0.43021665, 0.43021665, 0.43021665, 0.43021665, 0.43021665, 0.43021665, 0.43021665, 0.43021665, 0.43021665, 0.43021665]

        trans = ws.readY(0)
        scatt = ws.readY(1)
        np.testing.assert_array_almost_equal(trans, expected_trans, decimal=4)
        np.testing.assert_array_almost_equal(scatt, expected_scatt, decimal=4)

    def test_validate_density(self):
        """
        Tests validation on Density property.
        """

        # Using water sample
        formula = "H2-O"
        density = -0.1
        thickness = 0.1

        self.assertRaises(RuntimeError, CalculateSampleTransmission,
                          WavelengthRange='5.0,0.2,7.0', ChemicalFormula=formula,
                          NumberDensity=density, Thickness=thickness)

    def test_validate_thickness(self):
        """
        Tests validation on Thickness property.
        """

        # Using water sample
        formula = "H2-O"
        density = 0.1
        thickness = -0.1

        self.assertRaises(RuntimeError, CalculateSampleTransmission,
                          WavelengthRange='5.0,0.2,7.0', ChemicalFormula=formula,
                          NumberDensity=density, Thickness=thickness)


if __name__ == "__main__":
    unittest.main()
