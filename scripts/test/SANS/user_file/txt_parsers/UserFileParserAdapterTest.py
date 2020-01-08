# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import mock

from sans.common.enums import ReductionMode
from sans.user_file.settings_tags import BackId, DetectorId, range_entry
from sans.user_file.txt_parsers.UserFileParserAdapter import UserFileParserAdapter
from sans.user_file.user_file_reader import UserFileReader


class UserFileParserAdapterTest(unittest.TestCase):
    def setUp(self) -> None:
        self.mocked_adapter = mock.Mock(autospec=UserFileReader)
        self.instance = UserFileParserAdapter(filename="ABC", txt_user_file_reader=self.mocked_adapter)

    def test_handles_none_being_passed(self):
        self.assertIsNotNone(UserFileParserAdapter(filename="ABC"))

    def test_parses_on_creation(self):
        self.instance.get_tube_calibration_filename()
        self.mocked_adapter.read_user_file.assert_called_once()

        # Should not parse again
        self.instance.get_transmission_details()
        self.mocked_adapter.read_user_file.assert_called_once()

    def test_multiple_gets_packed_properly(self):
        return_val = {BackId.ALL_MONITORS: range_entry(1, 2),  # From background parser
                      DetectorId.REDUCTION_MODE: ReductionMode.MERGED  # From detector ID parser
                      }
        self.mocked_adapter.read_user_file.return_value = return_val

        self.assertIsNotNone(self.instance.get_background_details())
        self.assertIsNotNone(self.instance.get_detector_details())

        self.assertIsNotNone(self.instance.get_fit_details())


if __name__ == '__main__':
    unittest.main()
