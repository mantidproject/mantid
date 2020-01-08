# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.Containers.FloatRange import FloatRange
from sans.common.Containers.MonitorID import MonitorID
from sans.user_file.settings_tags import BackId, back_single_monitor_entry, range_entry
from test.SANS.user_file.txt_parsers.ParsedDictConverterTestCommon import ParsedDictConverterTestCommon


class ParsedDictConverterBackgroundDetailsTest(ParsedDictConverterTestCommon, unittest.TestCase):
    def test_tof_window_all_monitors(self):
        expected_float_range = FloatRange(100, 200)
        self.set_return_val({BackId.ALL_MONITORS: range_entry(100, 200)})

        returned = self.instance.get_background_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_float_range, returned.tof_window_all_monitors)

    def test_transmission_tof_range(self):
        expected_float_range = FloatRange(200, 300)
        self.set_return_val({BackId.TRANS: range_entry(200, 300)})

        returned = self.instance.get_background_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_float_range, returned.transmission_tof_range)

    def test_monitors_with_background_off(self):
        expected_monitor_list = [MonitorID(monitor_spec_num=123)]
        self.set_return_val({BackId.MONITOR_OFF: 123})

        returned = self.instance.get_background_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_monitor_list, returned.monitors_with_background_off)

    def test_tof_window_single_monitor(self):
        expected_val = [(MonitorID(234), FloatRange(1.2, 2.4))]
        self.set_return_val({BackId.SINGLE_MONITORS: back_single_monitor_entry(monitor=234, start=1.2, stop=2.4)})

        returned = self.instance.get_background_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.tof_window_single_monitor)


if __name__ == '__main__':
    unittest.main()
