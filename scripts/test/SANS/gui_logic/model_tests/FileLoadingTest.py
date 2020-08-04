# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from sans.gui_logic.models.file_loading import FileLoading


class FileLoadingTest(unittest.TestCase):
    def test_can_parse_toml_file(self):
        mock_path = mock.NonCallableMock()
        with mock.patch("sans.gui_logic.models.file_loading.TomlParser") as mocked_module:
            mocked_parser = mock.Mock()
            mocked_module.return_value = mocked_parser

            file_info = mock.NonCallableMock()
            result = FileLoading.load_user_file(file_path=mock_path, file_information=file_info)
            mocked_parser.parse_toml_file.assert_called_once_with(mock_path, file_information=file_info)
            self.assertEquals(result, mocked_parser.parse_toml_file.return_value)


if __name__ == '__main__':
    unittest.main()
