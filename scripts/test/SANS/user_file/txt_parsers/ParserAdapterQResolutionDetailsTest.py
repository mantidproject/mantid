# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.user_file.settings_tags import QResolutionId
from user_file.txt_parsers.ParserAdapterTestCommon import ParserAdapterTestCommon


class ParserAdapterQResolutionDetailsTest(ParserAdapterTestCommon, unittest.TestCase):
    def test_use_q_resolution_calculator_on(self):
        self.set_return_val({QResolutionId.ON: True})

        returned = self.instance.get_q_resolution_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.use_q_resolution_calculation)

    def test_use_q_resolution_calculator_default(self):
        self.set_return_val(None)

        returned = self.instance.get_q_resolution_details()

        self.assertIsNotNone(returned)
        self.assertFalse(returned.use_q_resolution_calculation)

    def test_floats_are_forwarded(self):
        to_test_list = ["a1", "a2", "h1", "h2", "w1", "w2", "collimation_length", "delta_r"]

        expected = 1.2
        for attr in to_test_list:
            key = getattr(QResolutionId, attr.upper())
            self.set_return_val({key : expected})

            returned = self.instance.get_q_resolution_details()

            self.assertIsNotNone(returned)
            self.assertEqual(expected, getattr(returned, attr),
                             msg="{0} did not move translate correctly".format(attr))

    def test_moderator_filename(self):
        expected = "abc.xml"
        self.set_return_val({QResolutionId.MODERATOR : expected})

        returned = self.instance.get_q_resolution_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected, returned.moderator_filename)


if __name__ == '__main__':
    unittest.main()
