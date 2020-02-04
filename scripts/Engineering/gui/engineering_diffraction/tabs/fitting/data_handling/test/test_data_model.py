# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantid.py3compat.mock import patch

from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_model import FittingDataModel

file_path = "Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_model"


class TestFittingDataModel(unittest.TestCase):
    def setUp(self):
        self.model = FittingDataModel()

    @patch(file_path + ".Load")
    def test_loading_single_file_stores_workspace(self, mock_load):
        mock_load.return_value = "mocked_workspace"

        self.model.load_files("/ar/a_filename.whatever")

        self.assertEqual(1, len(self.model._loaded_workspaces))
        self.assertEqual("mocked_workspace", self.model._loaded_workspaces["a_filename"])
        mock_load.assert_called_with("/ar/a_filename.whatever", OutputWorkspace="a_filename")

    @patch(file_path + ".logger")
    @patch(file_path + ".Load")
    def test_loading_single_file_invalid(self, mock_load, mock_logger):
        mock_load.side_effect = RuntimeError("Invalid Path")

        self.model.load_files("/ar/a_filename.whatever")

        self.assertEqual(0, len(self.model._loaded_workspaces))
        mock_load.assert_called_with("/ar/a_filename.whatever", OutputWorkspace="a_filename")
        self.assertEqual(1, mock_logger.error.call_count)

    @patch(file_path + ".Load")
    def test_loading_multiple_files(self, mock_load):
        mock_load.return_value = "mocked_workspace"

        self.model.load_files("/dir/file1.txt, /dir/file2.nxs")

        self.assertEqual(2, len(self.model._loaded_workspaces))
        self.assertEqual("mocked_workspace", self.model._loaded_workspaces["file1"])
        self.assertEqual("mocked_workspace", self.model._loaded_workspaces["file2"])
        mock_load.assert_any_call("/dir/file1.txt", OutputWorkspace="file1")
        mock_load.assert_any_call("/dir/file2.nxs", OutputWorkspace="file2")

    @patch(file_path + ".logger")
    @patch(file_path + ".Load")
    def test_loading_multiple_files_invalid(self, mock_load, mock_logger):
        mock_load.side_effect = RuntimeError("Invalid Path")

        self.model.load_files("/dir/file1.txt, /dir/file2.nxs")

        self.assertEqual(0, len(self.model._loaded_workspaces))
        mock_load.assert_any_call("/dir/file1.txt", OutputWorkspace="file1")
        mock_load.assert_any_call("/dir/file2.nxs", OutputWorkspace="file2")
        self.assertEqual(2, mock_logger.error.call_count)


if __name__ == '__main__':
    unittest.main()
