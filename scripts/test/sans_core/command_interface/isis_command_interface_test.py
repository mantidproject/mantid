# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest
import uuid

from unittest import mock
from sans_core.command_interface.ISISCommandInterface import MaskFile


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

    def test_mask_file_works_for_full_path(self):
        tmp_file = tempfile.NamedTemporaryFile(mode="r")
        path = tmp_file.name
        self.assertTrue(os.path.isfile(path))

        self.assertIsNone(MaskFile(path))

    def test_mask_file_for_existing_file(self):
        tmp_file = tempfile.NamedTemporaryFile(mode="r")

        file_name = os.path.basename(tmp_file.name)
        with mock.patch("sans_core.command_interface.ISISCommandInterface.find_full_file_path") as mocked_finder:
            mocked_finder.return_value = tmp_file.name
            self.assertIsNone(MaskFile(file_name))


if __name__ == "__main__":
    unittest.main()
