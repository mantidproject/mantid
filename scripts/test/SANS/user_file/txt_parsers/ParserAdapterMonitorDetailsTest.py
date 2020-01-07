# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import DetectorType
from sans.user_file.settings_tags import MonId, monitor_file, monitor_length, monitor_spectrum
from user_file.txt_parsers.ParserAdapterTestCommon import ParserAdapterTestCommon


class ParserAdapterMonitorDetailsTest(ParserAdapterTestCommon, unittest.TestCase):

    def test_direct_filename(self):
        input_val = "ABC.xml"
        for det in [DetectorType.LAB, DetectorType.LAB]:
            expected_val = {det: input_val}

            self.set_return_val({MonId.DIRECT: monitor_file(input_val, det)})

            returned = self.instance.get_monitor_details()

            self.assertIsNotNone(returned)
            self.assertEqual(expected_val, returned.direct_filename)

    def test_flood_source_filename(self):
        input_val = "flood_source.xml"
        for det in [DetectorType.LAB, DetectorType.LAB]:
            expected_val = {det: input_val}

            self.set_return_val({MonId.FLAT: monitor_file(input_val, det)})
            returned = self.instance.get_monitor_details()

            self.assertIsNotNone(returned)
            self.assertEqual(expected_val, returned.flood_source_filename)

    def test_monitor_pos_from_moderator(self):
        input_val = monitor_length(length=1, spectrum=2, interpolate=False)
        expected = [input_val]

        self.set_return_val({MonId.LENGTH: input_val})
        returned = self.instance.get_monitor_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected, returned.monitor_pos_from_moderator)

    def test_selected_spectrum(self):
        expected_val = monitor_spectrum(spectrum=123, is_trans=False, interpolate=False)

        self.set_return_val({MonId.SPECTRUM: expected_val})
        returned = self.instance.get_monitor_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.selected_spectrum)


if __name__ == '__main__':
    unittest.main()
