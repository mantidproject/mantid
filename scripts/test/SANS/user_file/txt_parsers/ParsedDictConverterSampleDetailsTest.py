# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import ReductionDimensionality
from sans.user_file.settings_tags import SampleId, OtherId, event_binning_string_values
from test.SANS.user_file.txt_parsers.ParsedDictConverterTestCommon import ParsedDictConverterTestCommon


class ParsedDictConverterSampleDetailsTest(ParsedDictConverterTestCommon, unittest.TestCase):

    def test_wide_angle_corrections_on(self):
        self.set_return_val({SampleId.PATH: True})

        returned = self.instance.get_sample_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.wide_angle_corrections)

    def test_wide_angle_corrections_default(self):
        self.set_return_val(None)

        returned = self.instance.get_sample_details()

        self.assertIsNotNone(returned)
        self.assertFalse(returned.wide_angle_corrections)

    def test_z_offset(self):
        expected_val = 1.2

        self.set_return_val({SampleId.OFFSET: expected_val})

        returned = self.instance.get_sample_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.z_offset)

    def test_selected_dimension(self):
        expected_val = ReductionDimensionality.TWO_DIM

        self.set_return_val({OtherId.REDUCTION_DIMENSIONALITY: expected_val})

        returned = self.instance.get_sample_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.selected_dimension)

    def test_event_slices(self):
        expected_val = event_binning_string_values(value="1,2,3")

        self.set_return_val({OtherId.EVENT_SLICES: expected_val})

        returned = self.instance.get_sample_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.event_slices)


if __name__ == '__main__':
    unittest.main()
