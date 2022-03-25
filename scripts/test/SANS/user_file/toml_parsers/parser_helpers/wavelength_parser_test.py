# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans.common.enums import RangeStepType
from sans.user_file.parser_helpers.wavelength_parser import parse_range_wavelength, DuplicateWavelengthStates, \
    WavelengthTomlParser


class WavelengthParserTest(unittest.TestCase):
    def test_parse_range_wavelength(self):
        wav_full_range, wav_pairs = parse_range_wavelength("2:2:10")
        # 2-10 in steps of 2
        self.assertEqual((2.0, 10.0), wav_full_range)
        self.assertEqual([(2., 4.), (4., 6.), (6., 8.), (8., 10.), (2., 10.)], wav_pairs)

    def test_parse_range_wavelength_dashes(self):
        wav_full_range, wav_pairs = parse_range_wavelength("2-5, 6-10")
        self.assertEqual((2., 10.), wav_full_range)
        self.assertEqual([(2., 5.), (6., 10.), (2., 10.)], wav_pairs)

        # Should also handle flipped
        wav_full_range, wav_pairs = parse_range_wavelength("6-10, 2-5")
        self.assertEqual((2., 10.), wav_full_range)
        self.assertEqual([(6., 10.), (2., 5.), (2., 10.)], wav_pairs)

    def test_wavelength_multiple_commas(self):
        # This is a legacy input which isn't documented in the current GUI
        # but some scientists still use from the older SANS GUI

        # Equiv to 1-3, 3-5, 5-7, 7-11
        wav_full_range, wav_pairs = parse_range_wavelength("1,3 ,5, 7,11")  # Random spacing intentional
        self.assertEqual((1., 11.), wav_full_range)
        self.assertEqual([(1., 3.), (3., 5.), (5., 7.), (7., 11.), (1., 11.0)], wav_pairs)

    @staticmethod
    def _get_wavelength_objs():
        return DuplicateWavelengthStates(
            mock.NonCallableMock(),
            mock.NonCallableMock(), mock.NonCallableMock())

    def _assert_wav_objs(self, state_objs: DuplicateWavelengthStates,
                         wav_high, wav_low, range_type, step_size=None):
        for i in state_objs.iterate_fields():
            self.assertEqual(wav_high, i.wavelength_interval.wavelength_full_range[1])
            self.assertEqual(wav_low, i.wavelength_interval.wavelength_full_range[0])
            self.assertEqual(range_type, i.wavelength_step_type)
            if step_size:
                self.assertEqual(step_size, i.wavelength_interval.wavelength_step)

    def test_linear_wavelength_setting(self):
        input_dict = {"binning": {"wavelength": {"type": "Lin",
                                                 "start": 1.0,
                                                 "stop": 10.0,
                                                 "step": 2.0}}}
        wav_objs = self._get_wavelength_objs()
        WavelengthTomlParser(input_dict).set_wavelength_details(wav_objs)

        self._assert_wav_objs(wav_objs, wav_low=1.0, wav_high=10.0,
                              range_type=RangeStepType.LIN, step_size=2.0)

    def test_rangelin_wavelength_setting(self):
        input_dict = {"binning": {"wavelength": {"type": "RangeLin",
                                                 "binning": "1-3,4-6",
                                                 "step": "0.5"}}}
        wav_objs = self._get_wavelength_objs()
        WavelengthTomlParser(input_dict).set_wavelength_details(wav_objs)

        self._assert_wav_objs(wav_objs, range_type=RangeStepType.RANGE_LIN,
                              wav_low=1., wav_high=6.)
        self.assertEqual([(1., 3.), (4., 6.), (1., 6.0)],  wav_objs.wavelength.wavelength_interval.selected_ranges)
        self.assertEqual(0.5, wav_objs.wavelength.wavelength_interval.wavelength_step)

    def test_rangelin_wavelength_setting_legacy(self):
        input_dict = {"binning": {"wavelength": {"type": "RangeLog",
                                                 "binning": "1,3,7,9",
                                                 "step": "0.5"}}}
        wav_objs = self._get_wavelength_objs()
        WavelengthTomlParser(input_dict).set_wavelength_details(wav_objs)

        self._assert_wav_objs(wav_objs, range_type=RangeStepType.RANGE_LOG,
                              wav_low=1., wav_high=9.)
        self.assertEqual([(1., 3.), (3., 7.), (7., 9.), (1., 9.)],  wav_objs.wavelength.wavelength_interval.selected_ranges)
        self.assertEqual(0.5, wav_objs.wavelength.wavelength_interval.wavelength_step)

    def test_can_handle_no_wavelength(self):
        input_dict = {"binning": {"another_field": None}}
        self.assertIsNone(WavelengthTomlParser(input_dict)
                          .set_wavelength_details(mock.NonCallableMock()))


if __name__ == '__main__':
    unittest.main()
