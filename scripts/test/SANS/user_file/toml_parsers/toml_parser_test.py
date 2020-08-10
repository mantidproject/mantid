# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans.user_file.toml_parsers.toml_parser import TomlParser


class TomlParserTest(unittest.TestCase):
    @staticmethod
    def mock_reader(toml_dict=None):
        if toml_dict is None:
            toml_dict = {}
            toml_dict["instrument"] = {"name": "SANS2D"}
            toml_dict["toml_file_version"] = 0

        mocked = mock.MagicMock()
        mocked.get_user_file_dict.return_value = toml_dict
        return mocked

    def test_toml_parser_appends_user_file(self):
        toml_file_path = mock.NonCallableMock()

        parser = TomlParser(toml_reader=self.mock_reader())
        state = parser.parse_toml_file(toml_file_path=toml_file_path, file_information=None)
        self.assertEqual(toml_file_path, state.save.user_file_name)


if __name__ == '__main__':
    unittest.main()
