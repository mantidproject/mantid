# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import os
from io import StringIO
import unittest

from mantid.py3compat import mock
import Muon.GUI.Common.utilities.muon_file_utils as utils


class MuonFileUtilsTest(unittest.TestCase):

    def test_parse_user_input_to_files_returns_single_file_as_list(self):
        filename = "EMU0001234.nxs"
        parsed_file = utils.parse_user_input_to_files(filename)

        self.assertEqual(parsed_file, [filename])

    def test_parse_user_input_to_files_returns_full_filepath(self):
        files = ["C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001234.nxs",
                 "dir1" + os.sep + "dir2" + os.sep + "EMU0001234.nxs"]

        for file_name in files:
            parsed_file = utils.parse_user_input_to_files(file_name)
            self.assertEqual(parsed_file, [file_name])

    def test_parse_user_input_to_files_returns_list_correctly(self):
        user_input = "C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001234.nxs;" \
                                                                         "C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001235.nxs;" \
                                                                                                                             "C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001236.nxs"
        files = ["C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001234.nxs",
                 "C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001235.nxs",
                 "C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001236.nxs"]

        parsed_file = utils.parse_user_input_to_files(user_input)
        self.assertEqual(parsed_file, files)

    def test_parse_user_input_to_files_filters_files_with_incorrect_extension(self):
        user_input = "C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001234.nxs;" \
                                                                         "C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001235.txt;" \
                                                                                                                             "C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001236.png"
        files = ["C:" + os.sep + "dir1" + os.sep + "dir2" + os.sep + "EMU0001234.nxs"]

        parsed_file = utils.parse_user_input_to_files(user_input, ['nxs'])
        self.assertEqual(parsed_file, files)

    def test_duplicates_removed_from_list_of_filenames_and_ordering_maintained(self):
        file_list = [os.sep + "dir1" + os.sep + "dir2" + os.sep + "file1.nxs",
                     os.sep + "dir1" + os.sep + "dir4" + os.sep + "file2.nxs",
                     os.sep + "dir4" + os.sep + "dir2" + os.sep + "file1.nxs",
                     os.sep + "dir1" + os.sep + "dir4" + os.sep + "file1.nxs"]
        unique_file_list = utils.remove_duplicated_files_from_list(file_list)
        self.assertEqual(unique_file_list, [os.sep + "dir1" + os.sep + "dir2" + os.sep + "file1.nxs",
                                            os.sep + "dir1" + os.sep + "dir4" + os.sep + "file2.nxs"])

    def test_that_get_current_run_filename_throws_if_autosave_file_not_found(self):
        utils.check_file_exists = mock.Mock(return_value=False)

        with self.assertRaises(ValueError):
            utils.get_current_run_filename("EMU")

    def test_that_get_current_run_returns_correct_run(self):
        utils.check_file_exists = mock.Mock(return_value=True)
        expected_file_name = os.sep + os.sep + "EMU" + os.sep + "data" + os.sep + "autoA"
        test_file_name = StringIO(u"autoA")
        utils.open = mock.Mock(return_value = test_file_name)
        current_file_name = utils.get_current_run_filename("EMU")
        self.assertEqual(current_file_name, expected_file_name)

    def test_that_get_current_run_throws_if_no_valid_run_in_autosave_run(self):
        utils.check_file_exists = mock.Mock(side_effect = [True, False])
        with self.assertRaises(ValueError):
            utils.get_current_run_filename("EMU")

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
