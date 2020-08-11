# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import pathlib
import tempfile
import unittest
from unittest import mock

from sans.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


@mock.patch("sans.user_file.txt_parsers.UserFileReaderAdapter.UserFileReader")
class UserFileReaderAdapterTest(unittest.TestCase):
    def test_get_input_dict(self, mock_file_reader):
        user_file_name = mock.NonCallableMock()

        adapter = UserFileReaderAdapter(file_information=None, user_file_name=user_file_name)
        mock_file_reader.assert_called_once_with(user_file_name)
        mock_file_reader.return_value.read_user_file.reset_mock()  # Will call during init
        # Check if calls read user file
        adapter._get_input_dict()
        mock_file_reader.return_value.read_user_file.assert_called()

    def test_state_save_appends_user_file(self, _):
        filename = tempfile.mkstemp()[1]  # Get filename instead of handle

        adapter = UserFileReaderAdapter(file_information=None, user_file_name=filename)
        returned = adapter.get_state_save()
        self.assertNotEqual('', returned.user_file_name)
        self.assertEqual(os.path.basename(filename), returned.user_file_name)


if __name__ == '__main__':
    unittest.main()
