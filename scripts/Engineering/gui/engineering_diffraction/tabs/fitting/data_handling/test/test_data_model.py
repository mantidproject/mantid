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

    @patch(file_path + '.FittingDataModel.write_table_row')
    @patch(file_path + '.FittingDataModel.update_log_group_name')
    @patch(file_path + '.ADS')
    @patch(file_path + ".Load")
    def test_loading_single_file_already_loaded_untracked(self, mock_load, mock_ads, mock_update_logname,
                                                          mock_writerow):
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.return_value = self.mock_ws

        self.model.load_files("/ar/a_filename.whatever", "TOF")

        self.assertEqual(1, len(self.model._loaded_workspaces))
        mock_load.assert_not_called()
        mock_update_logname.assert_called_once()  # needed to patch this as calls ADS.retrieve
        self.assertEqual(1, mock_writerow.call_count)  # no logs included

    @patch(file_path + '.FittingDataModel.update_log_group_name')
    @patch(file_path + ".Load")
    def test_loading_single_file_already_loaded_tracked(self, mock_load, mock_update_logname):
        fpath = "/ar/a_filename.whatever"
        xunit = "TOF"
        self.model._loaded_workspaces = {self.model._generate_workspace_name(fpath, xunit): self.mock_ws}

        self.model.load_files(fpath, xunit)

        self.assertEqual(1, len(self.model._loaded_workspaces))
        mock_load.assert_not_called()
        mock_update_logname.assert_called()

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
        self.model._log_values = {"name1": 1, "name2": 2}

        self.model.update_workspace_name("name1", "new_name")

        self.assertEqual({"new_name": self.mock_ws, "name2": self.mock_ws}, self.model._loaded_workspaces)
        self.assertEqual({"new_name": self.mock_ws, "name2": self.mock_ws}, self.model._background_workspaces)
        self.assertEqual({"new_name": [True, 80, 1000, False]}, self.model._bg_params)
        self.assertEqual({"new_name": 1, "name2": 2}, self.model._log_values)

    @patch(file_path + ".FittingDataModel.create_log_workspace_group")
    @patch(file_path + ".FittingDataModel.add_log_to_table")
    def test_update_logs_initial(self, mock_add_log, mock_create_log_group):
        self.model._loaded_workspaces = {"name1": self.mock_ws}
        self.model._log_workspaces = None

        self.model.update_log_workspace_group()
        mock_create_log_group.assert_called_once()
        mock_add_log.assert_called_with("name1", self.mock_ws, 0)

    @patch(file_path + ".FittingDataModel.make_log_table")
    @patch(file_path + ".FittingDataModel.make_runinfo_table")
    @patch(file_path + ".FittingDataModel.create_log_workspace_group")
    @patch(file_path + ".FittingDataModel.add_log_to_table")
    @patch(file_path + ".ADS")
    def test_update_logs_deleted(self, mock_ads, mock_add_log, mock_create_log_group, mock_make_runinfo,
                                 mock_make_log_table):
        self.model._loaded_workspaces = {"name1": self.mock_ws}
        self.model._log_workspaces = mock.MagicMock()
        self.model._log_names = ['LogName1', 'LogName2']
        # simulate LogName2 and run_info tables being deleted
        mock_ads.doesExist = lambda ws_name: ws_name == 'LogName1'

        self.model.update_log_workspace_group()
        mock_create_log_group.assert_not_called()
        mock_make_runinfo.assert_called_once()
        mock_make_log_table.assert_called_once_with('LogName2')
        self.model._log_workspaces.add.assert_any_call("run_info")
        self.model._log_workspaces.add.assert_any_call("LogName2")
        mock_add_log.assert_called_with("name1", self.mock_ws, 0)

    @patch(file_path + ".FittingDataModel.make_log_table")
    @patch(file_path + ".FittingDataModel.make_runinfo_table")
    @patch(file_path + ".get_setting")
    @patch(file_path + ".GroupWorkspaces")
    def test_create_log_workspace_group(self, mock_group, mock_get_setting, mock_make_runinfo, mock_make_log_table):
        log_names = ['LogName1', 'LogName2']
        mock_get_setting.return_value = ','.join(log_names)
        mock_group_ws = mock.MagicMock()
        mock_group.return_value = mock_group_ws

        self.model.create_log_workspace_group()

        mock_group.assert_called_once()
        for log in log_names:
            mock_group_ws.add.assert_any_call(log)
        self.assertEqual(self.model._log_names, log_names)
        mock_make_runinfo.assert_called_once()
        self.assertEqual(mock_make_log_table.call_count, len(log_names))

    @patch(file_path + ".FittingDataModel.write_table_row")
    @patch(file_path + ".ADS")
    @patch(file_path + ".FittingDataModel.update_log_group_name")
    @patch(file_path + ".AverageLogData")
    def test_add_log_to_table(self, mock_avglogs, mock_update_logname, mock_ads, mock_writerow):
        # grouped ws acts like a container/list of ws here
        self.model._log_workspaces = [mock.MagicMock(), mock.MagicMock()]
        self.model._log_workspaces[0].name.return_value = "run_info"
        self.model._log_workspaces[1].name.return_value = "LogName"
        mock_ads.retrieve = lambda ws_name: [ws for ws in self.model._log_workspaces if ws.name() == ws_name][0]
        self.model._log_values = {"name1": {"LogName": [2, 1]}}
        self.model._log_names = ["LogName"]

        self.model.add_log_to_table("name1", self.mock_ws, 3)
        mock_writerow.assert_any_call(self.model._log_workspaces[0], ['instrument', 1, 1, 1.0, 'title'], 3)
        mock_writerow.assert_any_call(self.model._log_workspaces[1], [2, 1], 3)
        mock_avglogs.assert_not_called()
        mock_update_logname.assert_called_once()

    def test_write_table_row(self):
        table_ws = mock.MagicMock()
        table_ws.rowCount.return_value = 3

        self.model.write_table_row(table_ws, [1, 2], 3)

        table_ws.setRowCount.assert_called_with(4)  # row added
        table_ws.setCell.assert_any_call(3, 0, 1)
        table_ws.setCell.assert_any_call(3, 1, 2)


if __name__ == '__main__':
    unittest.main()
