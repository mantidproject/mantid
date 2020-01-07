# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.Containers.FloatRange import FloatRange
from sans.common.enums import ReductionMode, SANSInstrument
from sans.user_file.settings_tags import BackId, DetectorId, GravityId, TubeCalibrationFileId, OtherId
from sans.user_file.txt_parsers.ParserAdapter import ParserAdapter
from user_file.txt_parsers.ParserAdapterTestCommon import ParserAdapterTestCommon


class ParserAdapterTestTest(ParserAdapterTestCommon, unittest.TestCase):

    def test_handles_none_being_passed(self):
        self.assertIsNotNone(ParserAdapter(filename="ABC"))

    def test_parses_on_creation(self):
        self.set_return_val(None)
        self.mocked_adapted.read_user_file.assert_called_once()

    def test_missing_vals_return_none(self):
        self.set_return_val({})
        self.assertIsNone(self.instance.get_fit_details())

    def test_multiple_gets_packed_properly(self):
        return_val = {BackId.ALL_MONITORS: FloatRange(1, 2),  # From background parser
                      DetectorId.REDUCTION_MODE: ReductionMode.MERGED  # From detector ID parser
                      }
        self.mocked_adapted.read_user_file.return_value = return_val

        self.assertIsNotNone(self.instance.get_background_details())
        self.assertIsNotNone(self.instance.get_detector_details())

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
