# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from unittest.mock import patch
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling import data_model, data_presenter, data_view

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling"


def _get_item_checked_mock(_, arg):
    if arg == 2:
        return True
    elif arg == 3:
        return False


class FittingDataPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(data_model.FittingDataModel, instance=True)
        self.view = mock.create_autospec(data_view.FittingDataView, instance=True)
        self.presenter = data_presenter.FittingDataPresenter(self.model, self.view)
        # TOF axes
        mock_unit_TOF = mock.MagicMock()
        mock_unit_TOF.unitID.return_value = "TOF"
        mock_axis_TOF = mock.MagicMock()
        mock_axis_TOF.getUnit.return_value = mock_unit_TOF
        # dSpacing axes
        mock_unit_d = mock.MagicMock()
        mock_unit_d.unitID.return_value = "dSpacing"
        mock_axis_d = mock.MagicMock()
        mock_axis_d.getUnit.return_value = mock_unit_d
        # make mock workspaces
        self.ws1 = mock.MagicMock()
        self.ws1.getAxis.return_value = mock_axis_TOF
        self.ws2 = mock.MagicMock()
        self.ws2.getAxis.return_value = mock_axis_TOF
        self.ws3 = mock.MagicMock()
        self.ws3.getAxis.return_value = mock_axis_d

    def _setup_fitting_table_rows(self, ws_names, active_ws_xunit=None):
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        for iws, ws_name in enumerate(ws_names):
            self.presenter.row_numbers[ws_name] = iws
        if active_ws_xunit is not None:
            ws = mock.MagicMock()
            ws.getXDimension().name = active_ws_xunit
            return ws
        else:
            return None

    @patch(dir_path + ".data_presenter.AsyncTask")
    def test_worker_started_correctly(self, mock_worker):
        self.view.is_searching.return_value = False
        self.view.get_filenames_to_load.return_value = "/a/file/to/load.txt, /another/one.nxs"
        self.model.load_files = "mocked model method"

        self.presenter.on_load_clicked()

        mock_worker.assert_called_with(
            "mocked model method",
            ("/a/file/to/load.txt, /another/one.nxs",),
            error_cb=self.presenter._on_worker_error,
            finished_cb=self.presenter._emit_enable_load_button_signal,
            success_cb=self.presenter._on_worker_success,
        )

    @patch(dir_path + ".data_presenter.create_error_message")
    @patch(dir_path + ".data_presenter.AsyncTask")
    def test_worker_not_started_while_searching(self, mock_worker, mock_error):
        self.view.is_searching.return_value = True
        self.view.get_filenames_valid.return_value = True

        self.presenter.on_load_clicked()

        self.assertEqual(0, mock_worker.call_count)
        self.assertEqual(0, self.view.get_filenames_to_load.call_count)
        mock_error.assert_called_with(self.view, "Mantid is searching for files. Please wait.")

    @patch(dir_path + ".data_presenter.create_error_message")
    @patch(dir_path + ".data_presenter.AsyncTask")
    def test_worker_not_started_while_files_invalid(self, mock_worker, mock_error):
        self.view.is_searching.return_value = False
        self.view.get_filenames_valid.return_value = False

        self.presenter.on_load_clicked()

        self.assertEqual(0, mock_worker.call_count)
        self.assertEqual(0, self.view.get_filenames_to_load.call_count)
        mock_error.assert_called_with(self.view, "Entered files are not valid.")

    @patch(dir_path + ".data_presenter.logger")
    def test_worker_error(self, logger):
        self.view.sig_enable_load_button = mock.MagicMock()

        self.presenter._on_worker_error("info")

        logger.error.assert_called_with("Error occurred when loading files.")
        self.assertEqual(1, self.view.sig_enable_load_button.emit.call_count)
        self.view.sig_enable_load_button.emit.called_with(True)

    def test_worker_success_valid_filename(self):
        model_dict = {"ENGINX_1_bank_1": self.ws1, "ENGINX_2_bank_South": self.ws3}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.get_sample_log_from_ws.return_value = "bankOrRunNumber"
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter._on_worker_success("info")

        self.assertEqual(1, self.view.remove_all.call_count)
        self.view.add_table_row.assert_any_call("bankOrRunNumber", "bankOrRunNumber", False, False, 50, 600, True)
        self.view.add_table_row.assert_any_call("bankOrRunNumber", "bankOrRunNumber", False, False, 50, 0.02, True)

    @patch(dir_path + ".data_presenter.logger")
    def test_worker_success_invalid_filename(self, mock_logger):
        model_dict = {"invalid": self.ws1, "invalid2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.get_sample_log_from_ws.side_effect = RuntimeError("No sample logs present")
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter._on_worker_success("info")

        self.assertEqual(1, self.view.remove_all.call_count)
        self.assertEqual(2, self.view.add_table_row.call_count)
        self.view.add_table_row.assert_any_call("invalid", "N/A", False, False, 50, 600, True)
        self.assertEqual(2, mock_logger.warning.call_count)

    @patch(dir_path + ".data_presenter.logger")
    def test_worker_success_valid_filename_no_sample_logs(self, mock_logger):
        model_dict = {"INSTRUMENT_10_bank_2": self.ws1, "INSTRUMENT_20_bank_1": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.get_sample_log_from_ws.side_effect = RuntimeError("No sample logs present")
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter._on_worker_success("info")

        self.assertEqual(1, self.view.remove_all.call_count)
        self.assertEqual(2, self.view.add_table_row.call_count)
        self.view.add_table_row.assert_any_call("10", "2", False, False, 50, 600, True)
        self.view.add_table_row.assert_any_call("20", "1", False, False, 50, 600, True)
        self.assertEqual(2, mock_logger.notice.call_count)

    def test_remove_workspace_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.remove_workspace.side_effect = model_dict.pop("name1")
        self.model.get_all_workspace_names.return_value = ["name1", "name2"]
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.plot_removed_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.remove_workspace("name1")

        self.model.remove_workspace.assert_called_once_with("name1")
        self.assertEqual({"name2": 0}, self.presenter.row_numbers)

    def test_remove_workspace_not_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.remove_workspace("name3")

        self.assertEqual({"name1": self.ws1, "name2": self.ws2}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)
        self.model.update_sample_log_workspace_group.assert_not_called()

    def test_remove_workspace_not_tracked_but_is_log_workspaces_group(self):
        self.model.get_all_workspace_names.return_value = ["name2"]
        self.model.get_all_log_workspaces_names.return_value = ["name1", "name2"]
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.plot_removed_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.remove_workspace("name1")

        self.model.remove_workspace.assert_not_called()
        self.model.update_sample_log_workspace_group.assert_called_once()

    def test_rename_workspace_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.get_all_workspace_names.return_value = ["name1", "name2", "new"]  # just return all so has no effect
        # lambda function to replace dict with new key and same ordering as before
        self.model.update_workspace_name.side_effect = lambda old, new: model_dict.update(
            {(key if key != old else new): val for key, val in (list(model_dict.items()), model_dict.clear())[0]}
        )
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.rename_workspace("name1", "new")
        # ADS rename always accompanied by replace so call that as well here
        self.presenter.replace_workspace("new", self.ws1)
        self.assertEqual({"new": self.ws1, "name2": self.ws2}, model_dict)
        self.assertTrue("new" in self.presenter.row_numbers)
        self.assertFalse("name1" == self.presenter.row_numbers)
        self.assertEqual(1, self.presenter.all_plots_removed_notifier.notify_subscribers.call_count)
        self.model.update_workspace_name.assert_called_once()

    def test_rename_workspace_not_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        # lambda function to replace dict with new key and same ordering as before
        self.model.update_workspace_name = lambda old, new: model_dict.update(
            {(key if key != old else new): val for key, val in (list(model_dict.items()), model_dict.clear())[0]}
        )
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.rename_workspace("name3", "new")

        self.assertEqual({"name1": self.ws1, "name2": self.ws2}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)
        self.assertEqual(0, self.presenter.all_plots_removed_notifier.notify_subscribers.call_count)
        self.model.update_sample_log_workspace_group.assert_not_called()

    def test_remove_all_tracked_workspaces(self):
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter._remove_all_tracked_workspaces()

        self.assertEqual({}, self.presenter.row_numbers)
        self.assertEqual(1, self.presenter.all_plots_removed_notifier.notify_subscribers.call_count)
        self.model.delete_workspaces.assert_called_once()

    def test_replace_workspace_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.get_all_workspace_names.return_value = ["name1", "name2"]
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.replace_workspace("name1", self.ws3)

        self.assertTrue("name1" in self.presenter.row_numbers)
        self.assertTrue("name2" in self.presenter.row_numbers)
        self.model.replace_workspace.assert_called_once_with("name1", self.ws3)

    def test_replace_workspace_not_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.replace_workspace("name3", self.ws3)

        self.assertEqual({"name1": self.ws1, "name2": self.ws2}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)

    def test_removing_selected_rows(self):
        def removeWorkspaceIfPresent(loaded_ws_name):
            model_dict.pop(loaded_ws_name, "")
            return model_dict.values()

        self._setup_fitting_table_rows(["name1", "name2", "name3"])
        model_dict = {"name1": self.ws1, "name2": self.ws2, "name3": self.ws3}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.delete_workspace.side_effect = removeWorkspaceIfPresent
        self.model.delete_workspace.return_value = ["name1", "name2"]
        self.view.remove_selected.return_value = [0, 2]
        self.presenter.plot_removed_notifier = mock.MagicMock()
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter._remove_selected_tracked_workspaces()
        self.presenter._handle_table_cell_changed(0, 2)

        self.assertEqual(1, self.view.remove_selected.call_count)
        test_dict = data_presenter.TwoWayRowDict()
        test_dict["name2"] = 0
        self.assertEqual(self.presenter.row_numbers, test_dict)
        self.assertEqual(self.model.delete_workspace.call_count, 2)
        self.assertEqual(1, self.presenter.all_plots_removed_notifier.notify_subscribers.call_count)
        self.assertEqual(1, self.presenter.plot_added_notifier.notify_subscribers.call_count)

    def test_handle_table_cell_changed_checkbox_ticked(self):
        mocked_table_item = mock.MagicMock()
        self.view.get_item_checked.side_effect = _get_item_checked_mock
        self.view.get_table_item.return_value = mocked_table_item
        self._setup_fitting_table_rows(["name1", "name2"])
        self.model.get_active_ws.return_value = self.ws1
        self.model.get_active_ws_name.return_value = "name1"
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.plot_removed_notifier = mock.MagicMock()

        self.presenter._handle_table_cell_changed(0, 2)

        self.assertEqual(1, self.presenter.plot_added_notifier.notify_subscribers.call_count)
        self.presenter.plot_added_notifier.notify_subscribers.assert_any_call(self.ws1)
        self.assertEqual(0, self.presenter.plot_removed_notifier.notify_subscribers.call_count)

    @patch(dir_path + ".data_presenter.ADS")
    def test_handle_table_cell_changed_ticked_to_plot_on_dspacing_when_tof_plotted(self, ads_patch):
        self.view.get_item_checked.side_effect = _get_item_checked_mock
        ws2 = self._setup_fitting_table_rows(
            ["TOF_WS_Name1", "dSpacing_WS_Name1"],
            "dSpacing",
        )
        self.model.get_active_ws.return_value = ws2
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.plot_removed_notifier = mock.MagicMock()
        self.presenter.plotted.add("TOF_WS_Name1")
        ads_patch.retrieve().getXDimension().name = "TOF"

        self.presenter._handle_table_cell_changed(1, 2)  # tick the 2nd dSpacing row to be plotted

        self.assertEqual(1, self.view.set_item_checkstate.call_count)
        self.assertEqual(0, self.presenter.plot_added_notifier.notify_subscribers.call_count)
        self.assertEqual(1, len(self.presenter.plotted))
        self.assertEqual("TOF_WS_Name1", next(iter(self.presenter.plotted)))
        self.view.set_item_checkstate.assert_called_once_with(1, 2, False)

    @patch(dir_path + ".data_presenter.ADS")
    def test_handle_table_cell_changed_ticked_to_plot_on_tof_when_tof_plotted(self, ads_patch):
        self.view.get_item_checked.side_effect = _get_item_checked_mock
        ws2 = self._setup_fitting_table_rows(["TOF_WS_Name1", "TOF_WS_Name2"], "TOF")
        self.model.get_active_ws.return_value = ws2
        self.model.get_active_ws_name.return_value = "TOF_WS_Name2"
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.plot_removed_notifier = mock.MagicMock()
        self.presenter.plotted.add("TOF_WS_Name1")
        ads_patch.retrieve().getXDimension().name = "TOF"

        self.presenter._handle_table_cell_changed(1, 2)  # tick the 2nd TOF row to be plotted

        self.assertEqual(0, self.view.set_item_checkstate.call_count)
        self.assertEqual(1, self.presenter.plot_added_notifier.notify_subscribers.call_count)
        self.assertEqual(2, len(self.presenter.plotted))

    @patch(dir_path + ".data_presenter.logger")
    @patch(dir_path + ".data_presenter.ADS")
    def test_loading_new_dspacing_file_with_ticked_to_plot_when_tof_plotted(self, ads_patch, mock_logger):
        self.presenter.plotted = {"TOF_WS_Name1"}
        ws1 = mock.MagicMock()
        ws1.getXDimension().name = "TOF"
        ws2 = mock.MagicMock()
        ws2.getXDimension().name = "dSpacing"
        self.model.get_sample_log_from_ws.return_value = "bankOrRunNumber"
        self.model.get_last_added.return_value = ["dSpacing_WS_Name1"]
        model_dict = {"TOF_WS_Name1": ws1, "dSpacing_WS_Name1": ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.view.get_add_to_plot.return_value = True

        self.model.get_active_ws_name.side_effect = lambda name: name
        self.model.get_active_ws.side_effect = lambda name: model_dict.get(name, None)
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()
        self.presenter._add_row_to_table = mock.MagicMock()

        self.presenter._on_worker_success("info")

        self.assertEqual(2, self.presenter._add_row_to_table.call_count)
        self.assertEqual(0, len(self.presenter.plotted))

    def test_handle_table_cell_changed_checkbox_unticked(self):
        self.view.get_item_checked.return_value = False
        self._setup_fitting_table_rows(["name1", "name2"])
        self.model.get_active_ws.return_value = self.ws1
        self.model.get_active_ws_name.return_value = "name1"
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.plot_removed_notifier = mock.MagicMock()

        self.presenter._handle_table_cell_changed(0, 2)

        self.assertEqual(0, self.presenter.plot_added_notifier.notify_subscribers.call_count)
        self.assertEqual(1, self.presenter.plot_removed_notifier.notify_subscribers.call_count)
        self.presenter.plot_removed_notifier.notify_subscribers.assert_any_call(self.ws1)

    def test_handle_table_cell_changed_other_element(self):
        mocked_table_item = mock.MagicMock()
        mocked_table_item.checkState.return_value = 2
        self.view.get_table_item.return_value = mocked_table_item
        self._setup_fitting_table_rows(["name1", "name2"])
        self.model.get_active_ws.return_value = self.ws1
        self.model.get_active_ws_name.return_value = "name1"
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.plot_removed_notifier = mock.MagicMock()

        self.presenter._handle_table_cell_changed(1, 1)

        self.assertEqual(0, self.presenter.plot_added_notifier.notify_subscribers.call_count)
        self.assertEqual(0, self.presenter.plot_removed_notifier.notify_subscribers.call_count)

    def test_bgsub_first_time(self):
        # setup row
        self._setup_bgsub_test()
        # subtract background for first time
        self.view.get_item_checked.return_value = True  # determines is bgSubtract is checked or not
        self.view.read_bg_params_from_table.return_value = [True, 40, 800, False]
        self.model.get_bgsub_workspaces.return_value = {"name2": None}
        self.presenter._handle_table_cell_changed(1, 3)
        self.model.create_or_update_bgsub_ws.assert_called_with("name2", self.view.read_bg_params_from_table(0))

    def test_bgparam_changed_when_bgsub_False(self):
        # setup row
        self._setup_bgsub_test()
        # activate bg subtraction before background is made (nothing should happen)
        self.view.get_item_checked.return_value = False
        self.presenter._handle_table_cell_changed(1, 4)
        self.model.create_or_update_bgsub_ws.assert_not_called()

    def test_bgparam_changed_when_bgsub_True(self):
        # setup row
        self._setup_bgsub_test()
        self.view.get_item_checked.return_value = True
        self.model.get_bg_params.return_value = {"name2": [True, 40, 800, False]}
        self.view.read_bg_params_from_table.return_value = [True, 200, 800, False]
        self.model.get_bgsub_workspaces.return_value = self.model.get_loaded_workspaces()
        self.presenter._handle_table_cell_changed(1, 4)
        self.model.create_or_update_bgsub_ws.assert_called_once_with("name2", self.view.read_bg_params_from_table(0))

    def test_undo_bgsub(self):
        self._setup_bgsub_test()
        # untick background subtraction
        self.view.get_item_checked.return_value = False
        self.presenter._handle_table_cell_changed(1, 3)
        self.model.create_or_update_bgsub_ws.assert_not_called()

    def test_inspect_bg_button_enables_and_disables(self):
        self.view.get_item_checked.return_value = False
        self._setup_fitting_table_rows(["name1", "name2"])
        self.view.get_selected_rows.return_value = self.presenter.row_numbers
        self.presenter._handle_selection_changed()
        self.view.set_inspect_bg_button_enabled.assert_called_with(True)
        self.view.get_selected_rows.return_value = {}
        self.presenter._handle_selection_changed()
        self.view.set_inspect_bg_button_enabled.assert_called_with(False)

    def _setup_bgsub_test(self):
        mocked_table_item = mock.MagicMock()
        mocked_table_item.checkState.return_value = True
        self.view.get_table_item.return_value = mocked_table_item
        self._setup_fitting_table_rows(["name1", "name2"])
        self.model.get_loaded_workspaces.return_value = {"name1": self.ws1, "name2": self.ws2}
        self.model.estimate_background.return_value = self.ws2, True

    def _setup_fitting_table_rows_for_invalid_bg_params(
        self, table_ws_names, changing_ws, original_bg_params, new_bg_params, get_item_checked_value, create_or_update_bgsub
    ):
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        for ws_index, ws in enumerate(table_ws_names):
            self.presenter.row_numbers[ws] = ws_index
        self.view.read_bg_params_from_table.return_value = new_bg_params
        self.model.get_bg_params.return_value = {changing_ws: original_bg_params}
        self.view.get_item_checked.return_value = get_item_checked_value
        self.model.create_or_update_bgsub_ws.return_value = create_or_update_bgsub

    def test_handle_table_cell_changed_invalid_bg_params_on_niter(self):
        original_bg_params = [True, 50, 600, True]
        self._setup_fitting_table_rows_for_invalid_bg_params(
            table_ws_names=["WS_Name1", "WS_Name2"],
            changing_ws="WS_Name2",
            original_bg_params=original_bg_params,
            new_bg_params=[True, -100, 200, True],
            get_item_checked_value=True,
            create_or_update_bgsub=False,
        )

        self.presenter._handle_table_cell_changed(1, 4)
        self.assertEqual(2, self.view.set_table_column.call_count)
        calls = [mock.call(1, 4, original_bg_params[1]), mock.call(1, 5, original_bg_params[2])]
        self.view.set_table_column.assert_has_calls(calls, any_order=False)

    def test_handle_table_cell_changed_invalid_bg_params_on_xwindow(self):
        original_bg_params = [True, 50, 600, True]
        self._setup_fitting_table_rows_for_invalid_bg_params(
            table_ws_names=["WS_Name1", "WS_Name2"],
            changing_ws="WS_Name2",
            original_bg_params=original_bg_params,
            new_bg_params=[True, 50, -200, True],
            get_item_checked_value=True,
            create_or_update_bgsub=False,
        )
        self.presenter._handle_table_cell_changed(1, 5)
        self.assertEqual(2, self.view.set_table_column.call_count)
        calls = [mock.call(1, 4, original_bg_params[1]), mock.call(1, 5, original_bg_params[2])]
        self.view.set_table_column.assert_has_calls(calls, any_order=False)

    def test_handle_table_cell_changed_invalid_bg_params_on_subtract_bg(self):
        original_bg_params = [False, 50, 600, True]
        self._setup_fitting_table_rows_for_invalid_bg_params(
            table_ws_names=["WS_Name1", "WS_Name2"],
            changing_ws="WS_Name2",
            original_bg_params=original_bg_params,
            new_bg_params=[True, -50, -200, True],
            get_item_checked_value=True,
            create_or_update_bgsub=False,
        )
        self.presenter._update_plotted_ws_with_sub_state = mock.MagicMock()
        self.presenter._handle_table_cell_changed(1, 3)
        self.assertEqual(2, self.view.set_table_column.call_count)
        calls = [mock.call(1, 4, original_bg_params[1]), mock.call(1, 5, original_bg_params[2])]
        self.view.set_table_column.assert_has_calls(calls, any_order=False)
        self.presenter._update_plotted_ws_with_sub_state.assert_called_once_with("WS_Name2", True)


if __name__ == "__main__":
    unittest.main()
