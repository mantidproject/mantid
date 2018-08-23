import Muon.GUI.Common.muon_file_utils as utils
import sys
import os

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class RunStringUtilsListToStringTest(unittest.TestCase):

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
        file_list = ["\\dir1\\dir2\\file1.nxs",
                     "\\dir1\\dir4\\file2.nxs",
                     "\\dir4\\dir2\\file1.nxs",
                     "\\dir1\\dir4\\file1.nxs"]
        unique_file_list = utils.remove_duplicated_files_from_list(file_list)
        self.assertEqual(unique_file_list, ["\\dir1\\dir2\\file1.nxs",
                                            "\\dir1\\dir4\\file2.nxs"])

    def test_that_get_current_run_filename_throws_if_autosave_file_not_found(self):
        utils.check_file_exists = mock.Mock(return_value=False)

        with self.assertRaises(ValueError):
            utils.get_current_run_filename("EMU")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
