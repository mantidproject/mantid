# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.user_file.settings_tags import SampleId
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


if __name__ == '__main__':
    unittest.main()
