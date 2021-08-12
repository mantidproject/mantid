# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from unittest.mock import patch
from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling import data_model, data_presenter, data_view

dir_path = "Engineering.gui.engineering_diffraction.tabs.fitting.data_handling"


def _get_item_checked_mock(_, arg):
    if arg == 2:
        return True
    elif arg == 3:
        return False


class FittingDataPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(data_model.FittingDataModel)
        self.view = mock.create_autospec(data_view.FittingDataView)
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

    @patch(dir_path + ".data_presenter.AsyncTask")
    def test_worker_started_correctly(self, mock_worker):
        self.view.is_searching.return_value = False
        self.view.get_filenames_to_load.return_value = "/a/file/to/load.txt, /another/one.nxs"
        self.model.load_files = "mocked model method"

        self.presenter.on_load_clicked()

        mock_worker.assert_called_with("mocked model method",
                                       ("/a/file/to/load.txt, /another/one.nxs",),
                                       error_cb=self.presenter._on_worker_error,
                                       finished_cb=self.presenter._emit_enable_load_button_signal,
                                       success_cb=self.presenter._on_worker_success)

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
        self.view.add_table_row.assert_any_call("bankOrRunNumber", "bankOrRunNumber", False, False, 100, 1000, True)
        self.view.add_table_row.assert_any_call("bankOrRunNumber", "bankOrRunNumber", False, False, 100, 0.05, True)

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
        self.view.add_table_row.assert_any_call("invalid", "N/A", False, False, 100, 1000, True)
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
        self.view.add_table_row.assert_any_call("10", "2", False, False, 100, 1000, True)
        self.view.add_table_row.assert_any_call("20", "1", False, False, 100, 1000, True)
        self.assertEqual(2, mock_logger.notice.call_count)

    def test_remove_workspace_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.plot_removed_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.remove_workspace("name1")

        self.assertEqual({"name2": self.ws2}, model_dict)
        self.assertEqual({"name2": 0}, self.presenter.row_numbers)
        self.assertEqual(1, self.presenter.plot_removed_notifier.notify_subscribers.call_count)

    def test_remove_workspace_not_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.remove_workspace("name3")

        self.assertEqual({"name1": self.ws1, "name2": self.ws2}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)

    def test_rename_workspace_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        # lambda function to replace dict with new key and same ordering as before
        self.model.update_workspace_name = lambda old, new: model_dict.update(
            {(key if key != old else new): val for key, val in (list(model_dict.items()), model_dict.clear())[0]})
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.rename_workspace("name1", "new")
        self.assertEqual({"new": self.ws1, "name2": self.ws2}, model_dict)
        self.assertTrue("new" in self.presenter.row_numbers)
        self.assertFalse("name1" == self.presenter.row_numbers)
        self.assertEqual(1, self.presenter.all_plots_removed_notifier.notify_subscribers.call_count)
        self.model.update_log_workspace_group.assert_called_once()

    def test_rename_workspace_not_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        # lambda function to replace dict with new key and same ordering as before
        self.model.update_workspace_name = lambda old, new: model_dict.update(
            {(key if key != old else new): val for key, val in (list(model_dict.items()), model_dict.clear())[0]})
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.rename_workspace("name3", "new")

        self.assertEqual({"name1": self.ws1, "name2": self.ws2}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)
        self.assertEqual(0, self.presenter.all_plots_removed_notifier.notify_subscribers.call_count)
        self.model.update_log_workspace_group.assert_not_called()

    def test_remove_all_tracked_workspaces(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter._remove_all_tracked_workspaces()

        self.assertEqual({}, model_dict)
        self.assertEqual({}, self.presenter.row_numbers)
        self.assertEqual(1, self.presenter.all_plots_removed_notifier.notify_subscribers.call_count)
        self.model.clear_logs.assert_called_once()

    def test_replace_workspace_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter.replace_workspace("name1", self.ws3)

        self.assertEqual({"name1": self.ws3, "name2": self.ws2}, model_dict)
        self.assertTrue("name1" in self.presenter.row_numbers)
        self.assertTrue("name2" in self.presenter.row_numbers)

    def test_replace_workspace_not_tracked(self):
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.replace_workspace("name3", self.ws3)

        self.assertEqual({"name1": self.ws1, "name2": self.ws2}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)

    def test_removing_selected_rows(self):
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        self.presenter.row_numbers["name1"] = 0
        self.presenter.row_numbers["name2"] = 1
        self.presenter.row_numbers["name3"] = 2
        model_dict = {"name1": self.ws1, "name2": self.ws2, "name3": self.ws3}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.view.remove_selected.return_value = [0, 2]
        self.presenter.plot_removed_notifier = mock.MagicMock()
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.all_plots_removed_notifier = mock.MagicMock()

        self.presenter._remove_selected_tracked_workspaces()

        self.assertEqual(1, self.view.remove_selected.call_count)
        test_dict = data_presenter.TwoWayRowDict()
        test_dict["name2"] = 0
        self.assertEqual(self.presenter.row_numbers, test_dict)
        self.assertEqual(model_dict, {"name2": self.ws2})
        self.assertEqual(2, self.presenter.plot_removed_notifier.notify_subscribers.call_count)
        self.assertEqual(1, self.presenter.plot_added_notifier.notify_subscribers.call_count)
        self.model.remove_log_rows.assert_called_once_with(self.view.remove_selected())

    def test_handle_table_cell_changed_checkbox_ticked(self):
        mocked_table_item = mock.MagicMock()
        self.view.get_item_checked.side_effect = _get_item_checked_mock
        mocked_table_item.checkState.return_value = 2
        self.view.get_table_item.return_value = mocked_table_item
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        self.presenter.row_numbers["name1"] = 0
        self.presenter.row_numbers["name2"] = 1
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.plot_added_notifier = mock.MagicMock()
        self.presenter.plot_removed_notifier = mock.MagicMock()

        self.presenter._handle_table_cell_changed(0, 2)

        self.assertEqual(1, self.presenter.plot_added_notifier.notify_subscribers.call_count)
        self.presenter.plot_added_notifier.notify_subscribers.assert_any_call(self.ws1)
        self.assertEqual(0, self.presenter.plot_removed_notifier.notify_subscribers.call_count)

    def test_handle_table_cell_changed_checkbox_unticked(self):
        self.view.get_item_checked.return_value = False
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        self.presenter.row_numbers["name1"] = 0
        self.presenter.row_numbers["name2"] = 1
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
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
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        self.presenter.row_numbers["name1"] = 0
        self.presenter.row_numbers["name2"] = 1
        model_dict = {"name1": self.ws1, "name2": self.ws2}
        self.model.get_loaded_workspaces.return_value = model_dict
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
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        self.presenter.row_numbers["name1"] = 0
        self.presenter.row_numbers["name2"] = 1
        self.view.get_selected_rows.return_value = self.presenter.row_numbers
        self.presenter._handle_selection_changed()
        self.view.set_inspect_bg_button_enabled.assert_called_with(False)
        self.view.get_item_checked.return_value = True
        self.presenter._handle_selection_changed()
        self.view.set_inspect_bg_button_enabled.assert_called_with(True)

    def _setup_bgsub_test(self):
        mocked_table_item = mock.MagicMock()
        mocked_table_item.checkState.return_value = True
        self.view.get_table_item.return_value = mocked_table_item
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        self.presenter.row_numbers["name1"] = 0
        self.presenter.row_numbers["name2"] = 1
        self.model.get_loaded_workspaces.return_value = {"name1": self.ws1, "name2": self.ws2}
        self.model.estimate_background.return_value = self.ws2


if __name__ == '__main__':
    unittest.main()
