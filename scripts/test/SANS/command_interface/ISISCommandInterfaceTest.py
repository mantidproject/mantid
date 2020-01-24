# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest
import uuid

import six

from sans.command_interface.ISISCommandInterface import MaskFile

if six.PY2:
    FileNotFoundError = IOError


class ISISCommandInterfaceTest(unittest.TestCase):
    def test_mask_file_raises_for_empty_file_name(self):
        with self.assertRaises(ValueError):
            MaskFile(file_name="")

    def test_mask_file_handles_folder_paths(self):
        tmp_dir = tempfile.gettempdir()
        non_existent_folder = str(uuid.uuid1())
        path_not_there = os.path.join(tmp_dir, non_existent_folder)

        self.assertFalse(os.path.exists(path_not_there))
        with self.assertRaises(FileNotFoundError):
            MaskFile(path_not_there)

    def test_mask_file_handles_non_existent_user_files(self):
        tmp_dir = tempfile.gettempdir()
        non_existent_file = str(uuid.uuid1())
        file_not_there = os.path.join(tmp_dir, non_existent_file)

        self.assertFalse(os.path.exists(file_not_there))
        with self.assertRaises(FileNotFoundError):
            MaskFile(file_not_there)


if __name__ == '__main__':
    unittest.main()
