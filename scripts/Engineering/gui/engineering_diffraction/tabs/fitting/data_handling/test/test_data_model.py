# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from unittest.mock import patch

from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_model import FittingDataModel

file_path = "Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_model"


class TestFittingDataModel(unittest.TestCase):
    def setUp(self):
        self.model = FittingDataModel()

    @patch(file_path + '.ConvertUnits')
    @patch(file_path + ".Load")
    def test_loading_single_file_stores_workspace(self, mock_load, mock_convunits):
        mock_ws = mock.MagicMock()
        mock_ws.getNumberHistograms.return_value = 1
        mock_load.return_value = mock_ws

        self.model.load_files("/ar/a_filename.whatever", "dSpacing")

        mock_convunits.assert_called_once()
        self.assertEqual(1, len(self.model._loaded_workspaces))
        self.assertEqual(mock_ws, self.model._loaded_workspaces["a_filename_dSpacing"])
        mock_load.assert_called_with("/ar/a_filename.whatever", OutputWorkspace="a_filename_dSpacing")

    @patch(file_path + '.ConvertUnits')
    @patch(file_path + ".logger")
    @patch(file_path + ".Load")
    def test_loading_single_file_invalid(self, mock_load, mock_logger, mock_convunits):
        mock_load.side_effect = RuntimeError("Invalid Path")

        self.model.load_files("/ar/a_filename.whatever", "TOF")
        mock_convunits.assert_not_called()
        self.assertEqual(0, len(self.model._loaded_workspaces))
        mock_load.assert_called_with("/ar/a_filename.whatever", OutputWorkspace="a_filename_TOF")
        self.assertEqual(1, mock_logger.error.call_count)

    @patch(file_path + ".Load")
    def test_loading_multiple_files(self, mock_load):
        mock_ws = mock.MagicMock()
        mock_ws.getNumberHistograms.return_value = 1
        mock_load.return_value = mock_ws

        self.model.load_files("/dir/file1.txt, /dir/file2.nxs", "TOF")

        self.assertEqual(2, len(self.model._loaded_workspaces))
        self.assertEqual(mock_ws, self.model._loaded_workspaces["file1_TOF"])
        self.assertEqual(mock_ws, self.model._loaded_workspaces["file2_TOF"])
        mock_load.assert_any_call("/dir/file1.txt", OutputWorkspace="file1_TOF")
        mock_load.assert_any_call("/dir/file2.nxs", OutputWorkspace="file2_TOF")

    @patch(file_path + ".logger")
    @patch(file_path + ".Load")
    def test_loading_multiple_files_too_many_spectra(self, mock_load, mock_logger):
        mock_ws = mock.MagicMock()
        mock_ws.getNumberHistograms.return_value = 2
        mock_load.return_value = mock_ws

        self.model.load_files("/dir/file1.txt, /dir/file2.nxs", "TOF")

        self.assertEqual(0, len(self.model._loaded_workspaces))
        mock_load.assert_any_call("/dir/file1.txt", OutputWorkspace="file1_TOF")
        mock_load.assert_any_call("/dir/file2.nxs", OutputWorkspace="file2_TOF")
        self.assertEqual(2, mock_logger.warning.call_count)

    @patch(file_path + ".logger")
    @patch(file_path + ".Load")
    def test_loading_multiple_files_invalid(self, mock_load, mock_logger):
        mock_load.side_effect = RuntimeError("Invalid Path")

        self.model.load_files("/dir/file1.txt, /dir/file2.nxs", "TOF")

        self.assertEqual(0, len(self.model._loaded_workspaces))
        mock_load.assert_any_call("/dir/file1.txt", OutputWorkspace="file1_TOF")
        mock_load.assert_any_call("/dir/file2.nxs", OutputWorkspace="file2_TOF")
        self.assertEqual(2, mock_logger.error.call_count)


if __name__ == '__main__':
    unittest.main()
