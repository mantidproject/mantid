# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantid.py3compat import mock
from mantid.py3compat.mock import patch

from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling import data_model, data_presenter, data_view

dir_path = "Engineering.gui.engineering_diffraction.tabs.fitting.data_handling"


class FittingDataPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(data_model.FittingDataModel)
        self.view = mock.create_autospec(data_view.FittingDataView)
        self.presenter = data_presenter.FittingDataPresenter(self.model, self.view)

    @patch(dir_path + ".data_presenter.AsyncTask")
    def test_worker_started_correctly(self, mock_worker):
        self.view.is_searching.return_value = False
        self.view.get_filenames_to_load.return_value = "/a/file/to/load.txt, /another/one.nxs"
        self.model.load_files = "mocked model method"

        self.presenter.on_load_clicked()

        mock_worker.assert_called_with("mocked model method",
                                       ("/a/file/to/load.txt, /another/one.nxs",),
                                       error_cb=self.presenter._on_worker_error,
                                       finished_cb=self.presenter._emit_enable_button_signal,
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
        model_dict = {"ENGINX_1_bank_1": "ws1", "ENGINX_2_bank_South": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.get_sample_log_from_ws.return_value = "bankOrRunNumber"

        self.presenter._on_worker_success("info")

        self.assertEqual(1, self.view.remove_all.call_count)
        self.view.add_table_row.assert_any_call("bankOrRunNumber", "bankOrRunNumber")
        self.view.add_table_row.assert_any_call("bankOrRunNumber", "bankOrRunNumber")

    @patch(dir_path + ".data_presenter.logger")
    def test_worker_success_invalid_filename(self, mock_logger):
        model_dict = {"invalid": "ws1", "invalid2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.get_sample_log_from_ws.side_effect = RuntimeError("No sample logs present")

        self.presenter._on_worker_success("info")

        self.assertEqual(1, self.view.remove_all.call_count)
        self.assertEqual(2, self.view.add_table_row.call_count)
        self.view.add_table_row.assert_any_call("invalid", "N/A")
        self.assertEqual(2, mock_logger.warning.call_count)

    @patch(dir_path + ".data_presenter.logger")
    def test_worker_success_valid_filename_no_sample_logs(self, mock_logger):
        model_dict = {"INSTRUMENT_10_bank_2": "ws1", "INSTRUMENT_20_bank_1": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.model.get_sample_log_from_ws.side_effect = RuntimeError("No sample logs present")

        self.presenter._on_worker_success("info")

        self.assertEqual(1, self.view.remove_all.call_count)
        self.assertEqual(2, self.view.add_table_row.call_count)
        self.view.add_table_row.assert_any_call("10", "2")
        self.view.add_table_row.assert_any_call("20", "1")
        self.assertEqual(2, mock_logger.notice.call_count)

    def test_remove_workspace_tracked(self):
        model_dict = {"name1": "ws1", "name2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.remove_workspace("name1")

        self.assertEqual({"name2": "ws2"}, model_dict)
        self.assertEqual({"name2": 0}, self.presenter.row_numbers)

    def test_remove_workspace_not_tracked(self):
        model_dict = {"name1": "ws1", "name2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.remove_workspace("name3")

        self.assertEqual({"name1": "ws1", "name2": "ws2"}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)

    def test_rename_workspace_tracked(self):
        model_dict = {"name1": "ws1", "name2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.rename_workspace("name1", "new")

        self.assertEqual({"new": "ws1", "name2": "ws2"}, model_dict)
        self.assertTrue("new" in self.presenter.row_numbers)
        self.assertFalse("name1" is self.presenter.row_numbers)

    def test_rename_workspace_not_tracked(self):
        model_dict = {"name1": "ws1", "name2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.rename_workspace("name3", "new")

        self.assertEqual({"name1": "ws1", "name2": "ws2"}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)

    def test_clear_workspaces(self):
        model_dict = {"name1": "ws1", "name2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.clear_workspaces()

        self.assertEqual({}, model_dict)
        self.assertEqual({}, self.presenter.row_numbers)

    def test_replace_workspace_tracked(self):
        model_dict = {"name1": "ws1", "name2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.replace_workspace("name1", "newWs")

        self.assertEqual({"name1": "newWs", "name2": "ws2"}, model_dict)
        self.assertTrue("name1" in self.presenter.row_numbers)
        self.assertTrue("name2" in self.presenter.row_numbers)

    def test_replace_workspace_not_tracked(self):
        model_dict = {"name1": "ws1", "name2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.presenter.row_numbers = {"name1": 0, "name2": 1}

        self.presenter.replace_workspace("name3", "new")

        self.assertEqual({"name1": "ws1", "name2": "ws2"}, model_dict)
        self.assertEqual({"name1": 0, "name2": 1}, self.presenter.row_numbers)

    def test_removing_selected_rows(self):
        self.presenter.row_numbers = data_presenter.TwoWayRowDict()
        self.presenter.row_numbers["name1"] = 0
        self.presenter.row_numbers["name2"] = 1
        model_dict = {"name1": "ws1", "name2": "ws2"}
        self.model.get_loaded_workspaces.return_value = model_dict
        self.view.remove_selected.return_value = [0]

        self.presenter._remove_selected_tracked_workspaces()

        self.assertEqual(1, self.view.remove_selected.call_count)
        test_dict = data_presenter.TwoWayRowDict()
        test_dict["name2"] = 0
        self.assertEqual(self.presenter.row_numbers, test_dict)
        self.assertEqual(model_dict, {"name2": "ws2"})


if __name__ == '__main__':
    unittest.main()
