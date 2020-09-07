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
        # setup a mock workspace
        mock_inst = mock.MagicMock()
        mock_inst.getFullName.return_value = 'instrument'
        mock_prop = mock.MagicMock()
        mock_prop.value = 1  # bank-id
        mock_run = mock.MagicMock()
        mock_run.getProtonCharge.return_value = 1.0
        mock_run.getProperty.return_value = mock_prop
        self.mock_ws = mock.MagicMock()
        self.mock_ws.getNumberHistograms.return_value = 1
        self.mock_ws.getRun.return_value = mock_run
        self.mock_ws.getInstrument.return_value = mock_inst
        self.mock_ws.getRunNumber.return_value = 1
        self.mock_ws.getTitle.return_value = 'title'

    @patch(file_path + '.ConvertUnits')
    @patch(file_path + ".Load")
    def test_loading_single_file_stores_workspace(self, mock_load, mock_convunits):
        mock_load.return_value = self.mock_ws

        self.model.load_files("/ar/a_filename.whatever", "dSpacing")

        mock_convunits.assert_called_once()
        self.assertEqual(1, len(self.model._loaded_workspaces))
        self.assertEqual(self.mock_ws, self.model._loaded_workspaces["a_filename_dSpacing"])
        mock_load.assert_called_with("/ar/a_filename.whatever", OutputWorkspace="a_filename_dSpacing")

    @patch(file_path + '.FittingDataModel.update_log_group_name')
    @patch(file_path + '.ADS')
    @patch(file_path + ".Load")
    def test_loading_single_file_already_loaded_untracked(self, mock_load, mock_ads, mock_update_logname):
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.return_value = self.mock_ws

        self.model.load_files("/ar/a_filename.whatever", "TOF")

        self.assertEqual(1, len(self.model._loaded_workspaces))
        mock_load.assert_not_called()
        mock_update_logname.assert_called_once()  # needed to patch this as calls ADS.retrieve

    @patch(file_path + '.FittingDataModel.update_log_group_name')
    @patch(file_path + ".Load")
    def test_loading_single_file_already_loaded_tracked(self, mock_load, mock_update_logname):
        fpath = "/ar/a_filename.whatever"
        xunit = "TOF"
        self.model._loaded_workspaces = {self.model._generate_workspace_name(fpath, xunit): self.mock_ws}

        self.model.load_files(fpath, xunit)

        self.assertEqual(1, len(self.model._loaded_workspaces))
        mock_load.assert_not_called()
        mock_update_logname.assert_not_called()

    @patch(file_path + '.get_setting')
    @patch(file_path + '.AverageLogData')
    @patch(file_path + ".Load")
    def test_loading_single_file_with_logs(self, mock_load, mock_avglogs, mock_getsetting):
        mock_load.return_value = self.mock_ws
        log_names = ['to', 'test']
        mock_getsetting.return_value = ','.join(log_names)
        mock_avglogs.return_value = (1.0, 1.0)  # avg, stdev

        self.model.load_files("/ar/a_filename.whatever", "TOF")

        self.assertEqual(1, len(self.model._loaded_workspaces))
        self.assertEqual(self.mock_ws, self.model._loaded_workspaces["a_filename_TOF"])
        mock_load.assert_called_with("/ar/a_filename.whatever", OutputWorkspace="a_filename_TOF")
        self.assertEqual(1 + len(log_names), len(self.model._log_workspaces))
        for ilog in range(0, len(log_names)):
            self.assertEqual(log_names[ilog], self.model._log_workspaces[ilog + 1].name())
            self.assertEqual(1, self.model._log_workspaces[ilog + 1].rowCount())

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
        mock_load.return_value = self.mock_ws

        self.model.load_files("/dir/file1.txt, /dir/file2.nxs", "TOF")

        self.assertEqual(2, len(self.model._loaded_workspaces))
        self.assertEqual(self.mock_ws, self.model._loaded_workspaces["file1_TOF"])
        self.assertEqual(self.mock_ws, self.model._loaded_workspaces["file2_TOF"])
        mock_load.assert_any_call("/dir/file1.txt", OutputWorkspace="file1_TOF")
        mock_load.assert_any_call("/dir/file2.nxs", OutputWorkspace="file2_TOF")

    @patch(file_path + ".logger")
    @patch(file_path + ".Load")
    def test_loading_multiple_files_too_many_spectra(self, mock_load, mock_logger):
        self.mock_ws.getNumberHistograms.return_value = 2
        mock_load.return_value = self.mock_ws

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

    @patch(file_path + ".EnggEstimateFocussedBackground")
    @patch(file_path + ".Minus")
    @patch(file_path + ".Plus")
    def test_do_background_subtraction_first_time(self, mock_plus, mock_minus, mock_estimate_bg):
        self.model._loaded_workspaces = {"name1": self.mock_ws}
        self.model._background_workspaces = {"name1": None}
        self.model._bg_params = dict()
        mock_estimate_bg.return_value = self.mock_ws

        bg_params = [True, 40, 800, False]
        self.model.do_background_subtraction("name1", bg_params)

        self.assertEqual(self.model._bg_params["name1"], bg_params)
        mock_minus.assert_called_once()
        mock_estimate_bg.assert_called_once()
        mock_plus.assert_not_called()

    @patch(file_path + ".EnggEstimateFocussedBackground")
    @patch(file_path + ".Minus")
    @patch(file_path + ".Plus")
    def test_do_background_subtraction_bgparams_changed(self, mock_plus, mock_minus, mock_estimate_bg):
        self.model._loaded_workspaces = {"name1": self.mock_ws}
        self.model._background_workspaces = {"name1": self.mock_ws}
        self.model._bg_params = {"name1": [True, 80, 1000, False]}
        mock_estimate_bg.return_value = self.mock_ws

        bg_params = [True, 40, 800, False]
        self.model.do_background_subtraction("name1", bg_params)

        self.assertEqual(self.model._bg_params["name1"], bg_params)
        mock_minus.assert_called_once()
        mock_estimate_bg.assert_called_once()
        mock_plus.assert_called_once()

    @patch(file_path + ".EnggEstimateFocussedBackground")
    @patch(file_path + ".Minus")
    @patch(file_path + ".Plus")
    def test_do_background_subtraction_no_change(self, mock_plus, mock_minus, mock_estimate_bg):
        self.model._loaded_workspaces = {"name1": self.mock_ws}
        self.model._background_workspaces = {"name1": self.mock_ws}
        bg_params = [True, 80, 1000, False]
        self.model._bg_params = {"name1": bg_params}
        mock_estimate_bg.return_value = self.mock_ws

        self.model.do_background_subtraction("name1", bg_params)

        self.assertEqual(self.model._bg_params["name1"], bg_params)
        mock_minus.assert_called_once()
        mock_estimate_bg.assert_not_called()
        mock_plus.assert_not_called()

    @patch(file_path + '.RenameWorkspace')
    @patch(file_path + ".ADS")
    @patch(file_path + ".DeleteTableRows")
    def test_remove_run_from_log_table(self, mock_delrows, mock_ads, mock_rename):
        mock_table = mock.MagicMock()
        mock_table.column.return_value = [1, 2]
        mock_table.rowCount.return_value = len(mock_table.column())  # two left post-removal
        mock_table.row.return_value = {'Instrument': 'test_inst'}
        mock_ads.retrieve.return_value = mock_table
        self.model._log_workspaces = mock.MagicMock()
        self.model._log_workspaces.name.return_value = 'test'

        self.model.remove_log_rows([0])

        mock_delrows.assert_called_once()
        new_name = f"{mock_table.row()['Instrument']}_{min(mock_table.column())}-{max(mock_table.column())}_logs"
        mock_rename.assert_called_with(InputWorkspace=self.model._log_workspaces.name(), OutputWorkspace=new_name)

    @patch(file_path + '.DeleteWorkspace')
    @patch(file_path + ".ADS")
    @patch(file_path + ".DeleteTableRows")
    def test_remove_last_run_from_log_table(self, mock_delrows, mock_ads, mock_delws):
        mock_table = mock.MagicMock()
        mock_table.rowCount.return_value = 0  # none left post-removal
        mock_ads.retrieve.return_value = mock_table
        self.model._log_workspaces = mock.MagicMock()
        name = 'test'
        self.model._log_workspaces.name.return_value = name

        self.model.remove_log_rows([0])

        mock_delrows.assert_called_once()
        mock_delws.assert_called_with(name)
        self.assertEqual(None, self.model._log_workspaces)

    def test_update_workspace_name(self):
        self.model._loaded_workspaces = {"name1": self.mock_ws, "name2": self.mock_ws}
        self.model._background_workspaces = {"name1": self.mock_ws, "name2": self.mock_ws}
        self.model._bg_params = {"name1": [True, 80, 1000, False]}

        self.model.update_workspace_name("name1", "new_name")

        self.assertEqual({"new_name": self.mock_ws, "name2": self.mock_ws}, self.model._loaded_workspaces)
        self.assertEqual({"new_name": self.mock_ws, "name2": self.mock_ws}, self.model._background_workspaces)
        self.assertEqual({"new_name": [True, 80, 1000, False]}, self.model._bg_params)


if __name__ == '__main__':
    unittest.main()
