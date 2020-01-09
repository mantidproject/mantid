# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.Containers.FloatRange import FloatRange
from sans.user_file.settings_tags import mask_angle_entry, LimitsId, q_rebin_values, simple_range, range_entry, OtherId
from test.SANS.user_file.txt_parsers.ParsedDictConverterTestCommon import ParsedDictConverterTestCommon


class ParsedDictConverterLimitDetailsTest(ParsedDictConverterTestCommon, unittest.TestCase):
    def test_angle_limit(self):
        expected_val = mask_angle_entry(min=1, max=2, use_mirror=False)
        self.set_return_val({LimitsId.ANGLE: expected_val})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.angle_limit)

    def test_event_binning(self):
        expected_val = "BINNING STRING"
        self.set_return_val({LimitsId.EVENTS_BINNING: expected_val})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.event_binning)

    def test_cut_limit_radius(self):
        expected_val = [(LimitsId.RADIUS_CUT, 1.2)]
        self.set_return_val({LimitsId.RADIUS_CUT: 1.2})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.cut_limit)

    def test_cut_limit_wavelength(self):
        expected_val = [(LimitsId.WAVELENGTH_CUT, 2.4)]
        self.set_return_val({LimitsId.WAVELENGTH_CUT: 2.4})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.cut_limit)

    def test_cut_limit_both(self):
        expected_val = [(LimitsId.RADIUS_CUT, 3.6), (LimitsId.WAVELENGTH_CUT, 4.8)]
        self.set_return_val({LimitsId.RADIUS_CUT: 3.6,
                             LimitsId.WAVELENGTH_CUT: 4.8})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.cut_limit)

    def test_radius_range(self):
        expected_val = FloatRange(1, 2)
        self.set_return_val({LimitsId.RADIUS: range_entry(1, 2)})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.radius_range)

    def test_q_limits(self):
        expected_val = q_rebin_values(1, 2, rebin_string="ABC123")
        self.set_return_val({LimitsId.Q: expected_val})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.q_limits)

    def test_qxy_limit(self):
        expected_val = simple_range(1, 2, step=None, step_type=None)
        self.set_return_val({LimitsId.QXY: expected_val})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.qxy_limit)

    def test_wavelength_limit(self):
        expected_val = simple_range(2, 3, step=None, step_type=None)
        self.set_return_val({LimitsId.WAVELENGTH: expected_val})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.wavelength_limit)

    def test_use_full_wavelength(self):
        self.set_return_val({OtherId.USE_FULL_WAVELENGTH_RANGE: True})

        returned = self.instance.get_limit_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.use_full_wavelength)


if __name__ == '__main__':
    unittest.main()
