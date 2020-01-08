# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.Containers.FloatRange import FloatRange
from sans.common.enums import ReductionMode, CorrectionType
from sans.user_file.settings_tags import DetectorId, range_entry
from test.SANS.user_file.txt_parsers.ParsedDictConverterTestCommon import ParsedDictConverterTestCommon


class ParsedDictConverterDetectorDetailsTest(ParsedDictConverterTestCommon, unittest.TestCase):

    def test_reduction_mode(self):
        expected_val = ReductionMode.ALL
        self.set_return_val({DetectorId.REDUCTION_MODE: ReductionMode.ALL})

        returned = self.instance.get_detector_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.reduction_mode)

    def test_detector_adjustments(self):
        expected_correction_x = float(123)
        expected_correction_y_tilt = 0.1

        self.set_return_val({DetectorId.CORRECTION_X: expected_correction_x,
                             DetectorId.CORRECTION_Y_TILT: expected_correction_y_tilt})

        returned = self.instance.get_detector_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_correction_x, returned.detector_adjustment[CorrectionType.X])
        self.assertEqual(expected_correction_y_tilt, returned.detector_adjustment[CorrectionType.Y_TILT])

    def test_merge_rescale(self):
        expected_val = 2.1
        self.set_return_val({DetectorId.RESCALE: expected_val})

        returned = self.instance.get_detector_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.merge_rescale)

    def test_merge_shift(self):
        expected_val = 2.3
        self.set_return_val({DetectorId.SHIFT: expected_val})

        returned = self.instance.get_detector_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.merge_shift)

    def test_merge_rescale_fit(self):
        expected_range = FloatRange(3.1, 4.1)
        self.set_return_val({DetectorId.RESCALE_FIT: range_entry(3.1, 4.1)})

        returned = self.instance.get_detector_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_range, returned.merge_fitted_rescale)

    def test_merge_shift_fit(self):
        expected_range = FloatRange(3.2, 4.2)
        self.set_return_val({DetectorId.SHIFT_FIT: range_entry(3.2, 4.2)})

        returned = self.instance.get_detector_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_range, returned.merge_fitted_shift)


if __name__ == '__main__':
    unittest.main()
