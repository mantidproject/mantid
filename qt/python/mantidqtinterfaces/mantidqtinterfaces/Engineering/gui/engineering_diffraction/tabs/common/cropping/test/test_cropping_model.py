# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_model import CroppingModel
from Engineering.common.instrument_config import SUPPORTED_INSTRUMENTS


class CroppingModelTest(unittest.TestCase):
    def setUp(self):
        self.model = CroppingModel()

    def test_validate_single_valid_spectra(self):
        self.assertTrue(self.model.validate_spectrum_numbers("1,2,3,4,5,6,7,8,9,10"))

    def test_validate_single_valid_spectra_regular_whitespace(self):
        self.assertTrue(self.model.validate_spectrum_numbers("1, 2, 3, 4, 5, 6, 7, 8, 9, 10"))

    def test_validate_single_valid_spectra_irregular_whitespace(self):
        self.assertTrue(self.model.validate_spectrum_numbers("1, 2,3,4,   5,6 ,7, 8,   9,  10"))

    def test_validate_single_spectra_invalid_negative(self):
        self.assertFalse(self.model.validate_spectrum_numbers("1,2,3,4,-5,6,7,8,9,10"))

    def test_validate_single_spectra_invalid_spectrum(self):
        self.assertFalse(self.model.validate_spectrum_numbers("1,2,3,4,5,6,77777,8,9,"))

    def test_validate_ranged_spectra(self):
        self.assertTrue(self.model.validate_spectrum_numbers("1-5, 5, 3  , 2-7, 7-13"))

    def test_clean_spectrum_numbers_regular_whitespace(self):
        self.assertEqual(self.model._clean_spectrum_numbers("1, 2, 5, 76, 3"), "1,2,5,76,3")

    def test_clean_spectrum_numbers_irregular_whitespace(self):
        self.assertEqual(self.model._clean_spectrum_numbers("1 , 2, 5      ,    76, 3     "), "1,2,5,76,3")

    def test_clean_spectrum_numbers_regular_ranges(self):
        self.assertEqual(self.model._clean_spectrum_numbers("1-2, 5-76, 3"), "1-2,5-76,3")

    def test_clean_spectrum_numbers_reversed_ranges(self):
        self.assertEqual(self.model._clean_spectrum_numbers("2-1, 76-5, 3"), "1-2,5-76,3")

    def test_clean_spectrum_numbers_equal_range(self):
        self.assertRaisesRegex(
            ValueError, "Ranges cannot contain the same value twice. Invalid Range:*", self.model._clean_spectrum_numbers, "1-1, 76-76, 3"
        )

    def test_validate_and_clean_with_valid_input(self):
        self.assertEqual(self.model.validate_and_clean_spectrum_numbers("1-6, 7-23, 46, 1"), ("", "1-6,7-23,46,1"))

    def test_validate_and_clean_reverse_ranges(self):
        self.assertEqual(self.model.validate_and_clean_spectrum_numbers("6-1, 7-24, 6-4,1"), ("", "1-6,7-24,4-6,1"))

    def test_validate_and_clean_equal_ranges(self):
        self.assertEqual(
            self.model.validate_and_clean_spectrum_numbers("6-6, 7-24, 6-4,1"),
            ("Ranges cannot contain the same value twice. Invalid Range: 6-6", ""),
        )

    def test_get_cropping_options_returns_instrument_specific_options(self):
        for instr in SUPPORTED_INSTRUMENTS:
            options = self.model.get_cropping_options(instr)
            self.assertTrue(len(options) > 0)


if __name__ == "__main__":
    unittest.main()
