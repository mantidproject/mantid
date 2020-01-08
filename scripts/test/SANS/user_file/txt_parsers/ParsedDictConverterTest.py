# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import SANSInstrument
from sans.user_file.settings_tags import DetectorId, GravityId, TubeCalibrationFileId, OtherId
from test.SANS.user_file.txt_parsers.ParsedDictConverterTestCommon import ParsedDictConverterTestCommon


class ParsedDictConverterConverterTest(ParsedDictConverterTestCommon, unittest.TestCase):

    def test_missing_vals_returns_something(self):
        self.set_return_val({})
        self.assertIsNotNone(self.instance.get_fit_details())

    # Interface definitions which do not return objects tested below
    def test_get_gravity_on_off(self):
        self.set_return_val({GravityId.ON_OFF: True})

        returned = self.instance.get_gravity_details()
        self.assertIsNotNone(returned)
        self.assertTrue(returned[0])
        self.assertEqual(0, returned[1])

    def test_get_instrument(self):
        expected_val = SANSInstrument.LARMOR
        self.set_return_val({DetectorId.INSTRUMENT: SANSInstrument.LARMOR})

        returned = self.instance.get_instrument()
        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned)

    def test_no_instrument_handled(self):
        expected_val = SANSInstrument.NO_INSTRUMENT
        self.set_return_val({})

        returned = self.instance.get_instrument()
        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned)

    def test_no_instrument_forwarded(self):
        expected_val = SANSInstrument.NO_INSTRUMENT
        self.set_return_val({DetectorId.INSTRUMENT: SANSInstrument.NO_INSTRUMENT})

        returned = self.instance.get_instrument()
        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned)

    def test_get_tube_calibration_filename(self):
        expected_val = "tube.xml"
        self.set_return_val({TubeCalibrationFileId.FILE: expected_val})

        returned = self.instance.get_tube_calibration_filename()
        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned)

    def test_is_compatibility_mode_on(self):
        self.set_return_val({OtherId.USE_COMPATIBILITY_MODE: True})
        returned = self.instance.is_compatibility_mode_on()
        self.assertTrue(returned)

    def test_is_compatibility_mode_default(self):
        self.set_return_val(None)
        returned = self.instance.is_compatibility_mode_on()
        self.assertFalse(returned)


if __name__ == '__main__':
    unittest.main()
