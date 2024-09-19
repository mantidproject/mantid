# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.sans_isis.gui_logic.models.file_loading import FileLoading, UserFileLoadException


class FileLoadingTest(unittest.TestCase):
    def test_can_parse_toml_file(self):
        mock_path = mock.NonCallableMock()
        with mock.patch("mantidqtinterfaces.sans_isis.gui_logic.models.file_loading.TomlParser") as mocked_module:
            mocked_parser = mock.Mock()
            mocked_module.return_value = mocked_parser

            file_info = mock.NonCallableMock()
            result = FileLoading.load_user_file(file_path=mock_path, file_information=file_info)
            mocked_parser.parse_toml_file.assert_called_once_with(mock_path, file_information=file_info)
            self.assertEqual(result, mocked_parser.parse_toml_file.return_value)

    def test_wraps_exceptions_toml(self):
        expected_wrapped = [KeyError(), NotImplementedError(), ValueError()]

        for known_exception in expected_wrapped:
            with mock.patch("mantidqtinterfaces.sans_isis.gui_logic.models.file_loading.TomlParser") as mocked_module:
                mocked_parser = mock.Mock()
                mocked_module.return_value = mocked_parser

                mocked_parser.parse_toml_file.side_effect = known_exception
                with self.assertRaises(UserFileLoadException):
                    FileLoading.load_user_file(mock.Mock(), None)

        not_wrapped = [KeyboardInterrupt(), SystemExit(), RuntimeError()]
        for unexpected_exception in not_wrapped:
            with mock.patch("mantidqtinterfaces.sans_isis.gui_logic.models.file_loading.TomlParser") as mocked_module:
                mocked_parser = mock.Mock()
                mocked_module.return_value = mocked_parser
                mocked_parser.parse_toml_file.side_effect = unexpected_exception
                with self.assertRaises(type(unexpected_exception)):
                    FileLoading.load_user_file(mock.Mock(), None)

    def test_wraps_legacy_exceptions(self):
        expected_wrapped = [RuntimeError(), ValueError()]

        for known_exception in expected_wrapped:
            with mock.patch("mantidqtinterfaces.sans_isis.gui_logic.models.file_loading.UserFileReaderAdapter") as mocked_module:
                mocked_parser = mock.Mock()
                mocked_module.return_value = mocked_parser

                mocked_parser.get_all_states.side_effect = known_exception
                with self.assertRaises(UserFileLoadException):
                    FileLoading.load_user_file("legacy.txt", None)

        not_wrapped = [KeyboardInterrupt(), SystemExit()]
        for unexpected_exception in not_wrapped:
            with mock.patch("mantidqtinterfaces.sans_isis.gui_logic.models.file_loading.UserFileReaderAdapter") as mocked_module:
                mocked_parser = mock.Mock()
                mocked_module.return_value = mocked_parser
                mocked_parser.get_all_states.side_effect = unexpected_exception
                with self.assertRaises(type(unexpected_exception)):
                    FileLoading.load_user_file("legacy.txt", None)


if __name__ == "__main__":
    unittest.main()
