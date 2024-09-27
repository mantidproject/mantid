# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans_core.user_file.toml_parsers.toml_parser import TomlParser
from sans_core.user_file.toml_parsers.toml_reader import TomlReader


class TomlParserTest(unittest.TestCase):
    def get_mocked_reader(self, mocked_return):
        self.mocked_reader = mock.Mock(spec=TomlReader)
        self.mocked_reader.get_user_file_dict.return_value = mocked_return
        return self.mocked_reader

    def test_returns_v1_parser(self):
        test_dict = {"toml_file_version": 1}

        parser = TomlParser(toml_reader=self.get_mocked_reader(test_dict))
        mocked_file_info = mock.NonCallableMock()
        with mock.patch("sans_core.user_file.toml_parsers.toml_parser.TomlV1Parser") as mocked_import:
            parser_version = parser.get_toml_parser(toml_file_path=mock.NonCallableMock, file_information=mocked_file_info)
            self.assertEqual(mocked_import.return_value, parser_version)
            # Check correct params were forwarded on
            mocked_import.assert_called_once_with(test_dict, file_information=mocked_file_info)

    def test_throws_for_unknown_version(self):
        test_dict = {"toml_file_version": 100}
        parser = TomlParser(toml_reader=self.get_mocked_reader(test_dict))
        with self.assertRaises(NotImplementedError):
            parser.get_toml_parser(toml_file_path=mock.NonCallableMock, file_information=None)

    def test_throws_for_missing_version(self):
        parser = TomlParser(toml_reader=self.get_mocked_reader({}))
        with self.assertRaises(KeyError):
            parser.get_toml_parser(toml_file_path=mock.NonCallableMock, file_information=None)

    def test_parse_toml_file_calls_get_all_states(self):
        test_dict = {"toml_file_version": 1}

        parser = TomlParser(toml_reader=self.get_mocked_reader(test_dict))
        with mock.patch("sans_core.user_file.toml_parsers.toml_parser.TomlV1Parser") as mocked_parser:
            parsed = parser.parse_toml_file(mock.NonCallableMock, file_information=None)
            mocked_parser.return_value.get_all_states.assert_called_once()
            self.assertEqual(mocked_parser.return_value.get_all_states.return_value, parsed)


if __name__ == "__main__":
    unittest.main()
