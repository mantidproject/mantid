import unittest
import numpy as np
import mantid.simpleapi
from mantid.simpleapi import CalculateSampleTransmission


class CalculateSampleTransmissionTest(unittest.TestCase):

    def test_sample_transmission_calculation(self):
        """
        Test a basic transmission calculation.
        """

        # Using water sample
        formula = "H2-O"
        density = 0.1
        thickness = 0.1

        ws = CalculateSampleTransmission(WavelengthRange='5.0,0.2,7.0', ChemicalFormula=formula,
                                         NumberDensity=density, Thickness=thickness)

        self.assertEqual(ws.getNumberHistograms(), 2)

        expected_trans = [0.564272, 0.564022, 0.563772, 0.563522, 0.563272, 0.563022, 0.562772, 0.562523, 0.562273, 0.562024]
        expected_scatt = [0.429309, 0.429309, 0.429309, 0.429309, 0.429309, 0.429309, 0.429309, 0.429309, 0.429309, 0.429309]
        trans = ws.readY(0)
        scatt = ws.readY(1)

        np.testing.assert_array_almost_equal(trans, expected_trans, decimal=4)
        np.testing.assert_array_almost_equal(scatt, expected_scatt, decimal=4)


    def test_validate_density(self):
        """
        Tests validation on NumberDensity property.
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


if __name__=="__main__":
    unittest.main()
