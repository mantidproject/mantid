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
from sans.command_interface.ISISCommandInterface import Clean, MaskFile, set_save
from sans.common.enums import OutputMode


class ISISCommandInterfaceTest(unittest.TestCase):
    def tearDown(self):
        Clean()

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
        with mock.patch("sans.command_interface.ISISCommandInterface.find_full_file_path") as mocked_finder:
            mocked_finder.return_value = tmp_file.name
            self.assertIsNone(MaskFile(file_name))

    def test_set_save_raises_an_error_with_wrong_save_algorithms(self):
        save_algs = [["SaveBad"], ["SaveRKH", "SaveBad"]]

        for alg in save_algs:
            with self.subTest(test_case=alg):
                with self.assertRaises(RuntimeError):
                    set_save(alg)

    def test_output_mode_defaults_to_publish_to_ads_if_save_algs_is_none(self):
        output_modes = [OutputMode.BOTH, OutputMode.SAVE_TO_FILE]

        for mode in output_modes:
            with self.subTest(test_case=mode):
                output_mode = set_save(None, mode)
                self.assertEqual(output_mode, OutputMode.PUBLISH_TO_ADS)

    def test_output_mode_defaults_to_BOTH_if_there_is_a_save_alg(self):
        output_mode = set_save(["SaveRKH"])
        self.assertEqual(output_mode, OutputMode.BOTH)


if __name__ == "__main__":
    unittest.main()
