# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch
from mantidqtinterfaces.SampleTransmissionCalculator.stc_model import SampleTransmissionCalculatorModel


class SampleTransmissionCalculatorModelTest(unittest.TestCase):
    def setUp(self):
        self.model = SampleTransmissionCalculatorModel()

    def test_calculate_does_not_throw_with_valid_inputs(self):
        input_dict = {
            "binning_type": 0,
            "single_low": 0.0,
            "single_width": 0.1,
            "single_high": 10.0,
            "multiple_bin": "0.0,0.1,10.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        output_dict = self.model.calculate(input_dict)
        self.assertEqual(len(output_dict["x"]), 100)

    @patch("mantidqtinterfaces.SampleTransmissionCalculator.stc_model.CalculateSampleTransmission")
    def test_calculate_uses_single_binning_when_type_set_to_single(self, alg_mock):
        model = SampleTransmissionCalculatorModel()
        input_dict = {
            "binning_type": 0,
            "single_low": 0.0,
            "single_width": 0.1,
            "single_high": 10.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        model.calculate(input_dict)
        alg_mock.assert_called_with(
            WavelengthRange="0.0,0.1,10.0", ChemicalFormula="C", DensityType="Number Density", density=0.1, thickness=0.1
        )

    @patch("mantidqtinterfaces.SampleTransmissionCalculator.stc_model.CalculateSampleTransmission")
    def test_calculate_uses_single_binning_when_type_set_to_multiple(self, alg_mock):
        model = SampleTransmissionCalculatorModel()
        input_dict = {
            "binning_type": 1,
            "single_low": 0.0,
            "single_width": 0.1,
            "single_high": 10.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        model.calculate(input_dict)
        alg_mock.assert_called_with(
            WavelengthRange="0.0,0.1,20.0", ChemicalFormula="C", DensityType="Number Density", density=0.1, thickness=0.1
        )

    def test_validate_correct(self):
        input_dict = {
            "binning_type": 0,
            "single_low": 0.0,
            "single_width": 0.1,
            "single_high": 10.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {})

    def test_validate_histogram_single_zero(self):
        input_dict = {
            "binning_type": 0,
            "single_low": 0.0,
            "single_width": 0.0,
            "single_high": 1.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"histogram": "Histogram must be greater than zero."})

    def test_validate_histogram_single_max_lower_than_min(self):
        input_dict = {
            "binning_type": 0,
            "single_low": 1.0,
            "single_width": 0.0,
            "single_high": 0.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"histogram": "Upper histogram edge must be greater than the lower bin."})

    def test_validate_histogram_width_greater_than_min_single(self):
        input_dict = {
            "binning_type": 0,
            "single_low": 1.0,
            "single_width": 10.0,
            "single_high": 5.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"histogram": "Width cannot be greater than the upper bin."})

    def test_validate_histogram_width_greater_than_min_multiple(self):
        input_dict = {
            "binning_type": 1,
            "single_low": 1.0,
            "single_width": 1.0,
            "single_high": 5.0,
            "multiple_bin": "-1,5,2",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"histogram": "Width cannot be greater than the upper bin."})

    def test_validate_histogram_width_greater_than_min_multiple_different_widths(self):
        input_dict = {
            "binning_type": 1,
            "single_low": 1.0,
            "single_width": 1.0,
            "single_high": 5.0,
            "multiple_bin": "0,1,2,2,3",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"histogram": "Width cannot be greater than the upper bin."})

    def test_validate_histogram_multiple_invalid_string(self):
        input_dict = {
            "binning_type": 1,
            "single_low": 1.0,
            "single_width": 0.1,
            "single_high": 0.0,
            "multiple_bin": "histogram",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"histogram": "Histogram string not readable."})

    def test_validate_histogram_multiple_not_enough_values(self):
        input_dict = {
            "binning_type": 1,
            "single_low": 1.0,
            "single_width": 0.1,
            "single_high": 0.0,
            "multiple_bin": "0.0,0.1",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(
            validation,
            {
                "histogram": "Histogram requires an odd number of values. It uses the same "
                "format as the Rebin algorithm, which is a comma separated list "
                "of first bin boundary, width, last bin boundary."
            },
        )

    def test_validate_no_formula(self):
        input_dict = {
            "binning_type": 0,
            "single_low": 0.0,
            "single_width": 0.1,
            "single_high": 10.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"chemical_formula": "Chemical formula has been left blank."})

    def test_validate_density_zero(self):
        input_dict = {
            "binning_type": 0,
            "single_low": 0.0,
            "single_width": 0.1,
            "single_high": 10.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.0,
            "thickness": 0.1,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"density": "Density can not be zero."})

    def test_validate_thickness_zero(self):
        input_dict = {
            "binning_type": 0,
            "single_low": 0.0,
            "single_width": 0.1,
            "single_high": 10.0,
            "multiple_bin": "0.0,0.1,20.0",
            "chemical_formula": "C",
            "density_type": "Number Density",
            "density": 0.1,
            "thickness": 0.0,
        }
        validation = self.model.validate(input_dict)
        self.assertEqual(validation, {"thickness": "Thickness can not be zero."})


if __name__ == "__main__":
    unittest.main()
