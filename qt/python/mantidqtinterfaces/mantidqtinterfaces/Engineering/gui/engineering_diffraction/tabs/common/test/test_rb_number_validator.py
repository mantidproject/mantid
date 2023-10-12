import unittest
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.rb_number_validator import RBNumberValidator


class RBNumberValidatorTest(unittest.TestCase):
    def setUp(self) -> None:
        self.rb_number_validator = RBNumberValidator()

    def test_invalid_characters_in_rb_number(self):
        invalid_rb_number_user_inputs = [
            r" dirName",
            r"dirName ",
            r" dir1/dir2",
            r"dir1/dir2 ",
            r"dir1 /dir2",
            r"dir1/ dir2",
            r"dir1 / dir2",
            r"/dirName",
            r"dirName  ",
            r"dir1///dir2",
            r"dir1\\\dir2",
            r" dir1\dir2",
            r"dir1\dir2 ",
            r"dir1 \dir2",
            r"dir1\ dir2",
            r"dir1 \ dir2",
            r"\dirName",
            r" dir1\\dir2",
            r"dir1\\dir2 ",
            r"dir1 \\dir2",
            r"dir1\\ dir2",
            r"dir1 \\ dir2",
            r"\\dirName",
            r"dir1\\\dir2",
            r"dir#",
            r"dir?",
            r"~dir",
            r"~/dir",
            r"{}",
            r"dir+",
            r"dir=",
            r"$%",
            r"%dir",
            r"^&",
            r"*pat(h)",
            r"i&n$v¬a`l|d",
            r"dir1/dir2/dir3 ",
            r"dir1/dir2 /dir3/dir4",
            r"dir1 /dir2/dir3",
            r"dir1/dir#2/dir3",
            r"dir1/dir2///dir3",
            r"dir1/dir@2/dir3",
        ]
        for user_input_rb_number in invalid_rb_number_user_inputs:
            self.assertFalse(self.rb_number_validator.validate_rb_number(user_input_rb_number))

    def test_valid_rb_numbers(self):
        valid_rb_number_user_inputs = [
            r"dirName",
            r"dir1/dir2",
            r"dir1//dir2",
            r"dir1\dir2",
            r"dir1\\dir2",
            r"dir1/dir2/dir3/dir4",
            r"directory name1/directory name 2/dir name 3/",
            r"dir_1/dir-2/new-dir_3/new folder",
            r"New Folder 1/NEW Folder 2/new-folder-3/new_folder_4",
        ]
        for user_input_rb_number in valid_rb_number_user_inputs:
            self.assertTrue(self.rb_number_validator.validate_rb_number(user_input_rb_number))


if __name__ == "__main__":
    unittest.main()
