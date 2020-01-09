# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.user_file.settings_tags import OtherId
from test.SANS.user_file.txt_parsers.ParsedDictConverterTestCommon import ParsedDictConverterTestCommon


class ParsedDictConverterSaveDetailsTest(ParsedDictConverterTestCommon, unittest.TestCase):
    def test_save_as_zero_error(self):
        self.set_return_val({OtherId.SAVE_AS_ZERO_ERROR_FREE: True})

        returned = self.instance.get_save_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.save_as_zero_error_free)

    def test_use_reduction_mode_as_suffix(self):
        self.set_return_val({OtherId.USE_REDUCTION_MODE_AS_SUFFIX: True})

        returned = self.instance.get_save_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.use_reduction_mode_as_suffix)

    def test_all_other_attrs_transposed(self):
        attr_map = {OtherId.SAVE_TYPES: "selected_save_algs",
                    OtherId.USER_SPECIFIED_OUTPUT_NAME: "user_output_name",
                    OtherId.USER_SPECIFIED_OUTPUT_NAME_SUFFIX: "user_output_suffix"}

        for old_key, new_attr in attr_map.items():
            expected_value = "ABC"
            self.set_return_val({old_key: expected_value})

            returned = self.instance.get_save_details()

            self.assertIsNotNone(returned)
            self.assertEqual(expected_value, getattr(returned, new_attr),
                             msg="{0} did not translate correctly".format(new_attr))


if __name__ == '__main__':
    unittest.main()
