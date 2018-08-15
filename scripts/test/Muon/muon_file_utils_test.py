import Muon.GUI.Common.muon_file_utils as utils
import unittest


class RunStringUtilsListToStringTest(unittest.TestCase):

    def test_parse_user_input_to_files_returns_file_correctly(self):
        file = "EMU0001234.nxs"
        parsed_file = utils.parse_user_input_to_files(file)
        self.assertEqual(parsed_file, [file])

    def test_parse_user_input_to_files_returns_full_filepath(self):
        files = ["C:\dir1\dir2\EMU0001234.nxs",
                 "/dir1/dir2/EMU0001234.nxs",
                 "dir1\dir2\EMU0001234.nxs",
                 "dir1/dir2/EMU0001234.nxs"]
        for file_name in files:
            parsed_file = utils.parse_user_input_to_files(file_name)
            self.assertEqual(parsed_file, [file_name])

    def test_parse_user_input_to_files_returns_list_correctly(self):
        user_input = "C:\dir1\dir2\EMU0001234.nxs;C:\dir1\dir2\EMU0001235.nxs;C:\dir1\dir2\EMU0001236.nxs"
        files = ["C:\dir1\dir2\EMU0001234.nxs", "C:\dir1\dir2\EMU0001235.nxs", "C:\dir1\dir2\EMU0001236.nxs"]
        parsed_file = utils.parse_user_input_to_files(user_input)
        self.assertEqual(parsed_file, files)

    def test_parse_user_input_to_files_filters_files_with_incorrect_extension(self):
        user_input = "C:\dir1\dir2\EMU0001234.nxs;C:\dir1\dir2\EMU0001235.txt;C:\dir1\dir2\EMU0001236.png"
        files = ["C:\dir1\dir2\EMU0001234.nxs"]
        parsed_file = utils.parse_user_input_to_files(user_input, ['.nxs'])
        self.assertEqual(parsed_file, files)

    def test_duplicates_removed_from_list_of_filenames_and_ordering_maintained(self):
        file_list = ["\\dir1\\dir2\\file1.nxs",
                     "\\dir1\\dir4\\file2.nxs",
                     "\\dir4\\dir2\\file1.nxs",
                     "\\dir1\\dir4\\file1.nxs"]
        unique_file_list = utils.remove_duplicated_files_from_list(file_list)
        self.assertEqual(unique_file_list, ["\\dir1\\dir2\\file1.nxs",
                                            "\\dir1\\dir4\\file2.nxs"])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
